#include "TcpServer.hpp"

#include <type_traits>

using namespace Net;
using namespace std;

TcpServer::TcpServer(const quint8 _connType, const QString _connTypeName, QObject* parent)
    : NetConnection(_connType, _connTypeName, parent)
    , m_pServer(new QTcpServer(this))
{
    connect(this, qOverload<QByteArray, QHostAddress, quint16>(&TcpServer::sendMessageToQueued), this, qOverload<QByteArray, QHostAddress, quint16>(&TcpServer::sendMessageTo), Qt::QueuedConnection);
    connect(this, qOverload<QByteArray, Net::AddressPort>(&TcpServer::sendMessageToQueued), this, qOverload<QByteArray, Net::AddressPort>(&TcpServer::sendMessageTo), Qt::QueuedConnection);
    connect(this, qOverload<QByteArray, QHostAddress>(&TcpServer::sendMessageToQueued), this, qOverload<QByteArray, QHostAddress>(&TcpServer::sendMessageTo), Qt::QueuedConnection);
    connect(this, &TcpServer::addAllowedAddressQueued, this, &TcpServer::addAllowedAddress, Qt::QueuedConnection);
    connect(this, &TcpServer::removeAllowedAddressQueued, this, &TcpServer::removeAllowedAddress, Qt::QueuedConnection);
    connect(m_pServer, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

TcpServer::~TcpServer()
{
    TcpServer::closeConnection();
    delete m_pServer;
}

Net::ConnectionState TcpServer::openConnection(ConnectionSettings const& a_connectionSettings)
{
    if (m_connectionState == ConnectionState::Created)
        return m_connectionState;
    m_connectionSettings = a_connectionSettings;
    bool isListenOk = m_pServer->listen(m_connectionSettings.ipLocal, m_connectionSettings.portIn);
    if (isListenOk == false)
    {
        m_connectionState = ConnectionState::NotCreated;
        f_logError(QString("%1: Unable to open connection - %2").arg(nameId()).arg(m_pServer->errorString()));
        emit openedConnection(false);
        return m_connectionState;
    }
    m_connectionState = ConnectionState::Created;
    f_logGeneral(QString("%1: Opened connection").arg(nameId()));
    printConnectionInfo();
    connect(m_pServer, &QTcpServer::acceptError, this, &TcpServer::printError);
    emit openedConnection(true);
    return m_connectionState;
}

void TcpServer::closeConnection()
{
    m_pServer->close();
    if (m_connectionState == ConnectionState::Created)
        f_logGeneral(QString("%1: Closed connection").arg(nameId()));
    m_connectionState = ConnectionState::NotCreated;
    disconnect(m_pServer, &QTcpServer::acceptError, this, &TcpServer::printError);

    while (!m_clientMap.empty()) // onSocketDisconnected() deletes socket right away after close()
    {
        auto socket = m_clientMap.begin().key();
        socket->close();
    }
    emit closedConnection();
}

void TcpServer::addAllowedAddress(QHostAddress addr)
{
    m_allowedAddresses.insert(addr);
}

void TcpServer::removeAllowedAddress(QHostAddress addr)
{
    m_allowedAddresses.remove(addr);
    QList<QTcpSocket*> toClose;
    for (auto iter = m_clientMap.begin(); iter != m_clientMap.end(); ++iter)
    {
        if (iter.value()->peerAddrPort.addr == addr)
            toClose.append(iter.key());
    }
    for (QTcpSocket* pSocket : toClose)
        pSocket->close();
}

void TcpServer::onNewConnection()
{
    while (m_pServer->hasPendingConnections())
    {
        QTcpSocket* pSocket = m_pServer->nextPendingConnection();
        if (m_isAllowAllAdresses == false)
        {
            if (m_allowedAddresses.contains(pSocket->peerAddress()) == false)
            {
                f_logGeneral(QString("%1: rejected client %3:%4 - client not in allowed list")
                             .arg(nameId())
                             .arg(pSocket->peerAddress().toString())
                             .arg(pSocket->peerPort()));
                pSocket->abort();
                pSocket->deleteLater();
                continue;
            }
        }

        {
        }

        ClientData* d = new ClientData;
        d->pSocket = pSocket;
        d->peerAddrPort = Net::AddressPort{pSocket->peerAddress(), pSocket->peerPort()};
        d->localAddrPort = Net::AddressPort{pSocket->localAddress(), pSocket->localPort()};

        m_clientMap.insert(pSocket, std::shared_ptr<std::remove_pointer_t<decltype(d)>>(d));
        m_clientByPeerAddressPort.insert({pSocket->peerAddress(), pSocket->peerPort()}, d);
        m_clientsByPeerAddress[pSocket->peerAddress()].insert(pSocket->peerPort(), d);
        connect(pSocket, &QTcpSocket::readyRead, this, &TcpServer::readReceived);
        connect(pSocket, &QTcpSocket::disconnected, this, &TcpServer::onSocketDisconnected);
        connect(pSocket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this, &TcpServer::printSocketError);
        f_logGeneral(QString("%1: client %2:%3 (local %4:%5) sockd:%6 connected")
                     .arg(nameId())
                     .arg(pSocket->peerAddress().toString())
                     .arg(pSocket->peerPort())
                     .arg(pSocket->localAddress().toString())
                     .arg(pSocket->localPort())
                     .arg(pSocket->socketDescriptor()));
        emit clientConnected({pSocket->peerAddress(), pSocket->peerPort()});
    }
    return;
}

qint64 TcpServer::sendMessage(const QByteArray& msg)
{
    qint64 ret = 0;
    for (auto clientIter = m_clientMap.begin(); clientIter != m_clientMap.end(); ++clientIter)
        ret += sendMessageTo(msg, clientIter.key());
    return ret;
}

// No validity check for pClientSocket since this method is private and all its calls are guaranteed to be safe
qint64 TcpServer::sendMessageTo(QByteArray msg, QTcpSocket* pClientSocket)
{
    qint64 bytesWritten = -1;
    const decltype(Net::PendingMessage::pendingSize) msgSize = static_cast<decltype(msgSize)>(msg.size());
    QByteArray header(reinterpret_cast<const char*>(&msgSize), sizeof(msgSize));
    bytesWritten = pClientSocket->write(header + msg);
    pClientSocket->waitForBytesWritten(1000);
    if (bytesWritten > 0)
        emit writeDone((bytesWritten == msg.size()) ? msg : msg.mid(0, bytesWritten));
    return bytesWritten;
}

qint64 TcpServer::sendMessageTo(QByteArray msg, QHostAddress address, quint16 port)
{
    return sendMessageTo(msg, Net::AddressPort{address, port});
}

qint64 TcpServer::sendMessageTo(QByteArray msg, Net::AddressPort addressPort)
{
    auto iter = m_clientByPeerAddressPort.find(addressPort);
    if (iter == m_clientByPeerAddressPort.end())
    {
        f_logError(QString("%1: can't send message to unconnected host %2:%3.")
                     .arg(nameId())
                     .arg(addressPort.addr.toString())
                     .arg(addressPort.port));
        return -1;
    }
    return sendMessageTo(msg, iter.value()->pSocket);
}

qint64 TcpServer::sendMessageTo(QByteArray msg, QHostAddress address)
{
    qint64 ret = 0;
    auto iterByAddr = m_clientsByPeerAddress.constFind(address);
    if (iterByAddr == m_clientsByPeerAddress.constEnd())
    {
        f_logError(QString("%1: can't send message to address %2 with no connections to it.")
                     .arg(nameId())
                     .arg(address.toString()));
        return -1;
    }
    auto clientsAtAddress = iterByAddr.value();
    for (auto iterByPort = clientsAtAddress.begin(); iterByPort != clientsAtAddress.end(); ++iterByPort)
        ret += sendMessageTo(msg, iterByPort.value()->peerAddrPort);
    return ret;
}

void TcpServer::onSocketDisconnected()
{
    QTcpSocket* pSocket = qobject_cast<QTcpSocket*>(sender());
    auto iterClient = m_clientMap.find(pSocket);
    const ClientData& d = *(iterClient.value());
    emit clientDisconnected(d.peerAddrPort);
    f_logGeneral(QString("%1: client %2:%3 (local %4:%5) disconnected")
                 .arg(nameId())
                 .arg(d.peerAddrPort.addr.toString())
                 .arg(d.peerAddrPort.port)
                 .arg(d.localAddrPort.addr.toString())
                 .arg(d.localAddrPort.port));
    m_clientByPeerAddressPort.remove(d.peerAddrPort);
    auto iterByAddr = m_clientsByPeerAddress.find(d.peerAddrPort.addr);
    iterByAddr.value().remove(d.peerAddrPort.port);
    if (iterByAddr.value().isEmpty())
        m_clientsByPeerAddress.erase(iterByAddr);
    m_clientMap.erase(iterClient);
    m_pendingMsgBySocket.remove(pSocket);
    pSocket->deleteLater();
}

void TcpServer::readReceived()
{
    QTcpSocket* pSocket = qobject_cast<QTcpSocket*>(sender());
    Net::PendingMessage& pendingMsg = m_pendingMsgBySocket[pSocket];
    MAKE_QDATASTREAM_NET_D(streamSocket, pSocket);
    while (pSocket->bytesAvailable() > 0)
    {
        if (pendingMsg.pendingSize == 0)
        {
            if (pSocket->bytesAvailable() < m_headerSize)
                break;
            streamSocket >> pendingMsg.pendingSize;
            pendingMsg.msg.resize(pendingMsg.pendingSize);
            pendingMsg.curPos = 0;
        }
        const decltype(Net::PendingMessage::pendingSize) availableSize = static_cast<decltype(availableSize)>(pSocket->bytesAvailable());
        if (availableSize < pendingMsg.pendingSize)
        {
            streamSocket.readRawData((pendingMsg.msg.data() + pendingMsg.curPos), availableSize);
            emit readPartialDone(pendingMsg.msg.mid(pendingMsg.curPos, availableSize));
            pendingMsg.curPos += availableSize;
            pendingMsg.pendingSize -= availableSize;
            break;
        }
        streamSocket.readRawData((pendingMsg.msg.data() + pendingMsg.curPos), pendingMsg.pendingSize);
        emit readPartialDone(pendingMsg.msg.mid(pendingMsg.curPos, pendingMsg.pendingSize));
        emit readDone(pendingMsg.msg);
        pendingMsg.curPos = 0;
        pendingMsg.pendingSize = 0;
        f_onReceivedMessage(pendingMsg.msg, this, {pSocket->peerAddress(), pSocket->peerPort()});
    }
    return;
}

Net::ConnectionSettings TcpServer::getConnectionSettingsActive() const
{
    Net::ConnectionSettings netSettings = getConnectionSettings();
    netSettings.ipLocal = m_pServer->serverAddress();
    netSettings.portIn = m_pServer->serverPort();
    return netSettings;
}

bool TcpServer::getIsClientConnected(const Net::AddressPort addrPort)
{
    auto iter = m_clientByPeerAddressPort.constFind(addrPort);
    if (iter == m_clientByPeerAddressPort.constEnd())
        return false;
    return true;
}

void TcpServer::printConnectionInfo() const
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
                  "Object name: %3\n"
                  "Port In: %4\n"
                  "Local IP: %5\n"
                  "--------------------------------------------")
          .arg(m_connectionTypeName)
          .arg(m_connectionId)
          .arg(objectName())
          .arg(m_pServer->serverPort())
          .arg(m_pServer->serverAddress().toString());
    f_logGeneral(msg);
    return;
}

