#pragma once

#include <algorithm>
#include <functional>
#include <memory>

#include <QtCore/QCache>
#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QVector>

#include "Common/Protocol.hpp"
#include "Common/Utils.hpp"
#include "Net/TcpServer.hpp"


struct Task
{
    std::unique_ptr<Protocol::Request> request;
    std::unique_ptr<QFutureWatcherBase> futureWatcher;
    // not part of task, but convenient to keep it here
    Net::AddressPort addrPort;
    quint64 rmsgHash{0};
};

/* pre qt6 QtConcurrent::map(), etc. don't work with non-global QThreadPool -> need to emulate similar behavior by
 * running multiple QtConcurrent::run() with whatever needed for progress updates, cancelation, etc. */
template<typename ResultT>
struct Task5
{
    std::unique_ptr<Protocol::Request> request;

    QVector<ResultT> subresults; // resize before executing task -> threads write by predefined index lock-free
    std::atomic<int> remainingSubtaskCount{0};
    std::atomic<int> progressValue{0};
    std::atomic<bool> isCanceled{false};

    // not part of task, but convenient to keep it here
    Net::AddressPort addrPort;
    quint64 rmsgHash{0};
};

class ExampleServer : public QObject
{
    Q_OBJECT
public:
    explicit ExampleServer(Net::ConnectionSettings serverSettings, QObject* parent = nullptr);

private:
    TcpServer* m_server;

    QHash<Net::AddressPort, std::shared_ptr<Task>> m_taskMap;
    QHash<Net::AddressPort, QThreadPool*> m_threadPoolMap;

    QCache<quint64, Protocol::Request> m_cache;

    const QString m_dtFormat{QStringLiteral("[yyyy.MM.dd-hh:mm:ss.zzz]")};
    uint m_regId_general = 0;
    std::function<void(QString)> f_logGeneral = [](QString msg) { qInfo(qUtf8Printable(QDateTime::currentDateTimeUtc().toString(QStringLiteral("[yyyy.MM.dd-hh:mm:ss.zzz]")) + msg)); };
    std::function<void(QString)> f_logError = [](QString msg) { qWarning(qUtf8Printable(QDateTime::currentDateTimeUtc().toString(QStringLiteral("[yyyy.MM.dd-hh:mm:ss.zzz]")) + msg)); };

    const int m_maxChunkCount = 100;
    const int m_minChunkSize = 100;

private:
    void loadSettings();

    void sendRequestToClient(const Protocol::Request* req, Net::AddressPort addrPort);
    void sendErrorToClient(Protocol::ErrorCode errorCode, Net::AddressPort addrPort, QString errorText = QString{});
    void parseRequest(QByteArray msg, NetConnection* const, Net::AddressPort addrPort);
    void onCorruptedMessage(QByteArray msg, Net::AddressPort addrPort, QString errorText = QString{});

    static QVector<int> sortArray(QVector<int> arr);
    static void sortArray_reduce(QVector<int>& aggregate, const QVector<int>& part) {
        aggregate.reserve(aggregate.size() + part.size());
        auto idxMiddle = aggregate.size();
        aggregate.append(part);
        std::inplace_merge(aggregate.begin(), (aggregate.begin() + idxMiddle), aggregate.end());
    }

    static QVector<int> findPrimeNumbers(int numFrom, int numTo);
    static QVector<int> findPrimeNumbersT(std::tuple<int, int> args) { return std::apply(&ExampleServer::findPrimeNumbers, args); }
    static void findPrimeNumbers_reduce(QVector<int>& aggregate, const QVector<int>& part) { aggregate.append(part); }

    static QVector<QPoint> calculateFunction(Protocol::EquationType equationType, int x_from, int x_to, int x_step, const int constantA, const int constantB, const int constantC);
    static QVector<QPoint> calculateFunctionT(std::tuple<Protocol::EquationType, int, int, int, const int, const int, const int> args) { return std::apply(&ExampleServer::calculateFunction, args); }
    static void calculateFunction_reduce(QVector<QPoint>& aggregate, const QVector<QPoint>& part) { aggregate.append(part); }

private slots:
    void onClientConnected(Net::AddressPort addrPort);
    void onClientDisconnected(Net::AddressPort addrPort);
};


