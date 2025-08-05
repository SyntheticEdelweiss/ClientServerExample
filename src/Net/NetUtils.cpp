#include "NetUtils.hpp"

using namespace Net;

bool Net::operator==(const ConnectionSettings& lhv, const ConnectionSettings& rhv)
{
    return (true // for prettier code alignment
            && (lhv.ipLocal == rhv.ipLocal)
            && (lhv.ipDestination == rhv.ipDestination)
            && (lhv.portIn == rhv.portIn)
            && (lhv.portOut == rhv.portOut)
            );
}
QDataStream& Net::operator<<(QDataStream& stream, const ConnectionSettings& data)
{
    stream << data.ipLocal;
    stream << data.ipDestination;
    stream << data.portIn;
    stream << data.portOut;
    return stream;
}
QDataStream& Net::operator>>(QDataStream& stream, ConnectionSettings& data)
{
    stream >> data.ipLocal;
    stream >> data.ipDestination;
    stream >> data.portIn;
    stream >> data.portOut;
    return stream;
}

QList<AddressPort> Net::addressPortListFromQString(const QString &s)
{
    QList<AddressPort> result;
    QStringList items = s.split(' ', Qt::SkipEmptyParts);
    for (const QString& item : items)
    {
        QStringList addrPortString = item.split(':', Qt::SkipEmptyParts);
        if (addrPortString.size() != 2)
            continue;
        QString addrString = addrPortString[0];
        QString portString = addrPortString[1];
        result << AddressPort{QHostAddress(addrString), portString.toUShort()};
    }
    return result;
}
