#pragma once

#include <string>
#include <tuple>
#include <type_traits>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QJsonValue>
#include <QtCore/QString>
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


// type_trait to check for classes derived from std::basic_string
template<typename T>
struct is_basic_string
{
private:
    // Overload chosen if T* is convertible to basic_string<…>*, which holds for std::basic_string and all derived classes
    template<typename U>
    static std::true_type test(std::basic_string<
                               typename U::value_type,
                               typename U::traits_type,
                               typename U::allocator_type>*);

    // Fallback when the above is not viable
    template<typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};
template<typename T>
inline constexpr bool is_basic_string_v = is_basic_string<T>::value;

template<typename T>
struct is_qt_string : std::false_type {};
template<>
struct is_qt_string<QString> : std::true_type {};
template<typename T>
inline constexpr bool is_qt_string_v = is_qt_string<T>::value;

template<typename T>
inline constexpr bool is_string_v = is_basic_string_v<T> || is_qt_string_v<T>;

// fails for e.g. std::queue because it doesn't have iterators
template<typename T, typename = void>
struct is_container : std::false_type {};
template<typename T>
struct is_container<T, std::void_t<
    decltype(std::declval<T>().size()),
    decltype(std::begin(std::declval<T>())),
    decltype(std::end(  std::declval<T>()))
>> : std::bool_constant<!is_basic_string_v<T>> {};
template<typename T>
inline constexpr bool is_container_v = is_container<T>::value;

template<typename T, typename = void>
struct is_associative_container : std::false_type {};
template<typename T>
struct is_associative_container<T, std::void_t<
    typename T::key_type,
    typename T::mapped_type
>> : std::bool_constant<is_container_v<T>> {};
template<typename T>
inline constexpr bool is_associative_container_v = is_associative_container<T>::value;

template<typename T>
struct is_non_associative_container : std::integral_constant<bool,
    is_container_v<T>
    && !is_associative_container_v<T
>> {};
template<typename T>
inline constexpr bool is_non_associative_container_v = is_non_associative_container<T>::value;


inline quint64 hash64_FNV1a(const QByteArray& data)
{
    constexpr quint64 FNV_offset = 1'469'598'103'934'665'603ull;
    constexpr quint64 FNV_prime = 1'099'511'628'211ull;
    quint64 hash = FNV_offset;
    for (auto byte : data)
    {
        hash ^= static_cast<quint8>(byte);
        hash *= FNV_prime;
    }
    return hash;

    QString s;
}

class TString : public std::string {};
static_assert(is_associative_container_v<QHash<int, int>>);
static_assert(is_associative_container_v<std::unordered_map<int, int>>);
static_assert(is_associative_container_v<std::map<int, int>>);
static_assert(is_associative_container_v<QMap<int, int>>);
static_assert(is_non_associative_container_v<QString>);
static_assert(!is_non_associative_container_v<int>);
static_assert(is_container_v<std::vector<int>>);
static_assert(is_container_v<QVector<int>>);
static_assert(is_container_v<std::map<int, int>>);
static_assert(is_container_v<QMap<int, int>>);
static_assert(!is_container_v<int>);
static_assert(is_basic_string_v<std::string>);
static_assert(is_basic_string_v<TString>);
static_assert(!is_container_v<std::string>);

template<typename T>
bool parseJsonVar(const QJsonObject& curObject, const QString& curValueName, T& targetVar, QString* errorText = nullptr)
{
    QJsonValue curValue = curObject.value(curValueName);
    if (curValue.isUndefined())
    {
        if (errorText)
            *errorText = QStringLiteral("\"%1\" missing field").arg(curValueName);
        return false;
    }

    if constexpr(is_non_associative_container_v<T>)
    {
        QJsonArray arr = curValue.toArray();
        targetVar.resize(arr.size());
        using item_t = typename T::value_type;
        for (int i = 0; i < arr.size(); ++i)
        {
            QVariant var = arr.at(i).toVariant();
            if (!var.canConvert<item_t>())
            {
                if (errorText)
                    *errorText = QStringLiteral("\"%1\" element at index %2 wrong type").arg(curValueName).arg(i);
                return false;
            }
            targetVar[i] = var.value<item_t>();
        }
        return true;
    }
    else
    {
        QVariant var = curValue.toVariant();
        if (!var.canConvert<T>())
        {
            if (errorText)
                *errorText = QStringLiteral("\"%1\" wrong type").arg(curValueName);
            return false;
        }
        targetVar = var.value<T>();
        return true;
    }
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
