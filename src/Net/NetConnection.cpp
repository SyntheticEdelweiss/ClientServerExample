#include "NetConnection.hpp"

using namespace Net;

NetConnection::NetConnection(const quint8 _connType, const QString _connTypeName, QObject* parent)
    : QObject(parent)
    , m_connectionType(_connType)
    , m_connectionTypeName(_connTypeName)
{
    qRegisterMetaType<QHostAddress>("QHostAddress");
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<Net::ConnectionState>("Net::ConnectionState");
    qRegisterMetaType<Net::AddressPort>("Net::AddressPort");
    qRegisterMetaType<Net::ConnectionSettings>("Net::ConnectionSettings");
    qRegisterMetaType<Net::LoginData>("Net::LoginData");

    m_pCallbackThreadContextHelper = new QObject;

    QNetworkProxyFactory::setUseSystemConfiguration(false); // depending on system proxy settings, it may be impossible to bind to desired ip/port
    setObjectName(_connTypeName);

    connect(this, qOverload<>(&NetConnection::openConnectionQueued), this, qOverload<>(&NetConnection::openConnection), Qt::QueuedConnection);
    connect(this, qOverload<Net::ConnectionSettings const&>(&NetConnection::openConnectionQueued),
            this, qOverload<Net::ConnectionSettings const&>(&NetConnection::openConnection), Qt::QueuedConnection);
    connect(this, qOverload<>(&NetConnection::reopenConnectionQueued), this, qOverload<>(&NetConnection::reopenConnection), Qt::QueuedConnection);
    connect(this, qOverload<Net::ConnectionSettings const&>(&NetConnection::reopenConnectionQueued),
            this, qOverload<Net::ConnectionSettings const&>(&NetConnection::reopenConnection), Qt::QueuedConnection);
    connect(this, &NetConnection::closeConnectionQueued, this, &NetConnection::closeConnection, Qt::QueuedConnection);
    connect(this, &NetConnection::sendMessageQueued, this, &NetConnection::sendMessage, Qt::QueuedConnection);

    //f_logGeneral(QString("NetConnection: %1 constructed").arg(m_connectionTypeName));
}

NetConnection::~NetConnection()
{
    delete m_pCallbackThreadContextHelper;
    //f_logGeneral(QString("NetConnection: %1 %2 destroyed").arg(m_connectionTypeName).arg(QString::number(m_connectionId));
}

Net::ConnectionState NetConnection::openConnection()
{
    return openConnection(m_connectionSettings);
}

Net::ConnectionState NetConnection::reopenConnection()
{
    return reopenConnection(getConnectionSettings());
}

Net::ConnectionState NetConnection::reopenConnection(ConnectionSettings const& a_connectionSettings)
{
    closeConnection();
    setConnectionSettings(a_connectionSettings);
    return NetConnection::openConnection();
}

void NetConnection::setConnectionSettings(Net::ConnectionSettings const& a_connectionSettings)
{
    if (m_connectionState == Net::ConnectionState::Created)
    {
        f_logGeneral(QString("%1: called setConnectionSettings() while connection is open - action forbidden").arg(nameId()));
        return;
    }
    m_connectionSettings = a_connectionSettings;
}

void NetConnection::setConnectionId(uint new_id)
{
    if (m_connectionState == Net::ConnectionState::Created)
    {
        f_logGeneral(QString("%1: called setConnectionId() while connection is open - action forbidden").arg(nameId()));
        return;
    }
    m_connectionId = new_id;
}

void NetConnection::setCallbackFunction(std::function<void(QByteArray, NetConnection* const, Net::AddressPort)> a_onReceivedMessage)
{
    if (m_connectionState == Net::ConnectionState::Created)
    {
        f_logGeneral(QString("%1: called setCallbackFunction() while connection is open - action forbidden").arg(nameId()));
        return;
    }

    if (m_pCallbackThread == nullptr)
    {
        f_onReceivedMessage = a_onReceivedMessage;
    }
    else
    {
        // NOTE: for QT version below 5.10, QTimer::singleShot is used, and that will not work if source thread (from which <this> called) doesn't have working QEventLoop
        // See - https://stackoverflow.com/questions/21646467/how-to-execute-a-functor-or-a-lambda-in-a-given-thread-in-qt-gcd-style
        // and for bug specifically - https://bugreports.qt.io/browse/QTBUG-66458
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        auto f_callback = [ctx = m_pCallbackThreadContextHelper, a_onReceivedMessage](QByteArray msg, NetConnection* const netConnection, Net::AddressPort addressPort){
            QMetaObject::invokeMethod(ctx, [a_onReceivedMessage, msg, netConnection, addressPort](){ a_onReceivedMessage(msg, netConnection, addressPort); }, Qt::QueuedConnection);
        };
#else
        auto f_callback = [ctx = m_pCallbackThreadContextHelper, a_onReceivedMessage](QByteArray msg, NetConnection* const netConnection, Net::AddressPort addressPort){
            QTimer::singleShot(0, ctx, [a_onReceivedMessage, msg, netConnection, addressPort](){ a_onReceivedMessage(msg, netConnection, addressPort); });
        };
#endif
        f_onReceivedMessage = f_callback;
    }
}

