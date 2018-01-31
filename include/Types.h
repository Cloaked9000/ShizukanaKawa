//
// Created by fred on 09/12/17.
//

#ifndef SFTPMEDIASTREAMER_TYPES_H
#define SFTPMEDIASTREAMER_TYPES_H
#include <string>
#include <cxxabi.h>

struct Attributes
{
    enum Type
    {
        Regular = 1,
        Directory = 2,
        Symlink = 3,
        Special = 4,
        Unknown = 5,
        TypeCount = 6 //Keep me at the end and updated
    };

    std::string name;
    std::string full_name;
    size_t size;
    Type type;

    explicit operator std::string() const
    {
        return full_name;
    }
};

//Duration constants in seconds so there's no magic '3600's in the code.
enum Duration
{
    None = 0,
    Second = 1,
    Minute = 60,
    Hour = 3600,
    Day = 86400,
};

#ifdef __GNUC__
static __attribute__ ((unused)) std::string demangle(const char *name)
{
    char *real = abi::__cxa_demangle(name, nullptr, nullptr, nullptr);
    std::string ret(real);
    free(real);
    return ret;
}
#else
static std::string demangle(const char *name)
{
    return "Demangling not supported on this platform. Mangled name: " + std::string(name);
}
#endif

#define DEMANGLE(x) demangle(typeid(x).name())
#define WINDOW_TITLE "ShizukanaKawa"
#define NO_SUCH_ENTRY 0
#define THUMBNAIL_WIDTH 256
#define THUMBNAIL_HEIGHT 144
#define SUB_TRACK_UNSET (-2)
#define AUDIO_TRACK_UNSET (-2)
#define RIGHT_CLICK 3

#endif //SFTPMEDIASTREAMER_TYPES_H
