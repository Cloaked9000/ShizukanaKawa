//
// Created by fred on 21/01/18.
//

#include "SQLiteEpisodeRepository.h"
#define column_names std::array<std::string, 9>{"id", "season_id", "filepath", "name", "watched", "watch_offset", "audio_track", "sub_track", "date_added"}

SQLiteEpisodeRepository::SQLiteEpisodeRepository(std::shared_ptr<SQLite3DB> database_)
: database(std::move(database_))
{
    //Create table and indexes
    database->unsafe_query("CREATE TABLE IF NOT EXISTS episode(id INTEGER PRIMARY KEY AUTOINCREMENT, season_id INTEGER NOT NULL, filepath VARCHAR(4096) NOT NULL, name VARCHAR(4096) NOT NULL, watched BOOLEAN NOT NULL, watch_offset INTEGER NOT NULL, audio_track INTEGER NOT NULL, sub_track INTEGER NOT NULL, date_added DATE INTEGER NOT NULL, FOREIGN KEY(season_id) REFERENCES season(id));");
    database->unsafe_query("CREATE INDEX IF NOT EXISTS episode_filepath_index ON episode(filepath);");
}

uint64_t SQLiteEpisodeRepository::database_create(EpisodeEntry *entry)
{
    return database->insert_query("INSERT INTO episode VALUES(NULL, ?, ?, ?, ?, ?, ?, ?, ?)",
                                  {entry->get_season_id(), entry->get_filepath(), entry->get_name(), entry->get_watched(), entry->get_watch_offset(), entry->get_audio_track(), entry->get_sub_track(), entry->get_date_added()});
}

std::shared_ptr<EpisodeEntry> SQLiteEpisodeRepository::database_load(uint64_t entry_id)
{
    SQLite3DB::query_t results = database->query("SELECT * FROM episode WHERE id=?", {entry_id});

    return std::make_shared<EpisodeEntry>(entry_id,
                                         results.at("season_id").at(0).get<uint64_t>(),
                                         results.at("filepath").at(0).get<std::string>(),
                                         results.at("name").at(0).get<std::string>(),
                                         results.at("watched").at(0).get<uint64_t>(),
                                         results.at("watch_offset").at(0).get<uint64_t>(),
                                         results.at("audio_track").at(0).get<int64_t>(),
                                         results.at("sub_track").at(0).get<int64_t>(),
                                         results.at("date_added").at(0).get<time_t>());
}

void SQLiteEpisodeRepository::database_update(std::shared_ptr<EpisodeEntry> entry)
{
    database->query("UPDATE episode SET season_id=?, filepath=?, name=?, watched=?, watch_offset=?, audio_track=?, sub_track=?, date_added=? WHERE id=?",
                    {entry->get_season_id(), entry->get_filepath(), entry->get_name(), entry->get_watched(), entry->get_watch_offset(), entry->get_audio_track(), entry->get_sub_track(), entry->get_date_added(), entry->get_id()});
}

void SQLiteEpisodeRepository::database_erase(uint64_t entry_id)
{
    database->query("DELETE FROM episode WHERE id=?", {entry_id});
}

uint64_t SQLiteEpisodeRepository::get_episode_id_from_filepath(const std::string &episode_filepath)
{
    SQLite3DB::query_t query = database->query("SELECT id FROM episode WHERE filepath=?", {episode_filepath});
    auto &iter = query.at("id");
    if(iter.empty())
        return NO_SUCH_ENTRY;
    return iter.at(0).get<uint64_t>();
}

void SQLiteEpisodeRepository::for_each_episode_in_season(uint64_t season_id, const std::function<bool(std::shared_ptr<EpisodeEntry>)> callback)
{
    auto stmt = database->compile_statement("SELECT * FROM episode WHERE season_id=? ORDER BY UPPER(name)");
    database->for_each<EpisodeEntry>(stmt, {season_id}, column_names, [&](std::shared_ptr<EpisodeEntry> obj) {
        return callback(store_cache(obj));
    });
}