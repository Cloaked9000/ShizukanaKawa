//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_WATCHHISTORYENTRY_H
#define SFTPMEDIASTREAMER_WATCHHISTORYENTRY_H


#include <cstdint>
#include <ctime>
#include <database/DatabaseRepository.h>

class WatchHistoryEntry
{
public:
    WatchHistoryEntry(uint64_t id_, uint64_t episode_id_, uint64_t time_)
    : id(id_),
      episode_id(episode_id_),
      date(time_)
    {}

    WatchHistoryEntry()
    : WatchHistoryEntry(0, 0, 0)
    {}

    WatchHistoryEntry(WatchHistoryEntry &&o)
    : id(o.id),
      episode_id(o.episode_id),
      date(o.date)
    {}

    db_define_dirty()
    db_entry_def(uint64_t, id)
    db_entry_def(uint64_t, episode_id)
    db_entry_def(time_t, date)
};


#endif //SFTPMEDIASTREAMER_WATCHHISTORYENTRY_H
