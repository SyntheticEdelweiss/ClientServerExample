#pragma once

#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslSocket>

#include "TcpServer.hpp"

class SslServer;
class SslServerInternal;

class SslServerInternal : public QTcpServer
{
    Q_OBJECT
    friend class SslServer;
public:
    SslServerInternal(SslServer* parent);
    virtual ~SslServerInternal() = default;

protected:
    SslServer* m_pServer = nullptr;

protected:
    virtual void incomingConnection(qintptr socketDescriptor) override;
};

class SslServer : public TcpServer
{
    Q_OBJECT
    friend class SslServerInternal;
protected:
    SslServer(const quint8 _connType, const QString _connTypeName, QObject* parent = nullptr) : TcpServer(_connType, _connTypeName, parent) {}
    inline virtual SslServerInternal* createServer() override { return (new SslServerInternal(this)); }

public:
    SslServer(QObject* parent = nullptr) : SslServer(Net::ConnectionType::SslServer, "SslServer", parent) {}
    virtual ~SslServer() {}
    SslServer(const SslServer&) = delete;            // Copy constructor
    SslServer(SslServer&&) = delete;                 // Move constructor
    SslServer& operator=(const SslServer&) = delete; // Copy assignment
    SslServer& operator=(SslServer&&) = delete;      // Move assignment

protected:
    SslServerInternal*& m_pSslServer = reinterpret_cast<SslServerInternal*&>(m_pServer);
    bool m_isIgnoreSslErrors = false;
    QSslConfiguration m_sslConfiguration;

public:
    inline void setIgnoreSslErrors(bool isEnabled) { m_isIgnoreSslErrors = isEnabled; }
    void setSslConfiguration(const QSslConfiguration& configuration);

public slots:
    // Net::ConnectionState openConnection(Net::ConnectionSettings const& a_connectionSettings) override;

protected slots:
    virtual void onSslErrors(const QList<QSslError>& errors);
};

