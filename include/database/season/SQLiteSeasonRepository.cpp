//
// Created by fred on 21/01/18.
//

#include "SQLiteSeasonRepository.h"

SQLiteSeasonRepository::SQLiteSeasonRepository(std::shared_ptr<SQLite3DB> database_)
: database(std::move(database_))
{
    //Create table and indexes
    database->unsafe_query("CREATE TABLE IF NOT EXISTS season(id INTEGER PRIMARY KEY AUTOINCREMENT, filepath VARCHAR(4096) NOT NULL, name VARCHAR(4096) NOT NULL, thumbnail BLOB NOT NULL);");
    database->unsafe_query("CREATE INDEX IF NOT EXISTS season_filepath_index ON season(filepath);");
}

uint64_t SQLiteSeasonRepository::database_create(SeasonEntry *entry)
{
    return database->insert_query("INSERT INTO season VALUES(NULL, ?, ?, ?)",
                                  {entry->filepath, entry->name, DBType(false, 0, entry->thumbnail, DBType::BLOB)});
}

std::shared_ptr<SeasonEntry> SQLiteSeasonRepository::database_load(uint64_t entry_id)
{
    SQLite3DB::query_t results = database->query("SELECT * FROM season WHERE id=?", {entry_id});

    return std::make_shared<SeasonEntry>(entry_id,
                                         results.at("filepath").at(0).get<std::string>(),
                                         results.at("name").at(0).get<std::string>(),
                                         results.at("thumbnail").at(0).get<std::string>());
}

void SQLiteSeasonRepository::database_update(std::shared_ptr<SeasonEntry> entry)
{
    database->query("UPDATE season SET filepath=?, name=?, thumbnail=? WHERE id=?",
                    {entry->filepath, entry->name, entry->thumbnail, entry->id});
}

void SQLiteSeasonRepository::database_erase(uint64_t entry_id)
{
    database->query("DELETE FROM season WHERE id=?", {entry_id});
}

void SQLiteSeasonRepository::for_each_season(const std::function<bool(std::shared_ptr<SeasonEntry> )> &callback)
{
    std::vector<DBType> *id_column = nullptr;
    database->query("SELECT id FROM season ORDER BY UPPER(name)", {}, 100, [&](SQLite3DB::query_t &query) -> bool{
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

uint64_t SQLiteSeasonRepository::get_season_id_from_filepath(const std::string &season_filepath)
{
    SQLite3DB::query_t query = database->query("SELECT id FROM season WHERE filepath=?", {season_filepath});
    auto &iter = query.at("id");
    if(iter.empty())
        return NO_SUCH_ENTRY;
    return iter.at(0).get<uint64_t>();
}
