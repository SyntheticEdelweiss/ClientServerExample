#pragma once

#include <QVector>

#include <QtCore/QObject>
#include <QtCore/QPoint>

#include "Common/Protocol.hpp"
#include "Net/TcpServer.hpp"

class ExampleServer : public QObject
{
    Q_OBJECT
public:
    explicit ExampleServer(Net::ConnectionSettings serverSettings, QObject* parent = nullptr);

private:
    TcpServer* m_server;

    uint m_regId_general = 0;
    std::function<void(QString)> f_logGeneral = [](QString msg) { qInfo(qUtf8Printable(QDateTime::currentDateTimeUtc().toString(QStringLiteral("[yyyy.MM.dd-hh:mm:ss.zzz]")) + msg)); };
    std::function<void(QString)> f_logError = [](QString msg) { qWarning(qUtf8Printable(QDateTime::currentDateTimeUtc().toString(QStringLiteral("[yyyy.MM.dd-hh:mm:ss.zzz]")) + msg)); };

private:
    void parseRequest(QByteArray msg, NetConnection* const, Net::AddressPort addrPort);

    QVector<int> sortArray(QVector<int> arr);
    QVector<int> findPrimeNumbers(int numFrom, int numTo);
    QVector<QPoint> calculateFunction(Protocol::EquationType equationType, int x_from, int x_to, int x_step, const int constantA, const int constantB, const int constantC);

signals:
};

