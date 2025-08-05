#pragma once

#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkProxyFactory>

#include "NetUtils.hpp"

class NetConnection : public QObject
{
    Q_OBJECT
public:
    NetConnection(const quint8 _connType, const QString _connTypeName, QObject* parent = nullptr);
    virtual ~NetConnection() = 0;
    NetConnection(const NetConnection&) = delete;            // Copy constructor
    NetConnection(NetConnection&&) = delete;                 // Move constructor
    NetConnection& operator=(const NetConnection&) = delete; // Copy assignment
    NetConnection& operator=(NetConnection&&) = delete;      // Move assignment

public: // members
    const quint8 m_connectionType = 0;
    const QString m_connectionTypeName;

protected: // members
    uint m_connectionId = 0;
    Net::ConnectionState m_connectionState = Net::ConnectionState::NotCreated;
    Net::ConnectionSettings m_connectionSettings;
    QThread* m_pCallbackThread = nullptr;
    QObject* m_pCallbackThreadContextHelper = nullptr;

    std::function<void(QByteArray, NetConnection* const, Net::AddressPort)> f_onReceivedMessage = {}; // empty function causing segfault is intended - if that happens, you're missing a setCallbackFunction() call
    std::function<void(QString)> f_logGeneral = [](QString msg) { qWarning(qUtf8Printable(QDateTime::currentDateTimeUtc().toString(QStringLiteral("[yyyy.MM.dd-hh:mm:ss.zzz]")) + msg)); };
    std::function<void(QString)> f_logError = [](QString msg) { qWarning(qUtf8Printable(QDateTime::currentDateTimeUtc().toString(QStringLiteral("[yyyy.MM.dd-hh:mm:ss.zzz]")) + msg)); };

public: // methods
    virtual void printConnectionInfo() const = 0;
    void printConnectionSettings() const;

    // Setters
    void setConnectionSettings(Net::ConnectionSettings const& a_connectionSettings);
    void setConnectionId(uint new_id);
    void setCallbackFunction(std::function<void(QByteArray, NetConnection* const, Net::AddressPort)> a_OnRecvMessage);
    void setLoggingFunctions(std::function<void(QString)> a_logGeneral, std::function<void(QString)> a_logError);
    virtual bool setIsCallbackDistinctThread(const bool a_isCallbackDistinctThread); // returns true if calling setCallbackFunction is required, and false otherwise // you MUST call setCallbackFunction AFTER this to avoid undefined behavior

    // Getters
    decltype(f_logGeneral) get_f_logGeneral() const { return f_logGeneral; }
    decltype(f_logError) get_f_logError() const { return f_logError; }
    uint getConnectionId() const { return m_connectionId; }
    quint8 getConnectionType() const { return m_connectionType; }
    Net::ConnectionState getConnectionState() const { return m_connectionState; }
    Net::ConnectionSettings const& getConnectionSettings() const { return m_connectionSettings; }
    virtual Net::ConnectionSettings getConnectionSettingsActive() const { return getConnectionSettings(); } // should differ from m_connectionSettings if ip=Any or port=0, those being actually used ones
    virtual QString getLastErrorString() const = 0;
    bool getIsCallbackDistinctThread() const { return (m_pCallbackThread == nullptr) ? false : true; }
    QObject* getCallbackThreadContext() const { return m_pCallbackThreadContextHelper; } // only meant for signal/slot connections to specify thread of slot execution. Do NOT do anything else with it or woe be upon ye

    QString nameId() const { return QString("%1 (id=%2)").arg(objectName()).arg(getConnectionId()); }

public slots:
    Net::ConnectionState openConnection();
    virtual Net::ConnectionState openConnection(Net::ConnectionSettings const& a_connectionSettings) = 0;
    Net::ConnectionState reopenConnection();
    Net::ConnectionState reopenConnection(Net::ConnectionSettings const& a_connectionSettings);
    virtual void closeConnection() = 0;
    virtual qint64 sendMessage(const QByteArray& msg) = 0;

protected slots:
    virtual void readReceived() = 0;
    virtual void printError() const = 0;

signals: // protected
    // Meant for internal use - should only be emitted from inside Net classes
    void socketStateChanged(const QAbstractSocket::SocketState&) const; // Retransmit QAbstractSocket::stateChanged or "substitute" it if not available (e.g. in serial port)
    void openedConnection(bool isOpened);
    void closedConnection();
    void readDone(const QByteArray& msg, const QDateTime& dt = QDateTime::currentDateTimeUtc()) const;
    void writeDone(const QByteArray& msg, const QDateTime& dt = QDateTime::currentDateTimeUtc()) const;

signals: // public
    // Meant for external use - can be called from anywhere
    void openConnectionQueued();
    void openConnectionQueued(const Net::ConnectionSettings&);
    void reopenConnectionQueued();
    void reopenConnectionQueued(const Net::ConnectionSettings&);
    void closeConnectionQueued();
    void sendMessageQueued(const QByteArray&) const;
};
