//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_SEASONENTRY_H
#define SFTPMEDIASTREAMER_SEASONENTRY_H


#include <cstdint>
#include <string>
#include <utility>

class SeasonEntry
{
public:
    SeasonEntry(uint64_t id_, std::string filepath_, std::string name_, std::string thumbnail_, time_t date_added_)
    : id(id_),
      filepath(std::move(filepath_)),
      name(std::move(name_)),
      thumbnail(std::move(thumbnail_)),
      date_added(date_added_)
    {}

    SeasonEntry()
    : SeasonEntry(0, "", "", "", 0)
    {}

    uint64_t id;
    std::string filepath;
    std::string name;
    std::string thumbnail;
    time_t date_added;
};


#endif //SFTPMEDIASTREAMER_SEASONENTRY_H
