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

//Simple placeholder until something more suitable like std::any is properly available
class DBType
{
public:
    enum Type
    {
        INTEGRAL = 0,
        FLOATING_POINT = 1,
        STRING = 2,
        BLOB = 3,
    };

    DBType(bool is_type_null_, uint64_t val_int_, std::string val_str_, Type type_)
    : is_type_null(is_type_null_),
      val_int(val_int_),
      val_str(std::move(val_str_)),
      type(type_)
    {

    }

    DBType()
    : DBType(false, 0, "", Type::INTEGRAL)
    {

    }

    DBType(std::string str)
    : DBType()
    {
        set(std::move(str));
    }

    template<typename T>
    DBType(T integral)
            : DBType()
    {
        set(integral);
    }

    template<typename T>
    DBType(const std::atomic<T> &var)
            : DBType()
    {
        *this = var.load();
    }

    DBType(const DBType &other)
            : DBType(other.is_type_null, other.val_int, other.val_str, other.type)
    {

    }


    inline void set(std::nullptr_t)
    {
        is_type_null = true;
    }

    inline void set(std::string val)
    {
        type = STRING;
        val_str = std::move(val);
        if(val_str == "NULL")
            is_type_null = true;
    }

    template<typename T = uint64_t, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    inline void set(T val)
    {
        type = INTEGRAL;
        val_int = val;
    }

    template<typename T = double, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    inline void set(T val)
    {
        type = FLOATING_POINT;
        val_double = val;
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
    inline void set(T val)
    {
        type = INTEGRAL;
        val_int = static_cast<typename std::underlying_type<T>::type>(val);
    }


    inline Type get_type() const
    {
        return type;
    }

    inline const bool &is_null() const
    {
        return is_type_null;
    }

    template<typename T>
    T get() const;

    template<typename T>
    explicit operator T()
    {
        return get<T>();
    }


    template<typename T>
    bool operator==(const T &other)
    {
        return other == get<T>();
    }


    inline const std::string &get_buffer() const
    {
        return val_str;
    }

    inline const uint64_t &get_integral() const
    {
        return val_int;
    }

    inline const double &get_floating() const
    {
        return val_double;
    }
private:
    bool is_type_null;
    union
    {
        uint64_t val_int;
        double val_double;
    };
    std::string val_str;
    Type type;
};

template<>
inline std::string DBType::get<std::string>() const
{
    return val_str;
}

template<typename T>
inline T DBType::get() const
{
    if(type == INTEGRAL)
        return static_cast<T>(val_int);
    return static_cast<T>(val_double);
}


#endif //SFTPMEDIASTREAMER_DBTYPE_H
