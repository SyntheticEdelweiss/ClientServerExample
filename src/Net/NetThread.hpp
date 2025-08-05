#pragma once

#include <QtCore/QCoreApplication>
#include <QtCore/QSet>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include "TcpClient.hpp"
#include "TcpServer.hpp"

namespace Net
{
template<typename ConnectionClass,
         std::enable_if_t<std::is_base_of_v<NetConnection, ConnectionClass>, bool> = true>
NetConnection* makeConnectionInstance()
{
    NetConnection* ptr = new ConnectionClass;
    return ptr;
}
}

class NetThread;
class NetThreadContextHelper;

/* QThread itself is affiliated with thread that instantiated it, so any slots connected to NetThread via QueuedConnection will be is executed in that parent thread.
 * Viable workaround is to use ContextHelper - simple QObject - which is moved to NetThread and therefore affiliated with it.
 * Making signal/slot connection with slot as NetThread member function would then require extra lambda, e.g. connect(this, signal, ContextHelper, [this](){ this->slot() });
 * To avoid such extra lambdas QObject-derived class can be made with required slots. NetThreadContextHelper is such QObject-derived class, implements slots originaly intended to be in NetThread.
 * NetThread doesn't call its signals/slots too often, so extra lambdas don't incur noteworthy performance penalty, thus it's rather a matter of extra code for each QObject::connect. */
class NetThread : public QThread
{
    Q_OBJECT
    friend class NetThreadContextHelper;
public:
    virtual ~NetThread();
    NetThread(QObject* parent = nullptr);

private:
    NetThreadContextHelper* m_contextHelper = nullptr;
    QSet<NetConnection*> m_netConnectionsSet;
    bool m_isQuitWhenNoConnections = true; // if true, NetThread will quit after its last NetConnection is destroyed

public:
    inline void setQuitWhenNoConnections(bool isQuit) { m_isQuitWhenNoConnections = isQuit; }
    inline bool getQuitWhenNoConnections() const { return m_isQuitWhenNoConnections; }
    inline int getConnectionsCount() const { return m_netConnectionsSet.count(); }
    inline NetThreadContextHelper* contextHelper() const { return m_contextHelper; }

signals:
    void finishedMakeNetConnection(NetConnection* pManager);
    void makeNetConnectionQueued(std::function<NetConnection*(void)>);
};

class NetThreadContextHelper : public QObject
{
    Q_OBJECT
    friend class NetThread;
private:
    explicit NetThreadContextHelper(NetThread* contextThread);

    NetThread* m_netThread = nullptr;

private slots:
    void onMakeNetConnection(std::function<NetConnection*(void)> f_makeNetConnection);
    void onNetConnectionDestroyed(QObject* pNetConnection);
};

namespace Net
{
// NOTE: do not call this function simultaneously from different threads with the same NetThread arg as it relies on NetThread::created signal, so async calls may lead to different signals getting mixed ->
// this function may wait for incorrect time and returned pointer may point to incorrect NetConnection
// NOTE: for QT version below 5.10, QTimer::singleShot is used, and that will not work if source thread (from which <this> called) doesn't have working QEventLoop
// See - https://stackoverflow.com/questions/21646467/how-to-execute-a-functor-or-a-lambda-in-a-given-thread-in-qt-gcd-style
// and for bug specifically - https://bugreports.qt.io/browse/QTBUG-66458
// Instantiate NetConnection in NetThread provided as arg or in a newly created NetThread if none is provided, and wait untill instantiation is finished
template<typename ConnectionClass>
std::tuple<ConnectionClass*, NetThread*> instantiateWaitThreadedConnection(NetThread* pThread = nullptr)
{
    if (pThread == nullptr)
    {
        pThread = new NetThread;
        pThread->start();
    }

    auto* contextHelper = pThread->contextHelper();

    ConnectionClass* pConnection = nullptr;
    QThread* pCurThread = QThread::currentThread();
    auto connectionHandle = QObject::connect(pThread, &NetThread::finishedMakeNetConnection, contextHelper, [&pConnection](NetConnection* a_pConnection) {
        pConnection = static_cast<ConnectionClass*>(a_pConnection); // safe to cast because NetConnectionThreadWorker is guaranteed to make instance of actual type and simply returns base class pointer
    });
    emit pThread->makeNetConnectionQueued(Net::makeConnectionInstance<ConnectionClass>);
    while (true)
    {
        if (pConnection != nullptr)
            break;
        pCurThread->msleep(5);
    }
    QObject::disconnect(connectionHandle);

    return std::make_tuple(pConnection, pThread);
}
void reopenWaitThreadedConnection(NetConnection* pConnection, Net::ConnectionSettings netSettings);
void openWaitThreadedConnection(NetConnection* pConnection, Net::ConnectionSettings netSettings);
void closeWaitThreadedConnection(NetConnection* pConnection);
void destroyWaitThreadedConnection(NetConnection* pConnection);
}