// This whole thing is meant as balance between safety and convenience, mapping RequestType to QFutureWatcher<T> of corresponding type T
// Usage: RFWMapper_t<RequestType> -> corresponding QFutureWatcher type; watcher_cast<RequestType>(fw) -> QFutureWatcher pointer of actual type
// Example: auto futureWatcher = make_shared<RFWMapper_t<RequestType::SortArray>>(); -> auto futureWatcher = make_shared<QFutureWatcher<QVector<int>>>();
//      auto fw = watcher_cast<ReqT>(futureWatcher); -> auto = QFutureWatcher<QVector<int>>*, when (ReqT == RequestType::SortArray), futureWatcher is QFutureWatcherBase*
template<Protocol::RequestType R>
struct RFWMapper;

#define XX(EnumVal, ResultT)                         \
    template<>                                       \
    struct RFWMapper<Protocol::RequestType::EnumVal> \
    {                                                \
        using WatcherT = QFutureWatcher<ResultT>;    \
    };

#define MAPPER_TYPE_LIST(XX)        \
    XX(SortArray, QVector<int>)        \
    XX(FindPrimeNumbers, QVector<int>) \
    XX(CalculateFunction, QVector<QPoint>)

MAPPER_TYPE_LIST(XX)
#undef XX
#undef MAPPER_TYPE_LIST

template<Protocol::RequestType R>
using RFWMapper_t = typename RFWMapper<R>::WatcherT;

template<Protocol::RequestType R>
RFWMapper_t<R>* watcher_cast(QFutureWatcherBase* base)
{
    // auto w = dynamic_cast<RFWMapper_t<R>*>(base); // code is tested enough to use static_cast
    auto w = static_cast<RFWMapper_t<R>*>(base); // qobject_cast is impossible since QFutureWatcher specializations do not declare Q_OBJECT
    Q_ASSERT(w);
    return w;
}

template<Protocol::RequestType R>
std::shared_ptr<RFWMapper_t<R>> watcher_cast(std::shared_ptr<QFutureWatcherBase> base)
{
    // auto w = std::dynamic_pointer_cast<RFWMapper_t<R>>(base); // code is tested enough to use static_cast
    auto w = std::static_pointer_cast<RFWMapper_t<R>>(base);
    Q_ASSERT(w.get());
    return w;
}

static_assert(std::is_same_v<RFWMapper_t<Protocol::RequestType::SortArray>, QFutureWatcher<QVector<int>>>, "sanity check");


// Same as above, but mapping RequestType -> struct Request_*
template<Protocol::RequestType R>
struct RStMapper;

#define XX(EnumVal)                                  \
    template<>                                       \
    struct RStMapper<Protocol::RequestType::EnumVal> \
    {                                                \
        using StructT = Protocol::Request_##EnumVal; \
    };

#define MAPPER_TYPE_LIST(XX) \
    XX(InvalidRequest)       \
    XX(SortArray)            \
    XX(FindPrimeNumbers)     \
    XX(CalculateFunction)    \
    XX(CancelCurrentTask)    \
    XX(ProgressRange)        \
    XX(ProgressValue)

MAPPER_TYPE_LIST(XX)
#undef XX
#undef MAPPER_TYPE_LIST

template<Protocol::RequestType R>
using RStMapper_t = typename RStMapper<R>::StructT;

template<Protocol::RequestType R>
RStMapper_t<R>* request_cast(Protocol::Request* base)
{
    // auto w = dynamic_cast<RStMapper_t<R>*>(base); // code is tested enough to use static_cast
    auto w = static_cast<RStMapper_t<R>*>(base);
    Q_ASSERT(w);
    return w;
}

template<Protocol::RequestType R>
std::shared_ptr<RStMapper_t<R>> request_cast(std::shared_ptr<Protocol::Request> base)
{
    // auto w = std::dynamic_pointer_cast<RStMapper_t<R>>(base); // code is tested enough to use static_cast
    auto w = std::static_pointer_cast<RStMapper_t<R>>(base);
    Q_ASSERT(w.get());
    return w;
}
