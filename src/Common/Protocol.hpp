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

enum class ErrorCode : uint
{
    Unspecified = 0,
    CorruptedData,
    InvalidRequestType,
    AlreadyRunningTask,
    NotRunningAnyTask,
};
inline QString toQString(ErrorCode code)
{
    switch (code)
    {
    case ErrorCode::CorruptedData: { return QStringLiteral("Received message with corrupted data"); }
    case ErrorCode::InvalidRequestType: { return QStringLiteral("Received message with invalid request type"); }
    case ErrorCode::AlreadyRunningTask: { return QStringLiteral("Received task request while already running task"); }
    case ErrorCode::NotRunningAnyTask: { return QStringLiteral("Received CancelCurrentTask while not running any task"); }
    case ErrorCode::Unspecified: [[fallthrough]];
    default: { return {}; }
    }
}


enum class RequestType
{
    InvalidRequest = 0,
    SortArray,
    FindPrimeNumbers,
    CalculateFunction,
    CancelCurrentTask,

    ProgressRange,
    ProgressValue,
};
inline QString toQString(RequestType d)
{
    switch (d)
    {
    case RequestType::InvalidRequest: { return QStringLiteral("InvalidRequest"); }
    case RequestType::SortArray: { return QStringLiteral("SortArray"); }
    case RequestType::FindPrimeNumbers: { return QStringLiteral("FindPrimeNumbers"); }
    case RequestType::CalculateFunction: { return QStringLiteral("CalculateFunction"); }
    case RequestType::CancelCurrentTask: { return QStringLiteral("CancelCurrentTask"); }
    case RequestType::ProgressRange: { return QStringLiteral("ProgressRange"); }
    case RequestType::ProgressValue: { return QStringLiteral("ProgressValue"); }
    default: { return {}; }
    }
}

struct Request
{
    RequestType type{RequestType::InvalidRequest};

    explicit Request(RequestType a_reqType = RequestType::InvalidRequest) : type(a_reqType) {}
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

struct Request_InvalidRequest : public Request
{
    ErrorCode errorCode{ErrorCode::Unspecified};
    QString errorText;

    Request_InvalidRequest() : Request(RequestType::InvalidRequest) {}
    virtual ~Request_InvalidRequest() = default;

    virtual QDataStream& serialize(QDataStream& stream) const final;
    virtual QDataStream& deserialize(QDataStream& stream) final;
};

struct Request_SortArray : public Request
{
    QVector<int> numbers;

    Request_SortArray() : Request(RequestType::SortArray) {}
    virtual ~Request_SortArray() = default;

    virtual QDataStream& serialize(QDataStream& stream) const final;
    virtual QDataStream& deserialize(QDataStream& stream) final;
    QJsonObject toJson() const;
    static Request_SortArray fromJson(const QJsonObject& json);
};

struct Request_FindPrimeNumbers : public Request
{
    int x_from;
    int x_to;
    QVector<int> primeNumbers;

    Request_FindPrimeNumbers() : Request(RequestType::FindPrimeNumbers) {}
    virtual ~Request_FindPrimeNumbers() = default;

    virtual QDataStream& serialize(QDataStream& stream) const final;
    virtual QDataStream& deserialize(QDataStream& stream) final;
    QJsonObject toJson() const;
    static Request_FindPrimeNumbers fromJson(const QJsonObject& json);
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

    virtual QDataStream& serialize(QDataStream& stream) const final;
    virtual QDataStream& deserialize(QDataStream& stream) final;
    QJsonObject toJson() const;
    static Request_CalculateFunction fromJson(const QJsonObject& json);
};

struct Request_CancelCurrentTask : public Request
{
    Request_CancelCurrentTask() : Request(RequestType::CancelCurrentTask) {}
    virtual ~Request_CancelCurrentTask() = default;
};

struct Request_ProgressRange : public Request
{
    int minimum = 0;
    int maximum = 0;

    Request_ProgressRange() : Request(RequestType::ProgressRange) {}
    virtual ~Request_ProgressRange() = default;

    virtual QDataStream& serialize(QDataStream& stream) const final;
    virtual QDataStream& deserialize(QDataStream& stream) final;
};

struct Request_ProgressValue : public Request
{
    int value = 0;

    Request_ProgressValue() : Request(RequestType::ProgressValue) {}
    virtual ~Request_ProgressValue() = default;

    virtual QDataStream& serialize(QDataStream& stream) const final;
    virtual QDataStream& deserialize(QDataStream& stream) final;
};

}  // namespace Protocol

Q_DECLARE_METATYPE(Protocol::EquationType)
Q_DECLARE_METATYPE(Protocol::ErrorCode)
Q_DECLARE_METATYPE(Protocol::RequestType)
