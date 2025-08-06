#include "TcpClient.hpp"

#include <cstdlib> // std::wctombs

using namespace Net;

TcpClient::TcpClient(const quint8 _connType, const QString _connTypeName, QObject* parent)
    : NetConnection(_connType, _connTypeName, parent)
    , m_pTcpSocket(createSocket())
{
    // m_pTcpSocket = a_socket ? a_socket : new QTcpSocket(this);
    connect(m_pTcpSocket, &QTcpSocket::connected,    this, &TcpClient::onConnected);
    connect(m_pTcpSocket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_pTcpSocket, &QTcpSocket::readyRead,    this, &TcpClient::readReceived);
    connect(m_pTcpSocket, &QTcpSocket::stateChanged, this, &NetConnection::socketStateChanged);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &TcpClient::tryConnectToHost, Qt::QueuedConnection);
}

TcpClient::~TcpClient()
{
    m_isReconnectEnabled = false;
    TcpClient::closeConnection();
    delete m_pTcpSocket;
}

Net::ConnectionState TcpClient::openConnection(ConnectionSettings const& a_connectionSettings)
{
    if (m_connectionState == ConnectionState::Created)
        return m_connectionState;
    m_connectionSettings = a_connectionSettings;
    if (m_connectionSettings.ipDestination.isNull())
    {
        f_logError(QString("%1: Unable to open connection - wrong TCP server (destination) IP!").arg(nameId()));
        m_connectionState = ConnectionState::IncorrectIp;
        emit openedConnection(false);
        return m_connectionState;
    }
    if ((m_connectionSettings.ipLocal != QHostAddress::Null))
    {
        bool isBinded = m_pTcpSocket->bind(m_connectionSettings.ipLocal, m_connectionSettings.portIn,
                                           QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
        if (isBinded == false)
        {
            m_connectionState = ConnectionState::NotCreated;
            f_logError(QString("%1: Failed to open connection - %2").arg(nameId()).arg(m_pTcpSocket->errorString()));
            emit openedConnection(false);
            return m_connectionState;
        }

        m_connectionState = ConnectionState::Created;
    }
    connect(m_pTcpSocket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this, &TcpClient::printError);
    tryConnectToHost();
    return m_connectionState;
}

void TcpClient::closeConnection()
{
    m_reconnectTimer->stop();
    m_pTcpSocket->close();
    if (m_connectionState == ConnectionState::Created)
        f_logGeneral(QString("%1: Closed connection").arg(nameId()));
    m_connectionState = ConnectionState::NotCreated;
    disconnect(m_pTcpSocket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this, &TcpClient::printError);
    emit closedConnection();
    return;
}

void TcpClient::tryConnectToHost()
{
    m_pTcpSocket->connectToHost(m_connectionSettings.ipDestination, m_connectionSettings.portOut, QIODevice::ReadWrite);
    bool isConnected = m_pTcpSocket->waitForConnected(m_waitForConnectedInterval);
    if (isConnected)
    {
        emit openedConnection(true);
        m_reconnectTimer->stop();
        authorize();
    }
    else
    {
        emit openedConnection(false);
        if (m_isReconnectEnabled)
            m_reconnectTimer->start(m_reconnectInterval);
    }
    return;
}

// Send the message and return the number of bytes that were actually sent, or -1 in case of error.
qint64 TcpClient::sendMessage(const QByteArray& msg)
{
    qint64 bytesWritten = -1;
    if (m_pTcpSocket->state() != QAbstractSocket::ConnectedState)
        return -1;

    const decltype(Net::PendingMessage::pendingSize) msgSize = static_cast<decltype(msgSize)>(msg.size());
    QByteArray header(reinterpret_cast<const char*>(&msgSize), sizeof(msgSize));
    bytesWritten = m_pTcpSocket->write(header + msg);
    m_pTcpSocket->waitForBytesWritten(1000);
    if (bytesWritten > 0)
        emit writeDone((bytesWritten == msg.size()) ? msg : msg.mid(0, bytesWritten));
    return bytesWritten;
}

void TcpClient::onConnected()
{
    f_logGeneral(QString("%1: %2:%3 connected to TCP server %4:%5")
                 .arg(nameId())
                 .arg(m_pTcpSocket->localAddress().toString())
                 .arg(m_pTcpSocket->localPort())
                 .arg(m_pTcpSocket->peerAddress().toString())
                 .arg(m_pTcpSocket->peerPort()));
}

void TcpClient::onDisconnected()
{
    f_logGeneral(QString("%1: %2:%3 disconnected from TCP server %4:%5")
                 .arg(nameId())
                 .arg(m_pTcpSocket->localAddress().toString())
                 .arg(m_pTcpSocket->localPort())
                 .arg(m_pTcpSocket->peerAddress().toString())
                 .arg(m_pTcpSocket->peerPort()));
    if (m_isReconnectEnabled && m_pTcpSocket->state() != QAbstractSocket::ConnectedState)
        m_reconnectTimer->start(m_reconnectInterval);
}

void TcpClient::authorize()
{
    if (!m_isAuthorizationEnabled)
        return;
    QByteArray msg;
    MAKE_QDATASTREAM_NET(stream, &msg, QIODevice::WriteOnly);
    stream << m_loginData;
    sendMessage(msg);
}

void TcpClient::readReceived()
{
    MAKE_QDATASTREAM_NET_D(streamSocket, m_pTcpSocket);
    while (m_pTcpSocket->bytesAvailable() > 0)
    {
        if (m_pendingMsg.pendingSize == 0)
        {
            if (m_pTcpSocket->bytesAvailable() < m_headerSize)
                break;
            streamSocket >> m_pendingMsg.pendingSize;
            m_pendingMsg.msg.resize(m_pendingMsg.pendingSize);
            m_pendingMsg.curPos = 0;
        }
        const decltype(Net::PendingMessage::pendingSize) availableSize = static_cast<decltype(availableSize)>(m_pTcpSocket->bytesAvailable());
        if (availableSize < m_pendingMsg.pendingSize)
        {
            streamSocket.readRawData((m_pendingMsg.msg.data() + m_pendingMsg.curPos), availableSize);
            emit readPartialDone(m_pendingMsg.msg.mid(m_pendingMsg.curPos, availableSize));
            m_pendingMsg.curPos += availableSize;
            m_pendingMsg.pendingSize -= availableSize;
            break;
        }
        streamSocket.readRawData((m_pendingMsg.msg.data() + m_pendingMsg.curPos), m_pendingMsg.pendingSize);
        emit readPartialDone(m_pendingMsg.msg.mid(m_pendingMsg.curPos, m_pendingMsg.pendingSize));
        emit readDone(m_pendingMsg.msg);
        m_pendingMsg.curPos = 0;
        m_pendingMsg.pendingSize = 0;
        f_onReceivedMessage(m_pendingMsg.msg, this, {m_connectionSettings.ipDestination, m_connectionSettings.portOut});
    }
    return;
}

void TcpClient::setEnableReconnect(bool isEnabled)
{
    m_isReconnectEnabled = isEnabled;
    QTimer::singleShot(0, this, [this](){
        if (m_isReconnectEnabled == false)
            m_reconnectTimer->stop();
        else if ((m_isReconnectEnabled == true) && (m_pTcpSocket->state() != QAbstractSocket::ConnectedState))
            m_reconnectTimer->start(m_reconnectInterval);
    });
    return;
}

void TcpClient::setWaitTimes(int reconnectInterval, int waitForConnectedInterval)
{
    m_reconnectInterval = reconnectInterval;
    m_waitForConnectedInterval = waitForConnectedInterval;
    // reconnectTimer is singleShot and calls start(m_reconnectInterval) each time, so calling QTimer::setInterval here is not required
    // QTimer::singleShot(0, this, [this](){ m_reconnectTimer->setInterval(m_reconnectInterval); }); // queued call in case caller's thread != timer's thread
}

void TcpClient::setLoginData(Net::LoginData a_loginData)
{
    if (m_connectionState == Net::ConnectionState::Created)
    {
        f_logGeneral(QString("%1: called setLoginData() while connection is open - action forbidden").arg(nameId()));
        return;
    }
    m_loginData = a_loginData;
}

void TcpClient::setAuthorizationEnabled(bool isEnabled)
{
    if (m_connectionState == Net::ConnectionState::Created)
    {
        f_logGeneral(QString("%1: called setAuthorizationEnabled() while connection is open - action forbidden").arg(nameId()));
        return;
    }
    m_isAuthorizationEnabled = isEnabled;
}

Net::ConnectionSettings TcpClient::getConnectionSettingsActive() const
{
    Net::ConnectionSettings netSettings = getConnectionSettings();
    netSettings.ipLocal = m_pTcpSocket->localAddress();
    netSettings.portIn = m_pTcpSocket->localPort();
    return netSettings;
}

void TcpClient::printConnectionInfo() const
{
    if (m_connectionState != ConnectionState::Created)
    {
        f_logGeneral(QString("%1: connection is not created.").arg(nameId()));
        return;
    }
    QString msg;
    msg = QString("------------- Connection Info --------------\n"
                  "Connection type: %1\n"
                  "Connection ID: %2\n"
                  "Connection name: %3\n"
                  "Port In: %4\n"
                  "Port Out: %5\n"
                  "Local IP: %6\n"
                  "Destination IP: %7\n"
                  "--------------------------------------------")
          .arg(m_connectionId)
          .arg(m_connectionTypeName)
          .arg(m_pTcpSocket->localPort())
          .arg(m_pTcpSocket->peerPort())
          .arg(m_pTcpSocket->localAddress().toString())
          .arg(m_pTcpSocket->peerAddress().toString());
    f_logGeneral(msg);
    return;
}

void TcpClient::printError() const
{
    f_logError(QString("%1: errorOccured: %2 (code %3)").arg(nameId()).arg(m_pTcpSocket->errorString()).arg(m_pTcpSocket->error()));
    return;
}
