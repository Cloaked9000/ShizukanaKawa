//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_EPISODEENTRY_H
#define SFTPMEDIASTREAMER_EPISODEENTRY_H


#include <cstdint>
#include <string>
#include <utility>
#include <database/DatabaseRepository.h>

class EpisodeEntry
{
public:
    EpisodeEntry(uint64_t id_, uint64_t season_id_, std::string filepath_, std::string name_, uint64_t watched_, uint64_t watch_offset_, uint64_t audio_track_, uint64_t sub_track_, uint64_t date_added_)
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

    EpisodeEntry(EpisodeEntry &&o)
    : id(o.id),
      season_id(o.season_id),
      filepath(std::move(o.filepath)),
      name(std::move(o.name)),
      watched(o.watched),
      watch_offset(o.watch_offset),
      audio_track(o.audio_track),
      sub_track(o.sub_track),
      date_added(o.date_added)
    {

    }

    db_define_dirty()
    db_entry_def(uint64_t, id)
    db_entry_def(uint64_t, season_id)
    db_entry_def(std::string, filepath)
    db_entry_def(std::string, name)
    db_entry_def(bool, watched)
    db_entry_def(uint64_t, watch_offset)
    db_entry_def(int64_t, audio_track)
    db_entry_def(int64_t, sub_track)
    db_entry_def(time_t, date_added)
};


#endif //SFTPMEDIASTREAMER_EPISODEENTRY_H
