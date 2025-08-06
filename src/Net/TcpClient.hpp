#pragma once

#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>

#include "NetConnection.hpp"

class TcpClient : public NetConnection
{
    Q_OBJECT
protected:
    TcpClient(const quint8 _connType, const QString _connTypeName, QObject* parent = nullptr);
    inline virtual QTcpSocket* createSocket() { return (new QTcpSocket(this)); }

public:
    TcpClient(QObject* parent = nullptr) : TcpClient(Net::ConnectionType::TcpClient, "TcpClient", parent) {}
    ~TcpClient();
    TcpClient(const TcpClient&) = delete;            // Copy constructor
    TcpClient(TcpClient&&) = delete;                 // Move constructor
    TcpClient& operator=(const TcpClient&) = delete; // Copy assignment
    TcpClient& operator=(TcpClient&&) = delete;      // Move assignment

protected: // members
    QTcpSocket* m_pTcpSocket;

    Net::PendingMessage m_pendingMsg;
    decltype(Net::PendingMessage::pendingSize) m_headerSize = sizeof(m_headerSize);

    QTimer* m_reconnectTimer = nullptr;
    bool m_isReconnectEnabled = true;
    int m_reconnectInterval = 60000; // msec
    int m_waitForConnectedInterval = 10000; // msec

    Net::LoginData m_loginData;
    bool m_isAuthorizationEnabled = false;

public: // methods
    virtual void printConnectionInfo() const override;

    QAbstractSocket::SocketState getSocketState() const { return m_pTcpSocket->state(); }
    QString getLastErrorString() const final { return m_pTcpSocket->errorString(); }
    Net::ConnectionSettings getConnectionSettingsActive() const final; // will differ from m_connectionSettings if ip=Any or port=0, those being actually used ones

    void setEnableReconnect(bool isEnabled);
    void setWaitTimes(int reconnectInterval, int waitForConnectedInterval);
    void setLoginData(Net::LoginData a_loginData);
    void setAuthorizationEnabled(bool isEnabled);

public slots:
    virtual Net::ConnectionState openConnection(Net::ConnectionSettings const& a_connectionSettings) override;
    virtual void closeConnection() override;
    virtual qint64 sendMessage(const QByteArray& msg) override;

protected slots:
    virtual void readReceived() override;
    void printError() const final;
    virtual void tryConnectToHost();
    void onConnected();
    void onDisconnected();
    void authorize();

signals:
    void readPartialDone(const QByteArray& msg, const QDateTime& dt = QDateTime::currentDateTimeUtc()) const;
};
