#pragma once

#include <QtNetwork/QSslSocket>

#include "TcpClient.hpp"

class SslClient : public TcpClient
{
    Q_OBJECT
protected:
    SslClient(const quint8 _connType, const QString _connTypeName, QObject* parent = nullptr);
    inline virtual QSslSocket* createSocket() override { return (new QSslSocket(this)); }

public:
    SslClient(QObject* parent = nullptr) : SslClient(Net::ConnectionType::SslClient, "SslClient", parent) {}
    virtual ~SslClient() = default;
    SslClient(const SslClient&) = delete;            // Copy constructor
    SslClient(SslClient&&) = delete;                 // Move constructor
    SslClient& operator=(const SslClient&) = delete; // Copy assignment
    SslClient& operator=(SslClient&&) = delete;      // Move assignment

protected:
    QSslSocket*& m_pSslSocket = reinterpret_cast<QSslSocket*&>(m_pTcpSocket);
    bool m_isIgnoreSslErrors = false;

public:
    inline void setIgnoreSslErrors(bool isEnabled) { m_isIgnoreSslErrors = isEnabled; }
    void setSslConfiguration(const QSslConfiguration& configuration);

protected slots:
    // virtual void readReceived() override;
    virtual void tryConnectToHost() override;

    virtual void onSslErrors(const QList<QSslError>& errors);
};
