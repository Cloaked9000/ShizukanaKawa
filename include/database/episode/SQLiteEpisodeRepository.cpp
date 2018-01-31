//
// Created by fred on 21/01/18.
//

#include "SQLiteEpisodeRepository.h"

SQLiteEpisodeRepository::SQLiteEpisodeRepository(std::shared_ptr<SQLite3DB> database_)
: database(std::move(database_))
{
    //Create table and indexes
    database->unsafe_query("CREATE TABLE IF NOT EXISTS episode(id INTEGER PRIMARY KEY AUTOINCREMENT, season_id INTEGER NOT NULL, filepath VARCHAR(4096) NOT NULL, name VARCHAR(4096) NOT NULL, watched BOOLEAN NOT NULL, watch_offset INTEGER NOT NULL, audio_track INTEGER NOT NULL, sub_track INTEGER NOT NULL, FOREIGN KEY(season_id) REFERENCES season(id));");
    database->unsafe_query("CREATE INDEX IF NOT EXISTS episode_filepath_index ON episode(filepath);");
}

uint64_t SQLiteEpisodeRepository::database_create(EpisodeEntry *entry)
{
    return database->insert_query("INSERT INTO episode VALUES(NULL, ?, ?, ?, ?, ?, ?, ?)",
                                  {entry->season_id, entry->filepath, entry->name, entry->watched, entry->watch_offset, entry->audio_track, entry->sub_track});
}

std::shared_ptr<EpisodeEntry> SQLiteEpisodeRepository::database_load(uint64_t entry_id)
{
    SQLite3DB::query_t results = database->query("SELECT * FROM episode WHERE id=?", {entry_id});

    return std::make_shared<EpisodeEntry>(entry_id,
                                         results.at("season_id").at(0).get<uint64_t>(),
                                         results.at("filepath").at(0).get<std::string>(),
                                         results.at("name").at(0).get<std::string>(),
                                         results.at("watched").at(0).get<bool>(),
                                         results.at("watch_offset").at(0).get<uint64_t>(),
                                         results.at("audio_track").at(0).get<int64_t>(),
                                         results.at("sub_track").at(0).get<int64_t>());
}

void SQLiteEpisodeRepository::database_update(std::shared_ptr<EpisodeEntry> entry)
{
    database->query("UPDATE episode SET season_id=?, filepath=?, name=?, watched=?, watch_offset=?, audio_track=?, sub_track=? WHERE id=?",
                    {entry->season_id, entry->filepath, entry->name, entry->watched, entry->watch_offset, entry->audio_track, entry->sub_track, entry->id});
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

void SQLiteEpisodeRepository::for_each_episode_in_season(uint64_t season_id, const std::function<bool(std::shared_ptr<EpisodeEntry>)> &callback)
{
    std::vector<DBType> *id_column = nullptr;
    database->query("SELECT id FROM episode WHERE season_id=? ORDER BY UPPER(name)", {season_id}, 100, [&](SQLite3DB::query_t &query) -> bool{
        if(id_column == nullptr)
            id_column = &query.at("id");

        for(size_t a = 0; a < id_column->size(); ++a)
        {
            auto entry = load((*id_column)[a].get<uint64_t>());
            if(!callback(entry))
                return false;
        }

        return true;
    });
}