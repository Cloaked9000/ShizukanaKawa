//
// Created by fred on 14/04/18.
//

#include "database/SQLiteMiscRepository.h"

SQLiteMiscRepository::SQLiteMiscRepository(std::shared_ptr<SQLite3DB> database_, std::shared_ptr<SeasonRepository> season_table_)
: database(std::move(database_)),
  season_table(std::move(season_table_))
{

}

void SQLiteMiscRepository::for_each_recently_added(const std::function<bool(std::shared_ptr<SeasonEntry>)> callback)
{
    auto stmt = database->compile_statement("SELECT id, date_added FROM season UNION ALL SELECT season_id as id, date_added FROM episode ORDER BY date_added DESC;");
    auto results = database->query(stmt, {});
    auto &col = results.at("id");
    for(auto id : col)
    {
        auto entry = season_table->load(id);
        if(!callback(entry))
            break;
    }
}

void SQLiteMiscRepository::for_each_recently_watched(const std::function<bool(std::shared_ptr<SeasonEntry>)>callback)
{
    auto stmt = database->compile_statement("SELECT season_id FROM watch_history INNER JOIN episode ON watch_history.episode_id = episode.id GROUP BY season_id ORDER BY watch_history.id DESC;");
    auto results = database->query(stmt, {});
    auto &col = results.at("season_id");
    for(auto id : col)
    {
        auto entry = season_table->load(id);
        if(!callback(entry))
            break;
    }
}
