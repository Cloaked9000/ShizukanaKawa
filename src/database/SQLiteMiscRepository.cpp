//
// Created by fred on 14/04/18.
//

#include "database/SQLiteMiscRepository.h"

SQLiteMiscRepository::SQLiteMiscRepository(std::shared_ptr<SQLite3DB> database_, std::shared_ptr<SeasonRepository> season_table_)
: database(std::move(database_)),
  season_table(std::move(season_table_))
{

}

void SQLiteMiscRepository::for_each_recently_added(const std::function<bool(std::shared_ptr<SeasonEntry>)> &callback)
{
    std::vector<DBType> *id_column = nullptr;
    database->query("SELECT id, date_added FROM season UNION ALL SELECT season_id as id, date_added FROM episode ORDER BY date_added DESC;", {}, 100, [&](SQLite3DB::query_t &query) -> bool{
        if(id_column == nullptr)
            id_column = &query.at("id");

        for(size_t a = 0; a < id_column->size(); ++a)
        {
            auto entry = season_table->load((*id_column)[a].get<uint64_t>());
            if(!callback(entry))
                return false;
        }

        return true;
    });
}

void SQLiteMiscRepository::for_each_recently_watched(const std::function<bool(std::shared_ptr<SeasonEntry>)> &callback)
{
    std::vector<DBType> *id_column = nullptr;
    database->query("SELECT season_id FROM watch_history INNER JOIN episode ON watch_history.episode_id = episode.id GROUP BY season_id ORDER BY watch_history.id DESC;", {}, 100, [&](SQLite3DB::query_t &query) -> bool{
        if(id_column == nullptr)
            id_column = &query.at("season_id");

        for(size_t a = 0; a < id_column->size(); ++a)
        {
            auto entry = season_table->load((*id_column)[a].get<uint64_t>());
            if(!callback(entry))
                return false;
        }

        return true;
    });
}
