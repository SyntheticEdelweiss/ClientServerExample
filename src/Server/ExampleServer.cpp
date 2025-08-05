#include "ExampleServer.hpp"

#include <cmath>
#include <functional>

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
    using namespace std::placeholders;
    m_server = std::get<0>(Net::instantiateWaitThreadedConnection<TcpServer>());
    m_server->setAllowAllAddresses(true);
    m_server->setCallbackFunction(std::bind(&ExampleServer::parseRequest, this, _1, _2, _3));
    Net::openWaitThreadedConnection(m_server, serverSettings);
}


void ExampleServer::parseRequest(QByteArray msg, NetConnection* const, Net::AddressPort addrPort)
{
    Request request;
    MAKE_QDATASTREAM_NET(streamMsg, &msg, QIODevice::ReadOnly);
    streamMsg >> request;
    if (streamMsg.status() != QDataStream::Ok)
    {
        // TODO: log error and send response indicating invalid request
        f_logError(QString("IServGUI: received message with corrupted data: %2").arg(QString::fromLatin1(msg.toHex())));

        // Response response;
        // QByteArray responseArr{QJsonDocument(response.toJson()).toJson(QJsonDocument::Compact)};
        // m_server->sendMessageToQueued(responseArr, addrPort);
        return;
    }

    switch (request.type)
    {
    case RequestType::SortArray:
    {
        Request_SortArray req;
        streamMsg.device()->seek(0);
        streamMsg >> req;
        // QVector<int> arr = req.numbers;
        // sort(arr.begin(), arr.end());
        sort(req.numbers.begin(), req.numbers.end());

        QByteArray resMsg;
        MAKE_QDATASTREAM_NET(streamRes, &resMsg, QIODevice::WriteOnly);
        streamRes << req;
        m_server->sendMessageTo(resMsg, addrPort);
        // auto _tuple = std::make_tuple<>(arr);
        // auto pTask = new VariadicRunnable<ExampleServer, QVector<int>>(this, &ExampleServer::sortArray, _tuple);
        // pTask->setAutoDelete(true);
        // QThreadPool::globalInstance()->start(pTask);
        break;
    }
    case RequestType::FindPrimeNumbers:
    {
        Request_PrimeNumbers req;
        streamMsg.device()->seek(0);
        streamMsg >> req;
        auto primeNumbers = findPrimeNumbers(req.x_from, req.x_to);
        req.primeNumbers = primeNumbers;

        QByteArray resMsg;
        MAKE_QDATASTREAM_NET(streamRes, &resMsg, QIODevice::WriteOnly);
        streamRes << req;
        m_server->sendMessageTo(resMsg, addrPort);
        break;
    }
    case RequestType::CalculateFunction:
    {
        Request_CalculateFunction req;
        streamMsg.device()->seek(0);
        streamMsg >> req;
        // calculateFunction(EquationType equationType, int x_from, int x_to, int x_step, const int constantA, const int constantB, const int constantC)
        auto points = calculateFunction(req.equationType, req.x_from, req.x_to, req.x_step, req.a, req.b, req.c);
        req.points = points;

        QByteArray resMsg;
        MAKE_QDATASTREAM_NET(streamRes, &resMsg, QIODevice::WriteOnly);
        streamRes << req;
        m_server->sendMessageTo(resMsg, addrPort);
        break;
    }
    default:
    {
        // TODO: log error and send response indicating invalid request
        f_logError(QString("IServGUI: received message with corrupted data: %2").arg(QString::fromLatin1(msg.toHex())));

        // Response response;
        // QByteArray responseArr{QJsonDocument(response.toJson()).toJson(QJsonDocument::Compact)};
        // m_server->sendMessageToQueued(responseArr, addrPort);
        return;
    }
    }
}

QVector<int> ExampleServer::sortArray(QVector<int> arr) // write own implementation of sorting, enabling early stop and progress updates
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
