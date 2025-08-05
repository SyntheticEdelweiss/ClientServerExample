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

    void f_logGeneral(QString) { return; } // TODO
    void f_logError(QString) { return; }

private:
    void parseRequest(QByteArray msg, NetConnection* const, Net::AddressPort addrPort);

    QVector<int> sortArray(QVector<int> arr);
    QVector<int> findPrimeNumbers(int numFrom, int numTo);
    QVector<QPoint> calculateFunction(Protocol::EquationType equationType, int x_from, int x_to, int x_step, const int constantA, const int constantB, const int constantC);

signals:
};

