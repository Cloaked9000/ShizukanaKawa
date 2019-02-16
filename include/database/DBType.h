//
// Created by fred on 21/12/17.
//

#ifndef SFTPMEDIASTREAMER_DBTYPE_H
#define SFTPMEDIASTREAMER_DBTYPE_H


#include <cstdio>
#include <experimental/any>
#include <cstdint>
#include <string>
#include <cstring>
#include <memory>
#include <atomic>
#include <iostream>

class DBType
{
public:
    enum Type
    {
        UNSINGED_INTEGRAL = 0,
        SIGNED_INTEGRAL = 1,
        FLOATING_POINT = 2,
        STRING = 3,
        NILL = 4,
        UNIX_TIME = 5,
        BLOB = 6,
        TYPE_COUNT = 7, //Keep me at the end and updated
    };

    /*!
     * Default constructor. Default type is NILL.
     */
    DBType()
    : type(NILL)
    {

    }

    /*!
     * Construct from a value
     *
     * @tparam T Type of data to store
     * @param val The data to store
     */
    template<typename T>
    DBType(T val)
    : DBType()
    {
        set(std::move(val));
    }

    DBType(std::string str, Type type_)
    : DBType()
    {
        set(std::move(str));
        type = type_;
    }

    /*!
     * Construct from an std::atomic<T> type.
     * (Value will be loaded from atomic type)
     *
     * @tparam T Type of data to store
     * @param val The data to store
     */
    template<typename T>
    DBType(const std::atomic<T> &val)
            : DBType()
    {
        set(std::move(val.load()));
    }

    /*!
     * Gets the data contained within the type
     *
     * @tparam T The type to extract the data as
     * @return The data
     */
    template<typename T>
    inline const T &get() const noexcept
    {
        static_assert(
                std::is_same<T, uint64_t>() || std::is_same<T, double>() || std::is_same<T, std::string>() || std::is_same<T, int64_t>(),
                "Must be uint64_t/double/std::string");
        return std::any_cast<const T&>(data);
    }

    template<typename T>
    inline T &get_raw() noexcept
    {
        static_assert(
                std::is_same<T, uint64_t>() || std::is_same<T, double>() || std::is_same<T, std::string>() || std::is_same<T, int64_t>(),
                "Must be uint64_t/double/std::string");
        return std::any_cast<T&>(data);
    }

    template<typename T>
    inline T &&get_move() noexcept
    {
        static_assert(
                std::is_same<T, uint64_t>() || std::is_same<T, double>() || std::is_same<T, std::string>() || std::is_same<T, int64_t>(),
                "Must be uint64_t/double/std::string");
        return std::move(std::any_cast<std::string&>(data));
    }

    /*!
     * Overload for getting the value when the requested type is known
     *
     * @tparam T The type to get the value as
     * @return The stored value
     */
    template<typename T>
    operator const T&() const noexcept
    {
        return get<T>();
    }
    template<typename T>
    operator T&() noexcept
    {
        return get_raw<T>();
    }
    operator std::string&&() noexcept //Hackish, ensures that move constructor is used if another object is constructed from a DBType containing an std::string
    {
        return get_move<std::string>();
    }


    /*!
     * Sets the value of the DBType
     *
     * @tparam T The type of the value being set
     * @param val The value itself to store
     */
    template<typename T>
    inline void set(T val)
    {
        (void)val;
        //Store data
        if constexpr(std::is_null_pointer<T>())
        {
            type = NILL;
            data = (uint64_t)0;
        }
        else if constexpr (std::is_integral<T>() && std::is_unsigned<T>())
        {
            data = static_cast<uint64_t>(val);
            type = UNSINGED_INTEGRAL;
        }
        else if constexpr (std::is_integral<T>() && std::is_signed<T>())
        {
            data = static_cast<int64_t>(val);
            type = SIGNED_INTEGRAL;
        }
        else if constexpr (std::is_enum<T>())
        {
            data = (uint64_t)static_cast<typename std::underlying_type<T>::type>(val);
            type = UNSINGED_INTEGRAL;
        }
        else if constexpr (std::is_floating_point<T>())
        {
            data = static_cast<double>(val);
            type = FLOATING_POINT;
        }
        else if constexpr (std::is_same<T, std::string>())
        {
            data = std::move(val);
            type = STRING;
        }
        else if constexpr (std::is_same<std::remove_const<T>(), char*>())
        {
            data = std::string(val);
            type = STRING;
        }
        else if constexpr (std::is_same<T, const char*>())
        {
            data = std::string(val);
            type = STRING;
        }
        else if constexpr (std::is_same<T, struct tm>())
        {
            data = timegm(&val);
            type = UNIX_TIME;
        }
        else
        {
            char *real = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
            std::string ret(real);
            free(real);
            throw std::logic_error("Unknown type: " + ret);
        }

    }

    /*!
     * Gets the type of the DBType.
     *
     * INTEGRAL       = integer
     * FLOATING_POINT = float/double
     * STRING         = std::string
     * NILL           = null value
     * @return The type being stored
     */
    inline Type get_type() const
    {
        return type;
    }

private:

    std::any data;
    Type type;
};



#endif //SFTPMEDIASTREAMER_DBTYPE_H
