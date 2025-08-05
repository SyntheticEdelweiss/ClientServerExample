#include "NetThread.hpp"

#include <atomic>

NetThread::~NetThread()
{
    while (!m_netConnectionsSet.isEmpty())
        Net::destroyWaitThreadedConnection(*(m_netConnectionsSet.begin()));
}

NetThread::NetThread(QObject* parent)
    : QThread(parent)
{
    qRegisterMetaType<std::function<NetConnection*(void)>>("std::function<NetConnection*(void)>");
    // Calling moveToThread(targetThread) is valid even when such thread hasn't started yet
    m_contextHelper = new NetThreadContextHelper(this);
}

NetThreadContextHelper::NetThreadContextHelper(NetThread* contextThread)
{
    m_netThread = contextThread;
    this->moveToThread(m_netThread);
    connect(m_netThread, &NetThread::makeNetConnectionQueued, this, &NetThreadContextHelper::onMakeNetConnection, Qt::QueuedConnection);
    connect(m_netThread, &QThread::finished, this, &QObject::deleteLater);
    connect(m_netThread, &QThread::finished, m_netThread, &QThread::deleteLater);
}

void NetThreadContextHelper::onMakeNetConnection(std::function<NetConnection*(void)> f_makeNetConnection)
{
    NetConnection* pConnection = f_makeNetConnection();
    connect(pConnection, &QObject::destroyed, this, &NetThreadContextHelper::onNetConnectionDestroyed);
    m_netThread->m_netConnectionsSet.insert(pConnection);
    emit m_netThread->finishedMakeNetConnection(pConnection);
}

void NetThreadContextHelper::onNetConnectionDestroyed(QObject* pNetConnection)
{
    m_netThread->m_netConnectionsSet.remove(static_cast<NetConnection*>(pNetConnection));
    if (m_netThread->m_netConnectionsSet.isEmpty() && m_netThread->m_isQuitWhenNoConnections)
        m_netThread->quit();
}

namespace Net
{

void reopenWaitThreadedConnection(NetConnection* pConnection, Net::ConnectionSettings netSettings)
{
    if (pConnection == nullptr)
        return;
    QThread* pCurThread = QThread::currentThread();
    if (pCurThread == pConnection->thread())
    {
        pConnection->reopenConnection(netSettings);
        return;
    }
    std::atomic<bool> isDone(false);
    auto tempConnectHandler = QObject::connect(pConnection, &NetConnection::openedConnection, pConnection, [&isDone]() { isDone.store(true, std::memory_order_relaxed); });
    emit pConnection->reopenConnectionQueued(netSettings);
    while (true)
    {
        if (isDone.load(std::memory_order_relaxed))
            break;
        pCurThread->msleep(5);
    }
    QObject::disconnect(tempConnectHandler);
    return;
}

void openWaitThreadedConnection(NetConnection* pConnection, Net::ConnectionSettings netSettings)
{
    if (pConnection == nullptr)
        return;
    QThread* pCurThread = QThread::currentThread();
    if (pCurThread == pConnection->thread())
    {
        pConnection->openConnection(netSettings);
        return;
    }
    std::atomic<bool> isDone(false);
    auto tempConnectHandler = QObject::connect(pConnection, &NetConnection::openedConnection, pConnection, [&isDone]() { isDone.store(true, std::memory_order_relaxed); });
    emit pConnection->openConnectionQueued(netSettings);
    while (true)
    {
        if (isDone.load(std::memory_order_relaxed))
            break;
        pCurThread->msleep(5);
    }
    QObject::disconnect(tempConnectHandler);
    return;
}

void closeWaitThreadedConnection(NetConnection* pConnection)
{
    if (pConnection == nullptr)
        return;
    QThread* pCurThread = QThread::currentThread();
    if (pCurThread == pConnection->thread())
    {
        pConnection->closeConnection();
        return;
    }
    std::atomic<bool> isDone(false);
    auto tempConnectHandler = QObject::connect(pConnection, &NetConnection::closedConnection, pConnection, [&isDone]() { isDone.store(true, std::memory_order_relaxed); });
    emit pConnection->closeConnectionQueued();
    while (true)
    {
        if (isDone.load(std::memory_order_relaxed))
            break;
        pCurThread->msleep(5);
    }
    QObject::disconnect(tempConnectHandler);
    return;
}

void destroyWaitThreadedConnection(NetConnection* pConnection)
{
    if (pConnection == nullptr)
        return;
    QThread* pCurThread = QThread::currentThread();
    if (pCurThread == pConnection->thread())
    {
        delete pConnection;
        return;
    }
    std::atomic<bool> isDone(false);
    auto tempConnectHandler = QObject::connect(pConnection, &NetConnection::destroyed, pConnection, [&isDone]() { isDone.store(true, std::memory_order_relaxed); });
    pConnection->deleteLater();
    while (true)
    {
        if (isDone.load(std::memory_order_relaxed))
            break;
        pCurThread->msleep(5);
    }
    QObject::disconnect(tempConnectHandler);
    return;
}

}