void NetConnection::setLoggingFunctions(std::function<void(QString)> a_logGeneral, std::function<void(QString)> a_logError)
{
    if (m_connectionState == Net::ConnectionState::Created)
    {
        f_logGeneral(QString("%1: called setLoggingFunctions() while connection is open - action forbidden").arg(nameId()));
        return;
    }
    if (a_logGeneral)
        f_logGeneral = a_logGeneral;
    if (a_logError)
        f_logError = a_logError;
}

bool NetConnection::setIsCallbackDistinctThread(const bool a_isCallbackDistinctThread)
{
    if (m_connectionState == Net::ConnectionState::Created)
    {
        f_logGeneral(QString("%1: called setIsCallbackDistinctThread() while connection is open - action forbidden").arg(nameId()));
        return false;
    }

    if (a_isCallbackDistinctThread == true)
    {
        if (m_pCallbackThread == nullptr)
        {
            m_pCallbackThread = new QThread;
            // make main thread responsible for deletion of this thread, otherwise <this>'s thread will be deleted before deleting callback's thread, resulting in dangling active thread
            m_pCallbackThread->moveToThread(QCoreApplication::instance()->thread());
            QObject::connect(this, &QObject::destroyed, m_pCallbackThread, &QThread::quit);
            QObject::connect(m_pCallbackThread, &QThread::finished, m_pCallbackThread, &QThread::deleteLater);
            m_pCallbackThread->start();
            f_onReceivedMessage = {}; // empty function causing segfault is intended - you should always call setCallbackFunction after changing setIsCallbackDistinctThread

            if (QThread::currentThread() != m_pCallbackThreadContextHelper->thread())
            {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
                QMetaObject::invokeMethod(m_pCallbackThreadContextHelper, [this](){
                    m_pCallbackThreadContextHelper->moveToThread(m_pCallbackThread);
                }, Qt::QueuedConnection);
#else
                QTimer::singleShot(0, this, [this](){
                    m_pCallbackThreadContextHelper->moveToThread(m_pCallbackThread);
                });
#endif
            }
            else
            {
                m_pCallbackThreadContextHelper->moveToThread(m_pCallbackThread);
            }
            return true;
        }
    }
    else
    {
        if (m_pCallbackThread != nullptr)
        {
            if (QThread::currentThread() != m_pCallbackThreadContextHelper->thread())
            {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
                QMetaObject::invokeMethod(m_pCallbackThreadContextHelper, [this](){
                    m_pCallbackThreadContextHelper->moveToThread(this->thread());
                }, Qt::QueuedConnection);
#else
                QTimer::singleShot(0, this, [this](){
                    m_pCallbackThreadContextHelper->moveToThread(this->thread());
                });
#endif
            }
            else
            {
                m_pCallbackThreadContextHelper->moveToThread(this->thread());
            }
            m_pCallbackThread->exit(0);
            m_pCallbackThread = nullptr;
            f_onReceivedMessage = {}; // empty function causing segfault is intended - you should always call setCallbackFunction after changing setIsCallbackDistinctThread
            return true;
        }
    }
    return false;
}

void NetConnection::printConnectionSettings() const
{
    QString msg;
    msg = QString("------------ Connection Settings -----------\n"
                  "Connection type: %1\n"
                  "Connection ID: %2\n"
                  "Object name: %3\n"
                  "Port In: %4\n"
                  "Port Out: %5\n"
                  "Local IP: %6\n"
                  "Destination IP: %7\n"
                  "--------------------------------------------")
          .arg(m_connectionTypeName)
          .arg(m_connectionId)
          .arg(this->objectName())
          .arg(m_connectionSettings.portIn)
          .arg(m_connectionSettings.portOut)
          .arg(m_connectionSettings.ipLocal.toString())
          .arg(m_connectionSettings.ipDestination.toString());
    f_logGeneral(msg);
    return;
}
