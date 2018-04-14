//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_EPISODEENTRY_H
#define SFTPMEDIASTREAMER_EPISODEENTRY_H


#include <cstdint>
#include <string>
#include <utility>

class EpisodeEntry
{
public:
    EpisodeEntry(uint64_t id_, uint64_t season_id_, std::string filepath_, std::string name_, bool watched_, uint64_t watch_offset_, int64_t audio_track_, int64_t sub_track_, time_t date_added_)
    : id(id_),
      season_id(season_id_),
      filepath(std::move(filepath_)),
      name(std::move(name_)),
      watched(watched_),
      watch_offset(watch_offset_),
      audio_track(audio_track_),
      sub_track(sub_track_),
      date_added(date_added_)
    {}

    EpisodeEntry()
    : EpisodeEntry(0, 0, "", "", false, 0, 0, 0, 0)
    {}

    uint64_t id;
    uint64_t season_id;
    std::string filepath;
    std::string name;
    bool watched;
    uint64_t watch_offset;
    int64_t audio_track;
    int64_t sub_track;
    time_t date_added;
};


#endif //SFTPMEDIASTREAMER_EPISODEENTRY_H
