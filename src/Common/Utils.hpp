#pragma once

#include <tuple>
#include <type_traits>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QJsonValue>
#include <QtCore/QVector>
#include <QtGlobal>


#ifdef _WIN32
    #ifndef BYTESWAP_CALLS_NORMALIZED
        #define BYTESWAP_CALLS_NORMALIZED
        #define bswap_16(x) _byteswap_ushort(x)
        #define bswap_32(x) _byteswap_ulong(x)
        #define bswap_64(x) _byteswap_uint64(x)
    #endif
#else
    #include <byteswap.h>
#endif

#if (__cplusplus < 202002L)
#define remove_cvref_t(T) \
    std::remove_cv_t<std::remove_reference_t<T>>
#endif

#define under_cast(EnumValue) \
    static_cast<std::underlying_type_t<decltype(EnumValue)>>(EnumValue)

#define c_cast(type, var) \
    (type)((var))

#define Q_CRITICAL_UNREACHABLE() \
    qCritical(__FILE__ ":" __STRINGIFY(__LINE__) " should be unreachable.");

template<typename T>
class IsQVariantConvertible
{
private:
    template<typename U>
    static auto test(int) -> decltype(QVariant::fromValue(std::declval<U>()), std::true_type());

    template<typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};
template<class T>
constexpr bool IsQVariantConvertible_v = IsQVariantConvertible<T>::value;


template<typename T>
bool parseJsonVar(const QJsonObject& curObject, QString curValueName, T& targetVar, QString* errorText = nullptr)
{
    static_assert(IsQVariantConvertible_v<T>, "type unsupported by parseJsonVar()");
    QJsonValue curValue = curObject[curValueName];
    if (curValue.isUndefined())
    {
        if (errorText != nullptr)
            *errorText = QStringLiteral("\"%1\" is not found").arg(curValueName);
        return false;
    }
    if (!curValue.toVariant().canConvert<T>())
    {
        if (errorText != nullptr)
            *errorText = QStringLiteral("\"%1\" is invalid").arg(curValueName);
        return false;
    }
    targetVar = curValue.toVariant().value<T>();
    return true;
}

QVector<std::tuple<int, int>> divideIntoChunks(int x_from, int x_to, int max_chunk_count, int min_chunk_size = 1);

template<typename T>
QVector<QVector<T>> divideIntoChunks(QVector<T> container, int max_chunk_count, int min_chunk_size)
{
    QVector<QVector<T>> subcontainers;
    auto indexRanges = divideIntoChunks(0, container.size() - 1, max_chunk_count, min_chunk_size);
    if (indexRanges.isEmpty())
        return {};
    for (auto const& indexRange : indexRanges)
    {
        auto indexStart = std::get<0>(indexRange);
        auto indexEnd = std::get<1>(indexRange) + 1;
        subcontainers.append(QVector<T>{container.begin() + indexStart, container.begin() + indexEnd});
    }
    return subcontainers;
}
