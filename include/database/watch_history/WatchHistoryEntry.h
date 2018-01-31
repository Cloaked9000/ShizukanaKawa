//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_WATCHHISTORYENTRY_H
#define SFTPMEDIASTREAMER_WATCHHISTORYENTRY_H


#include <cstdint>
#include <ctime>

class WatchHistoryEntry
{
public:
    WatchHistoryEntry(uint64_t id_, uint64_t episode_id_, time_t time_)
    : id(id_),
      episode_id(episode_id_),
      date(time_)
    {}

    WatchHistoryEntry()
    : WatchHistoryEntry(0, 0, 0)
    {}

    uint64_t id;
    uint64_t episode_id;
    time_t  date;
};


#endif //SFTPMEDIASTREAMER_WATCHHISTORYENTRY_H
