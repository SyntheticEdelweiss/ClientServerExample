#pragma once

#include <memory>

#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "NetConnection.hpp"

class TcpServer : public NetConnection
{
    Q_OBJECT
public:
    struct ClientData
    {
        QTcpSocket* pSocket = nullptr;
        Net::AddressPort peerAddrPort;
        Net::AddressPort localAddrPort;
        Net::LoginData loginData;
    };

protected:
    TcpServer(const quint8 _connType, const QString _connTypeName, QObject* parent = nullptr);

public:
    TcpServer(QObject* parent = nullptr) : TcpServer(Net::ConnectionType::TcpServer, "TcpServer", parent) {}
    virtual ~TcpServer();
    TcpServer(const TcpServer&) = delete;            // Copy constructor
    TcpServer(TcpServer&&) = delete;                 // Move constructor
    TcpServer& operator=(const TcpServer&) = delete; // Copy assignment
    TcpServer& operator=(TcpServer&&) = delete;      // Move assignment

protected: // members
    QTcpServer* m_pServer;
    QHash<QTcpSocket*, std::shared_ptr<ClientData>> m_clientMap; // need to keep Data as pointer for polymorphic access // should be unique_ptr, but QHash requires copy-able value
    QHash<Net::AddressPort, ClientData*> m_clientByPeerAddressPort;
    QHash<QHostAddress, QHash<quint16, ClientData*>> m_clientsByPeerAddress;
    QHash<QString, ClientData*> m_clientsByLoginUsername;

    QSet<QHostAddress> m_allowedAddresses;
    bool m_isAllowAllAdresses = true;

    QSet<Net::LoginData> m_loginData;
    bool m_isAuthorizationEnabled = false;
    QMap<QTcpSocket*, std::shared_ptr<QTimer>> m_socketAuthMap;
    const int m_authTimeoutTime = 3000;

    QHash<QTcpSocket*, Net::PendingMessage> m_pendingMsgBySocket;
    decltype(Net::PendingMessage::pendingSize) m_headerSize = sizeof(m_headerSize);

public: // methods
    void printConnectionInfo() const override;

    QString getLastErrorString() const final { return m_pServer->errorString(); }
    Net::ConnectionSettings getConnectionSettingsActive() const final;
    inline uint getConnectionCount() const { return m_clientMap.size(); }

    void setAllowAllAddresses(bool isAllowed) { m_isAllowAllAdresses = isAllowed; }
    bool getIsClientConnected(const Net::AddressPort addrPort);

    void setAuthorizationEnabled(bool isEnabled);
    void addLoginData(Net::LoginData loginData);
    void removeLoginData(Net::LoginData loginData);

protected:
    qint64 sendMessageTo(QByteArray msg, QTcpSocket* pClientSocket);

public slots:
    Net::ConnectionState openConnection(Net::ConnectionSettings const& a_connectionSettings) override;
    void closeConnection() override;
    qint64 sendMessage(const QByteArray& msg) override;
    qint64 sendMessageTo(QByteArray msg, QHostAddress address, quint16 port);
    qint64 sendMessageTo(QByteArray msg, Net::AddressPort addressPort);
    qint64 sendMessageTo(QByteArray msg, QHostAddress address);
    qint64 sendMessageTo(QByteArray msg, QString loginUsername);

    void addAllowedAddress(QHostAddress addr);
    void removeAllowedAddress(QHostAddress addr);

protected slots:
    void readReceived() override;
    virtual void onNewConnection();
    virtual void onSocketDisconnected();
    void printError() const final;
    void printSocketError() const;

signals:
    void clientConnected(Net::AddressPort);
    void clientDisconnected(Net::AddressPort);

    void sendMessageToQueued(QByteArray msg, QHostAddress address, quint16 port);
    void sendMessageToQueued(QByteArray msg, Net::AddressPort addressPort);
    void sendMessageToQueued(QByteArray msg, QHostAddress address);
    void sendMessageToQueued(QByteArray msg, QString loginUsername);

    void addAllowedAddressQueued(QHostAddress addr);
    void removeAllowedAddressQueued(QHostAddress addr);

    void addLoginDataQueued(Net::LoginData loginData);
    void removeLoginDataQueued(Net::LoginData loginData);
    void clientAuthorized(QString username, Net::AddressPort addrPort);

    void readPartialDone(QByteArray msg, QDateTime dt = QDateTime::currentDateTimeUtc()) const;
};
