#include "ExampleServer.hpp"

#include <cmath>
#include <functional>

#include <QtConcurrent>

#include "Common/Protocol.hpp"
#include "Common/RegLogger.hpp"
#include "Common/Utils.hpp"
#include "Net/NetHeaders.hpp"

using namespace std;
using namespace Protocol;
using namespace Net;

ExampleServer::ExampleServer(Net::ConnectionSettings serverSettings, QObject* parent)
    : QObject{parent}
{
    RegLoggerThreadWorker::instantiateRegLogger(qApp->applicationDirPath());
    m_regId_general = RegLogger::instance().addFile("log_server");

    auto log_lambda = [this](QString msg){
        RegLogger::instance().logData(this->m_regId_general, msg);
        qWarning(qUtf8Printable(QDateTime::currentDateTimeUtc().toString(m_dtFormat) + msg));
    };
    f_logGeneral = log_lambda;
    f_logError = f_logGeneral;

    // Using byteSize of Request as cost
    m_cache.setMaxCost(std::numeric_limits<int>::max());

    using namespace std::placeholders;
    m_server = std::get<0>(Net::instantiateWaitThreadedConnection<TcpServer>());
    m_server->setAllowAllAddresses(true);
    // since m_server works in distinct thread, parseRequest() should work in ExampleServer's thread, since this thread is not occupied with any other work
    // m_server->setCallbackFunction(std::bind(&ExampleServer::parseRequest, this, _1, _2, _3));
    m_server->setCallbackFunction([this](QByteArray arg1, NetConnection* const arg2, Net::AddressPort arg3) {
        QMetaObject::invokeMethod(this, [=](){
            parseRequest(arg1, arg2, arg3);
        }, Qt::QueuedConnection);
    });
    m_server->setLoggingFunctions(f_logGeneral, f_logError);

    m_server->setAuthorizationEnabled(true);
    m_server->addLoginDataQueued(Net::LoginData{QStringLiteral("Chuck"), QStringLiteral("Norris")});

    QObject::connect(m_server, &TcpServer::clientConnected, this, &ExampleServer::onClientConnected);
    QObject::connect(m_server, &TcpServer::clientDisconnected, this, &ExampleServer::onClientDisconnected);

    Net::openWaitThreadedConnection(m_server, serverSettings);
}

void ExampleServer::sendRequestToClient(const Protocol::Request* req, Net::AddressPort addrPort)
{
    QByteArray resMsg;
    MAKE_QDATASTREAM_NET(streamRes, &resMsg, QIODevice::WriteOnly);
    streamRes << *req;
    m_server->sendMessageToQueued(resMsg, addrPort);
}

void ExampleServer::sendErrorToClient(Protocol::ErrorCode errorCode, Net::AddressPort addrPort, QString errorText)
{
    Request_InvalidRequest req;
    req.errorCode = errorCode;
    req.errorText = errorText;
    sendRequestToClient(&req, addrPort);
}

