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
void Request::serialize(QJsonObject& target) const
{
    target.insert("type", under_cast(type));
}
bool Request::deserialize(QJsonObject& target, QString* errorText)
{
    if (!parseJsonVar(target, "type", type, errorText)) goto goto_parseError;
    return true;
goto_parseError:;
    return false;
}
int Request::byteSize()
{
    return sizeof(type);
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
void Request_InvalidRequest::serialize(QJsonObject& target) const
{
    Request::serialize(target);
    target.insert("errorCode", under_cast(errorCode));
    target.insert("errorText", errorText);
}
bool Request_InvalidRequest::deserialize(QJsonObject& target, QString* errorText)
{
    if (!Request::deserialize(target, errorText)) goto goto_parseError;
    if (!parseJsonVar(target, "errorCode", errorCode)) goto goto_parseError;
    if (!parseJsonVar(target, "errorText", this->errorText)) goto goto_parseError;
    return true;
goto_parseError:;
    return false;
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
void Request_SortArray::serialize(QJsonObject& target) const
{
    Request::serialize(target);
    target.insert("numbers", QJsonArray::fromVariantList(QVariantList(numbers.cbegin(), numbers.cend())));
}
bool Request_SortArray::deserialize(QJsonObject& target, QString* errorText)
{
    if (!Request::deserialize(target, errorText)) goto goto_parseError;
    if (!parseJsonVar(target, "numbers", numbers)) goto goto_parseError;
    return true;
goto_parseError:;
    return false;
}
int Request_SortArray::byteSize()
{
    int res = Request::byteSize();
    res += numbers.size();
    return res;
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
void Request_FindPrimeNumbers::serialize(QJsonObject& target) const
{
    Request::serialize(target);
    target.insert("x_from", x_from);
    target.insert("x_to", x_to);
    target.insert("primeNumbers", QJsonArray::fromVariantList(QVariantList(primeNumbers.cbegin(), primeNumbers.cend())));
}
bool Request_FindPrimeNumbers::deserialize(QJsonObject& target, QString* errorText)
{
    if (!Request::deserialize(target, errorText)) goto goto_parseError;
    if (!parseJsonVar(target, "x_from", x_from)) goto goto_parseError;
    if (!parseJsonVar(target, "x_to", x_to)) goto goto_parseError;
    if (!parseJsonVar(target, "primeNumbers", primeNumbers)) goto goto_parseError;
    return true;
goto_parseError:;
    return false;
}
int Request_FindPrimeNumbers::byteSize()
{
    int res = Request::byteSize();
    res += primeNumbers.size() * sizeof(decltype(primeNumbers)::value_type);
    return res;
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
void Request_CalculateFunction::serialize(QJsonObject& target) const
{
    Request::serialize(target);
    target.insert("equationType", under_cast(equationType));
    target.insert("x_from", x_from);
    target.insert("x_to", x_to);
    target.insert("x_step", x_step);
    target.insert("a", a);
    target.insert("b", b);
    target.insert("c", c);
    QVariantList stringPoints;
    stringPoints.reserve(points.size());
    for (QPoint const& item : points)
        stringPoints.push_back(QStringLiteral("%1;%2").arg(item.x()).arg(item.y()));
    target.insert("points", QJsonArray::fromVariantList(stringPoints));
}
bool Request_CalculateFunction::deserialize(QJsonObject& target, QString* errorText)
{
    if (!Request::deserialize(target, errorText)) goto goto_parseError;
    if (!parseJsonVar(target, "equationType", equationType)) goto goto_parseError;
    if (!parseJsonVar(target, "x_from", x_from)) goto goto_parseError;
    if (!parseJsonVar(target, "x_to", x_to)) goto goto_parseError;
    if (!parseJsonVar(target, "x_step", x_step)) goto goto_parseError;
    if (!parseJsonVar(target, "a", a)) goto goto_parseError;
    if (!parseJsonVar(target, "b", b)) goto goto_parseError;
    if (!parseJsonVar(target, "c", c)) goto goto_parseError;
    {
        QVector<QString> stringPoints;
        if (!parseJsonVar(target, "points", stringPoints)) goto goto_parseError;
        for (int i = 0; i < stringPoints.size(); ++i)
        {
            QString const& item = stringPoints[i];
            QStringList stringPoint = item.split(';');
            if (stringPoint.size() != 2)
            {
                *errorText = QStringLiteral("\"%1\" element at index %2 wrong type").arg("points").arg(i);
            }
            points.push_back(QPoint{stringPoint[0].toInt(), stringPoint[1].toInt()});
        }
    }
    return true;
goto_parseError:;
    return false;
}
int Request_CalculateFunction::byteSize()
{
    int res = Request::byteSize();
    res += sizeof(equationType);
    res += sizeof(x_from);
    res += sizeof(x_to);
    res += sizeof(x_step);
    res += sizeof(a);
    res += sizeof(b);
    res += sizeof(c);
    res += points.size() * sizeof(decltype(points)::value_type);
    return res;
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
void Request_ProgressRange::serialize(QJsonObject& target) const
{
    Request::serialize(target);
    target.insert("minimum", minimum);
    target.insert("maximum", maximum);
}
bool Request_ProgressRange::deserialize(QJsonObject& target, QString* errorText)
{
    if (!Request::deserialize(target, errorText)) goto goto_parseError;
    if (!parseJsonVar(target, "minimum", minimum)) goto goto_parseError;
    if (!parseJsonVar(target, "maximum", maximum)) goto goto_parseError;
    return true;
goto_parseError:;
    return false;
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
void Request_ProgressValue::serialize(QJsonObject& target) const
{
    Request::serialize(target);
    target.insert("value", value);
}
bool Request_ProgressValue::deserialize(QJsonObject& target, QString* errorText)
{
    if (!Request::deserialize(target, errorText)) goto goto_parseError;
    if (!parseJsonVar(target, "value", value)) goto goto_parseError;
    return true;
goto_parseError:;
    return false;
}

}  // namespace Protocol
