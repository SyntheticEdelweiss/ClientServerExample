#pragma once

#include <QVector>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QPoint>
#include <QtCore/QVector>

#include <QtCore/QDataStream>

namespace Protocol {

enum class EquationType : int
{
    Linear,
    Quadratic
};
inline QString toQString(EquationType data)
{
    switch (data)
    {
    case EquationType::Linear: { return "Linear"; }
    case EquationType::Quadratic: { return "Quadratic"; }
    default: { return "Invalid"; }
    }
}


enum class RequestType
{
    Invalid = 0,
    SortArray,
    FindPrimeNumbers,
    CalculateFunction,

    StopCurrentOperation,
};

struct Request
{
    RequestType type{RequestType::Invalid};

    explicit Request(RequestType a_reqType = RequestType::Invalid) : type(a_reqType) {}
    virtual ~Request() = default;

    friend QDataStream& operator<<(QDataStream& stream, const Request& data);
    friend QDataStream& operator>>(QDataStream& stream, Request& data);

    virtual QDataStream& serialize(QDataStream& stream) const;
    virtual QDataStream& deserialize(QDataStream& stream);
    QJsonObject toJson() const;
    static Request fromJson(const QJsonObject& json);
};
inline QDataStream& operator<<(QDataStream& stream, const Request& data) { return data.serialize(stream); }
inline QDataStream& operator>>(QDataStream& stream, Request& data) { return data.deserialize(stream); }

struct Request_SortArray : public Request
{
    QVector<int> numbers;

    Request_SortArray() : Request(RequestType::SortArray) {}
    virtual ~Request_SortArray() = default;

    QDataStream& serialize(QDataStream& stream) const final;
    QDataStream& deserialize(QDataStream& stream) final;
    QJsonObject toJson() const;
    static Request_SortArray fromJson(const QJsonObject& json);
};

struct Request_PrimeNumbers : public Request
{
    int x_from;
    int x_to;
    QVector<int> primeNumbers;

    Request_PrimeNumbers() : Request(RequestType::FindPrimeNumbers) {}
    virtual ~Request_PrimeNumbers() = default;

    QDataStream& serialize(QDataStream& stream) const final;
    QDataStream& deserialize(QDataStream& stream) final;
    QJsonObject toJson() const;
    static Request_PrimeNumbers fromJson(const QJsonObject& json);
};

struct Request_CalculateFunction : public Request
{
    EquationType equationType;
    int x_from;
    int x_to;
    int x_step;
    int a;
    int b;
    int c;
    QVector<QPoint> points;

    Request_CalculateFunction() : Request(RequestType::CalculateFunction) {}
    virtual ~Request_CalculateFunction() = default;

    QDataStream& serialize(QDataStream& stream) const final;
    QDataStream& deserialize(QDataStream& stream) final;
    QJsonObject toJson() const;
    static Request_CalculateFunction fromJson(const QJsonObject& json);
};


enum class ResponseType
{
    Invalid = 0,
    ProgressUpdate
};

struct Response
{
    ResponseType type;
    bool success;
    QString message;

    explicit Response(ResponseType a_resType = ResponseType::Invalid) : type(a_resType) {}
    virtual ~Response() = default;

    friend QDataStream& operator<<(QDataStream& stream, const Response& data);
    friend QDataStream& operator>>(QDataStream& stream, Response& data);
    virtual QDataStream& serialize(QDataStream& stream) const;
    virtual QDataStream& deserialize(QDataStream& stream);
    QJsonObject toJson() const;
    static Response fromJson(const QJsonObject& json);
};
inline QDataStream& operator<<(QDataStream& stream, const Response& data) { return data.serialize(stream); }
inline QDataStream& operator>>(QDataStream& stream, Response& data) { return data.deserialize(stream); }

struct Response_ProgressUpdate : public Response
{
    int cur;
    int max;

    explicit Response_ProgressUpdate(ResponseType a_resType = ResponseType::ProgressUpdate) : Response(a_resType) {}
    virtual ~Response_ProgressUpdate() = default;

    QDataStream& serialize(QDataStream& stream) const final;
    QDataStream& deserialize(QDataStream& stream) final;
    QJsonObject toJson() const;
    static Response_ProgressUpdate fromJson(const QJsonObject& json);
};

}  // namespace Protocol

Q_DECLARE_METATYPE(Protocol::RequestType)
Q_DECLARE_METATYPE(Protocol::ResponseType)
Q_DECLARE_METATYPE(Protocol::EquationType)
