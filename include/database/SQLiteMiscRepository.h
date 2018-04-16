//
// Created by fred on 14/04/18.
//

#ifndef SFTPMEDIASTREAMER_SQLITEMISCREPOSITORY_H
#define SFTPMEDIASTREAMER_SQLITEMISCREPOSITORY_H


#include <database/season/SeasonRepository.h>
#include "MiscRepository.h"
#include "SQLite3DB.h"

class SQLiteMiscRepository : public MiscRepository
{
public:
    explicit SQLiteMiscRepository(std::shared_ptr<SQLite3DB> database_, std::shared_ptr<SeasonRepository> season_table);
    ~SQLiteMiscRepository() final = default;


    /*!
     * Iterate through seasons which have been recently added, and
     * seasons which have recently had new episodes added.
     *
     * @param callback The callback to call for each entry. Should return true if it wants more
     * entries, false if it has had enough.
     */
    void for_each_recently_added(const std::function<bool(std::shared_ptr<SeasonEntry>)> &callback) final;

    /*!
     * Iterate through seasons which have been recently watched.
     *
     * @param callback The callback to call for each entry. Should return true if it wants more
     * entries, false if it has had enough.
     */
    void for_each_recently_watched(const std::function<bool(std::shared_ptr<SeasonEntry>)> &callback) final;

private:

    std::shared_ptr<SQLite3DB> database;
    std::shared_ptr<SeasonRepository> season_table;
};


#endif //SFTPMEDIASTREAMER_SQLITEMISCREPOSITORY_H