void ExampleServer::parseRequest(QByteArray msg, NetConnection* const, Net::AddressPort addrPort)
{
    auto lambda_makeConnects = [this](Task* task, QFutureWatcherBase* fw){
        QObject::connect(fw, &QFutureWatcherBase::started, this, [this, task](){
            f_logGeneral(QStringLiteral("Started task %1 for %2").arg(toQString(task->request->type)).arg(toQString(task->addrPort)));
        });
        QObject::connect(fw, &QFutureWatcherBase::finished, this, [this, task](){
            f_logGeneral(QStringLiteral("Finished task %1 for %2").arg(toQString(task->request->type)).arg(toQString(task->addrPort)));
            if (task->futureWatcher->isCanceled())
            {
                Request_CancelCurrentTask req;
                sendRequestToClient(&req, task->addrPort); // canceled() is emitted before finished() -> still safe to access task here
            }
            else
            {
                // should be safe to release it at that point, since task is about to be deleted anyway
                Request* r = task->request.release();
                m_cache.insert(task->rmsgHash, r, r->byteSize());
            }
            m_taskMap.remove(task->addrPort);
        });
        QObject::connect(fw, &QFutureWatcherBase::progressRangeChanged, [this, task](int minimum, int maximum) {
            Request_ProgressRange req;
            req.minimum = minimum;
            req.maximum = maximum;
            sendRequestToClient(&req, task->addrPort);
        });
        QObject::connect(fw, &QFutureWatcherBase::progressValueChanged, [this, task](int progressValue) {
            Request_ProgressValue req;
            req.value = progressValue;
            sendRequestToClient(&req, task->addrPort);
        });
    };

    Request request;
    MAKE_QDATASTREAM_NET(streamMsg, &msg, QIODevice::ReadOnly);
    streamMsg >> request;
    if (streamMsg.status() != QDataStream::Ok)
    {
        onCorruptedRequest(msg, addrPort);
        return;
    }

    quint64 msgHash;
    // check cached requests first
    if (request.type != RequestType::CancelCurrentTask)
    {
        // easiest way is to hash QByteArray
        msgHash = hash64_FNV1a(msg);
        auto* req = m_cache[msgHash];
        // since requests store incoming data too, might as well do extra checks?
        if (req != nullptr)
        {
            f_logGeneral(QStringLiteral("Fetched cached result for task %1 for %2").arg(toQString(req->type)).arg(toQString(addrPort)));
            sendRequestToClient(req, addrPort);
            return;
        }
    }

    switch (request.type)
    {
    case RequestType::SortArray:
    {
        if (m_taskMap.contains(addrPort))
        {
            sendErrorToClient(Protocol::ErrorCode::AlreadyRunningTask, addrPort);
            return;
        }
        constexpr RequestType ReqT = RequestType::SortArray;
        auto req = make_unique<RStMapper_t<ReqT>>();
        streamMsg.device()->seek(0);
        streamMsg >> *req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedRequest(msg, addrPort);
            return;
        }

        auto sequence = divideIntoChunks(req->numbers, m_maxChunkCount, m_minChunkSize);

        auto iter = m_taskMap.insert(addrPort, make_shared<Task>());
        Task* task = iter.value().get();
        task->request = std::move(req);
        task->futureWatcher = make_unique<RFWMapper_t<ReqT>>();
        task->addrPort = addrPort;
        task->rmsgHash = msgHash;
        auto* fw = watcher_cast<ReqT>(task->futureWatcher.get());
        QObject::connect(fw, &QFutureWatcherBase::finished, this, [this, task]() {
            constexpr RequestType ReqT = RequestType::SortArray;
            auto req = request_cast<ReqT>(task->request.get());
            auto fw = watcher_cast<ReqT>(task->futureWatcher.get());
            if (fw->isCanceled()) // don't send anything if task was canceled
                return;

            req->numbers.resize(0); // capacity == sum of results' sizes
            int totalSize = 0;
            for (auto& result : fw->future())
            {
                auto idxMiddle = req->numbers.size();
                req->numbers.append(result);
                std::inplace_merge(req->numbers.begin(), (req->numbers.begin() + idxMiddle), req->numbers.end());
            }

            sendRequestToClient(task->request.get(), task->addrPort);
        });
        lambda_makeConnects(task, fw);
        auto future = QtConcurrent::mapped(sequence, &ExampleServer::sortArray); // slightly better to make all inplace_merge in the end instead of reduce?
        fw->setFuture(future);
        break;
    }
    case RequestType::FindPrimeNumbers:
    {
        if (m_taskMap.contains(addrPort))
        {
            sendErrorToClient(Protocol::ErrorCode::AlreadyRunningTask, addrPort);
            return;
        }
        constexpr RequestType ReqT = RequestType::FindPrimeNumbers;
        auto req = make_unique<RStMapper_t<ReqT>>();
        streamMsg.device()->seek(0);
        streamMsg >> *req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedRequest(msg, addrPort);
            return;
        }

        auto sequence = divideIntoChunks(req->x_from, req->x_to, m_maxChunkCount, m_minChunkSize);

        auto iter = m_taskMap.insert(addrPort, make_shared<Task>());
        Task* task = iter.value().get();
        task->request = std::move(req);
        task->futureWatcher = make_unique<RFWMapper_t<ReqT>>();
        task->addrPort = addrPort;
        task->rmsgHash = msgHash;
        auto* fw = watcher_cast<ReqT>(task->futureWatcher.get());
        QObject::connect(fw, &QFutureWatcherBase::finished, this, [this, task]() {
            constexpr RequestType ReqT = RequestType::FindPrimeNumbers;
            auto req = request_cast<ReqT>(task->request.get());
            auto fw = watcher_cast<ReqT>(task->futureWatcher.get());
            if (fw->isCanceled()) // don't send anything if task was canceled
                return;

            req->primeNumbers = std::move(fw->result());

            sendRequestToClient(task->request.get(), task->addrPort);
        });
        lambda_makeConnects(task, fw);
        auto future = QtConcurrent::mappedReduced(sequence, &ExampleServer::findPrimeNumbersT, &ExampleServer::findPrimeNumbers_reduce, QtConcurrent::OrderedReduce);
        fw->setFuture(future);
        break;
    }
    case RequestType::CalculateFunction:
    {
        if (m_taskMap.contains(addrPort))
        {
            sendErrorToClient(Protocol::ErrorCode::AlreadyRunningTask, addrPort);
            return;
        }
        constexpr RequestType ReqT = RequestType::CalculateFunction;
        auto req = make_unique<RStMapper_t<ReqT>>();
        streamMsg.device()->seek(0);
        streamMsg >> *req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedRequest(msg, addrPort);
            return;
        }

        auto sequence = [&](){
            QVector<std::tuple<Protocol::EquationType, int, int, int, const int, const int, const int>> sequence;
            auto ranges = divideIntoChunks(req->x_from, req->x_to, m_maxChunkCount, m_minChunkSize);
            for (auto const& range : ranges)
                sequence.append(make_tuple(req->equationType, req->x_from, req->x_to, req->x_step, req->a, req->b, req->c));
            return sequence;
        }();

        auto iter = m_taskMap.insert(addrPort, make_shared<Task>());
        Task* task = iter.value().get();
        task->request = std::move(req);
        task->futureWatcher = make_unique<RFWMapper_t<ReqT>>();
        task->addrPort = addrPort;
        task->rmsgHash = msgHash;
        auto* fw = watcher_cast<ReqT>(task->futureWatcher.get());
        QObject::connect(fw, &QFutureWatcherBase::finished, this, [this, task]() {
            constexpr RequestType ReqT = RequestType::CalculateFunction;
            auto req = request_cast<ReqT>(task->request.get());
            auto fw = watcher_cast<ReqT>(task->futureWatcher.get());
            if (fw->isCanceled()) // don't send anything if task was canceled
                return;

            req->points = std::move(fw->result());

            sendRequestToClient(task->request.get(), task->addrPort);
        });
        lambda_makeConnects(task, fw);
        auto future = QtConcurrent::mappedReduced(sequence, &ExampleServer::calculateFunctionT, &ExampleServer::calculateFunction_reduce, QtConcurrent::OrderedReduce);
        fw->setFuture(future);
        break;
    }
    case RequestType::CancelCurrentTask:
    {
        constexpr RequestType ReqT = RequestType::CancelCurrentTask;
        auto req = make_unique<RStMapper_t<ReqT>>();
        streamMsg.device()->seek(0);
        streamMsg >> *req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedRequest(msg, addrPort);
            return;
        }

        auto iter = m_taskMap.find(addrPort);
        if (iter == m_taskMap.end())
        {
            f_logError(QStringLiteral("%1; addrPort=%2").arg(toQString(Protocol::ErrorCode::NotRunningAnyTask)).arg(toQString(addrPort)));
            // Respond to client anyway since it awaits answer
            sendRequestToClient(req.get(), addrPort);
        }
        else
        {
            Task* task = iter.value().get();
            task->futureWatcher->cancel(); // the rest should be done by functions connected to finished() and canceled()
        }
        break;
    }
    default:
    {
        auto errorCode = Protocol::ErrorCode::InvalidRequestType;
        f_logError(QStringLiteral("%1; addrPort=%2; msg=%3").arg(toQString(errorCode)).arg(toQString(addrPort)).arg(QString::fromLatin1(msg.toHex())));
        sendErrorToClient(errorCode, addrPort);
        return;
    }
    }
}

