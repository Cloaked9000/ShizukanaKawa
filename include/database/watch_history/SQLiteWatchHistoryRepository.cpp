//
// Created by fred on 21/01/18.
//

#include "SQLiteWatchHistoryRepository.h"
#define column_names std::array<std::string, 3>{"id", "episode_id", "date"}

SQLiteWatchHistoryRepository::SQLiteWatchHistoryRepository(std::shared_ptr<SQLite3DB> database_)
: database(std::move(database_))
{
    //Create table
    database->unsafe_query("CREATE TABLE IF NOT EXISTS watch_history(id INTEGER PRIMARY KEY AUTOINCREMENT, episode_id INTEGER NOT NULL, date INTEGER NOT NULL, FOREIGN KEY(episode_id) REFERENCES episode(id));");
}

uint64_t SQLiteWatchHistoryRepository::database_create(WatchHistoryEntry *entry)
{
    return database->insert_query("INSERT INTO watch_history VALUES(NULL, ?, ?)",
                                  {entry->get_episode_id(), entry->get_date()});
}

std::shared_ptr<WatchHistoryEntry> SQLiteWatchHistoryRepository::database_load(uint64_t entry_id)
{
    SQLite3DB::query_t results = database->query("SELECT * FROM watch_history WHERE id=?", {entry_id});

    return std::make_shared<WatchHistoryEntry>(entry_id,
                                               results.at("episode_id").at(0).get<uint64_t>(),
                                               results.at("date").at(0).get<time_t>());
}

void SQLiteWatchHistoryRepository::database_update(std::shared_ptr<WatchHistoryEntry> entry)
{
    database->query("UPDATE watch_history SET episode_id=?, date=? WHERE id=?",
                    {entry->get_episode_id(), entry->get_date(), entry->get_id()});
}

void SQLiteWatchHistoryRepository::database_erase(uint64_t entry_id)
{
    database->query("DELETE FROM watch_history WHERE id=?", {entry_id});
}

void SQLiteWatchHistoryRepository::for_each_entry(bool unique, const std::function<bool(std::shared_ptr<WatchHistoryEntry>)> callback)
{
    const static std::string unique_query = "SELECT * FROM watch_history GROUP BY episode_id ORDER BY id DESC";
    const static std::string non_unique_query = "SELECT id FROM watch_history ORDER BY id DESC";

    auto stmt = database->compile_statement(unique ? unique_query : non_unique_query);
    database->for_each<WatchHistoryEntry>(stmt, {}, column_names, [&](std::shared_ptr<WatchHistoryEntry> obj) {
        return callback(store_cache(obj));
    });
}

void SQLiteWatchHistoryRepository::erase_for_episode(uint64_t episode_id)
{
    database->query("DELETE FROM watch_history WHERE episode_id=?", {episode_id});
}
