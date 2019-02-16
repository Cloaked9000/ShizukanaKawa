//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_SEASONENTRY_H
#define SFTPMEDIASTREAMER_SEASONENTRY_H


#include <cstdint>
#include <string>
#include <utility>
#include <atomic>

#include "database/DatabaseRepository.h"

class SeasonEntry
{
public:
    SeasonEntry(uint64_t id_, std::string filepath_, std::string name_, std::string thumbnail_, uint64_t date_added_)
    : id(id_),
      filepath(std::move(filepath_)),
      name(std::move(name_)),
      thumbnail(std::move(thumbnail_)),
      date_added(date_added_)
    {}

    SeasonEntry(SeasonEntry&&o)
    : id(o.id),
      filepath(std::move(o.filepath)),
      name(std::move(o.name)),
      thumbnail(std::move(o.thumbnail)),
      date_added(o.date_added)
    {

    }

    SeasonEntry()
    : SeasonEntry(0, "", "", "", 0)
    {}

    db_define_dirty()
    db_entry_def(uint64_t, id)
    db_entry_def(std::string, filepath)
    db_entry_def(std::string, name)
    db_entry_def(std::string, thumbnail)
    db_entry_def(time_t, date_added)
};


#endif //SFTPMEDIASTREAMER_SEASONENTRY_H