void ExampleServer::onCorruptedRequest(QByteArray msg, Net::AddressPort addrPort)
{
    QString errorText(QStringLiteral("Received message with corrupted data"));
    f_logError(QString("%1: %2").arg(errorText).arg(QString::fromLatin1(msg.toHex())));
    Request_InvalidRequest response;
    response.errorText = errorText;
    sendRequestToClient(&response, addrPort);
}

QVector<int> ExampleServer::sortArray(QVector<int> arr)
{
    std::sort(arr.begin(), arr.end());
    return arr;
}

QVector<int> ExampleServer::findPrimeNumbers(int numFrom, int numTo)
{
    auto isPrime = [](int n) -> bool {
        int limit = sqrt(n);
        for (int i = 3; i <= limit; i += 2)
        {
            if (n % i == 0)
                return false;
        }
        return true;
    };

    if (numFrom > numTo)
        return {};

    QVector<int> primes;
    // Start checking from 3 onwards
    if (numFrom <= 2)
    {
        primes.push_back(2);
        numFrom = 3;
    }
    else if (numFrom % 2 == 0) // Ensure odd starting point
    {
        numFrom++;
    }

    for (int num = numFrom; num <= numTo; num += 2)
    {
        if (isPrime(num))
            primes.push_back(num);
    }
    return primes;
}

QVector<QPoint> ExampleServer::calculateFunction(EquationType equationType, int x_from, int x_to, int x_step, const int constantA, const int constantB, const int constantC)
{
    QVector<int> x_values;
    x_values.reserve((x_to - x_from) / x_step);
    for (int x = x_from; x <= x_to; x += x_step)
        x_values.push_back(x);

    QVector<QPoint> result;
    switch (equationType)
    {
    case EquationType::Linear:
    {
        for (auto x : x_values)
        {
            int y = constantA * x + constantB;
            result.push_back(QPoint{x, y});
        }
        break;
    }
    case EquationType::Quadratic:
    {
        for (auto x : x_values)
        {
            int y = constantA * pow(x, 2) + constantB * x + constantC;
            result.push_back(QPoint{x, y});
        }
        break;
    }
    default: { break; }
    }
    return result;
}

void ExampleServer::onClientConnected(Net::AddressPort addrPort)
{
    return; // Nothing to do I guess, since authorization is handled by TcpServer itself
}

// Client can disconnect without sending cancel request -> need to force cancel its task
void ExampleServer::onClientDisconnected(Net::AddressPort addrPort)
{
    auto iter = m_taskMap.find(addrPort);
    if (iter == m_taskMap.end())
        return;
    Task* task = iter.value().get();
    task->futureWatcher->cancel();
}