void TcpServer::printError() const
{
    f_logError(QString("%1: errorOccured: %2 (code %3)").arg(nameId()).arg(m_pServer->errorString()).arg(m_pServer->serverError()));
    return;
}

void TcpServer::printSocketError() const
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(QObject::sender());    
    auto iterClient = m_clientMap.find(clientSocket);
    const ClientData& d = *(iterClient.value());
    QAbstractSocket::SocketError err = clientSocket->error();
    if (err == QAbstractSocket::RemoteHostClosedError)
    {
        f_logGeneral(QString("%1: client %2:%3 (local %4:%5) sockd:%6: %7 (code %8)")
                     .arg(nameId())
                     .arg(d.peerAddrPort.addr.toString())
                     .arg(d.peerAddrPort.port)
                     .arg(d.localAddrPort.addr.toString())
                     .arg(d.localAddrPort.port)
                     .arg(clientSocket->socketDescriptor())
                     .arg(clientSocket->errorString())
                     .arg(clientSocket->error()));
    }
    else
    {
        f_logError(QString("%1: client %2:%3 (local %4:%5) sockd:%6: errorOccured: %7 (code %8)")
                   .arg(nameId())
                   .arg(d.peerAddrPort.addr.toString())
                   .arg(d.peerAddrPort.port)
                   .arg(d.localAddrPort.addr.toString())
                   .arg(d.localAddrPort.port)
                   .arg(clientSocket->socketDescriptor())
                   .arg(clientSocket->errorString())
                   .arg(clientSocket->error()));
    }
    return;
}
