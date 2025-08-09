#include "Protocol.hpp"

#include "Utils.hpp"

using namespace std;

namespace Protocol {

QDataStream& Request::serialize(QDataStream& stream) const
{
    stream << type;
    return stream;
}
QDataStream& Request::deserialize(QDataStream& stream)
{
    stream >> type;
    return stream;
}
QJsonObject Request::toJson() const
{
    QVariantMap result;
    result.insert("type", under_cast(type));
    // result.insert("data", data);
    return QJsonObject::fromVariantMap(result);
}
Request Request::fromJson(const QJsonObject& jsonObject)
{
    Request request;
    if (!parseJsonVar(jsonObject, "type", request.type)) goto goto_parseError;
    // if (!parseJsonVar(jsonObject, "data", request.data)) goto goto_parseError;
    return request;

    goto_parseError:;
    return Request{};
}

QDataStream& Request_InvalidRequest::serialize(QDataStream& stream) const
{
    Request::serialize(stream);
    stream << errorCode;
    stream << errorText;
    return stream;
}
QDataStream& Request_InvalidRequest::deserialize(QDataStream& stream)
{
    Request::deserialize(stream);
    stream >> errorCode;
    stream >> errorText;
    return stream;
}

QDataStream& Request_SortArray::serialize(QDataStream& stream) const
{
    Request::serialize(stream);
    stream << numbers;
    return stream;
}
QDataStream& Request_SortArray::deserialize(QDataStream& stream)
{
    Request::deserialize(stream);
    stream >> numbers;
    return stream;
}

QDataStream& Request_FindPrimeNumbers::serialize(QDataStream& stream) const
{
    Request::serialize(stream);
    stream << x_from;
    stream << x_to;
    stream << primeNumbers;
    return stream;
}
QDataStream& Request_FindPrimeNumbers::deserialize(QDataStream& stream)
{
    Request::deserialize(stream);
    stream >> x_from;
    stream >> x_to;
    stream >> primeNumbers;
    return stream;
}

QDataStream& Request_CalculateFunction::serialize(QDataStream& stream) const
{
    Request::serialize(stream);
    stream << equationType;
    stream << x_from;
    stream << x_to;
    stream << x_step;
    stream << a;
    stream << b;
    stream << c;
    stream << points;
    return stream;
}
QDataStream& Request_CalculateFunction::deserialize(QDataStream& stream)
{
    Request::deserialize(stream);
    stream >> equationType;
    stream >> x_from;
    stream >> x_to;
    stream >> x_step;
    stream >> a;
    stream >> b;
    stream >> c;
    stream >> points;
    return stream;
}
QJsonObject Request_CalculateFunction::toJson() const
{
    QVariantMap result;
    result.insert("type", under_cast(equationType));
    result.insert("x_from", x_from);
    result.insert("x_to", x_to);
    result.insert("x_step", x_step);
    result.insert("a", a);
    result.insert("b", b);
    result.insert("c", c);
    result.insert("points", QVariantList(points.cbegin(), points.cend()));
    return QJsonObject::fromVariantMap(result);
}
Request_CalculateFunction Request_CalculateFunction::fromJson(const QJsonObject& jsonObject)
{
    Request_CalculateFunction request;
    if (!parseJsonVar(jsonObject, "equationType", request.equationType)) goto goto_parseError;
    if (!parseJsonVar(jsonObject, "x_from", request.x_from)) goto goto_parseError;
    if (!parseJsonVar(jsonObject, "x_to", request.x_to)) goto goto_parseError;
    if (!parseJsonVar(jsonObject, "x_step", request.x_step)) goto goto_parseError;
    if (!parseJsonVar(jsonObject, "a", request.a)) goto goto_parseError;
    if (!parseJsonVar(jsonObject, "b", request.b)) goto goto_parseError;
    if (!parseJsonVar(jsonObject, "c", request.c)) goto goto_parseError;
    if (!parseJsonVar(jsonObject, "points", request.points)) goto goto_parseError;
    return request;

    goto_parseError:;
    return Request_CalculateFunction{};
}

QDataStream& Request_ProgressRange::serialize(QDataStream& stream) const
{
    Request::serialize(stream);
    stream << minimum;
    stream << maximum;
    return stream;
}
QDataStream& Request_ProgressRange::deserialize(QDataStream& stream)
{
    Request::deserialize(stream);
    stream >> minimum;
    stream >> maximum;
    return stream;
}

QDataStream& Request_ProgressValue::serialize(QDataStream& stream) const
{
    Request::serialize(stream);
    stream << value;
    return stream;
}
QDataStream& Request_ProgressValue::deserialize(QDataStream& stream)
{
    Request::deserialize(stream);
    stream >> value;
    return stream;
}

}  // namespace Protocol
