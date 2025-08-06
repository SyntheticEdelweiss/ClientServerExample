#pragma once

#include <QtCore/QDataStream>
#include <QtCore/QEventLoop>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>

#ifdef _WIN32
    #include <winsock2.h>
    #include <mstcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/ip.h>
    #include <netinet/tcp.h>
#endif

#ifndef MAKE_QDATASTREAM
#define MAKE_QDATASTREAM(streamName, bytearrayPtr, openMode, byteOrder)\
    QDataStream streamName(bytearrayPtr, openMode);\
    streamName.setByteOrder(byteOrder);
#endif

// One-liner macros to uniformly instantiate QDataStream with proper ByteOrder.
// Has to be macro because QDataStream forbids copy/move. inline function with copy elision should also suffice, but that requires c++17 and compiler support, which at least MinGW 8.1.0 doesn't provide
#define MAKE_QDATASTREAM_NET(streamName, bytearrayPtr, openMode)\
    QDataStream streamName(bytearrayPtr, openMode);\
    streamName.setByteOrder(Net::g_endianness);

#define MAKE_QDATASTREAM_NET_D(streamName, devicePtr)\
    QDataStream streamName(devicePtr);\
    streamName.setByteOrder(Net::g_endianness);

namespace Net
{
constexpr auto g_endianness = NET_ENDIANNESS; // set via cmake option

// Intended to be extendable, thus not enum
namespace ConnectionType
{
constexpr quint8 IncorrectType        = 0;
constexpr quint8 TcpServer            = 3;
constexpr quint8 TcpClient            = 4;
constexpr quint8 SslServer            = 5;
constexpr quint8 SslClient            = 6;
}

enum class ConnectionState
{
    Created,
    NotCreated,
    IncorrectIp
};

struct AddressPort
{
    QHostAddress addr{QHostAddress::Null};
    quint16 port{0};
};
inline QString toQString(AddressPort const& data) { return data.addr.toString() + ':' + QString::number(data.port); }
inline bool operator==(const Net::AddressPort& lhv, const Net::AddressPort& rhv)
{
    return (lhv.addr == rhv.addr) && (lhv.port == rhv.port);
}
inline uint qHash(const Net::AddressPort& key, uint seed)
{
    return ::qHash(key.addr.toIPv4Address(), seed) ^ key.port;
}
inline QDataStream& operator<<(QDataStream& stream, const AddressPort& data)
{
    return (stream << data.addr << data.port);
}
inline QDataStream& operator>>(QDataStream& stream, AddressPort& data)
{
    return (stream >> data.addr >> data.port);
}

struct ConnectionSettings // Contains data required to create a connection
{
    QHostAddress ipLocal{QHostAddress::AnyIPv4};
    QHostAddress ipDestination;
    quint16 portIn = 0;
    quint16 portOut = 0;
};
bool operator==(const ConnectionSettings& lhv, const ConnectionSettings& rhv);
QDataStream& operator<<(QDataStream& stream, const ConnectionSettings& data);
QDataStream& operator>>(QDataStream& stream, ConnectionSettings& data);

struct PendingMessage
{
    QByteArray msg;
    quint32 pendingSize = 0;
    int curPos = 0;
};


struct LoginData
{
    QString username;
    QString password;
};
inline bool operator==(const Net::LoginData& lhv, const Net::LoginData& rhv)
{
    return (lhv.username == rhv.username) && (lhv.password == rhv.password);
}
inline uint qHash(const LoginData& key, uint seed = 0)
{
    return qHash(key.username, seed) ^ qHash(key.password, seed << 1);
}
inline QDataStream& operator<<(QDataStream& stream, const LoginData& data)
{
    return (stream << data.username << data.password);
}
inline QDataStream& operator>>(QDataStream& stream, LoginData& data)
{
    return (stream >> data.username >> data.password);
}

template<typename T>
QString toQString(QList<T> const& a_list, QString const& delimiter)
{
    QString result;
    for (const auto& item : a_list)
        result.append(toQString(item)).append(delimiter);
    result.chop(delimiter.size());
    return result;
}

QList<AddressPort> addressPortListFromQString(const QString &s);

} // namespace Net

Q_DECLARE_METATYPE(Net::ConnectionState)
Q_DECLARE_METATYPE(Net::AddressPort)
Q_DECLARE_METATYPE(Net::ConnectionSettings)
Q_DECLARE_METATYPE(Net::LoginData)
