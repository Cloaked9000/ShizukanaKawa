//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_SQLITESEASONREPOSITORY_H
#define SFTPMEDIASTREAMER_SQLITESEASONREPOSITORY_H


#include <database/SQLite3DB.h>
#include <functional>
#include "SeasonRepository.h"

class SQLiteSeasonRepository : public SeasonRepository
{
public:
    explicit SQLiteSeasonRepository(std::shared_ptr<SQLite3DB> database_);
    ~SQLiteSeasonRepository() override {flush();};

    /*!
     * Creates a new entry and saves it to the database
     *
     * @throws An std::logic_error on failure
     * @returns The ID of the newly created object
     */
    uint64_t database_create(SeasonEntry *entry) override;

    /*!
     * Loads an existing entry from the database
     *
     * @throws An std::logic_error on failure.
     * @param entry_id The ID of the entry to load
     */
    std::shared_ptr<SeasonEntry> database_load(uint64_t entry_id) override;

    /*!
     * Updates the entry if it's already
     * an existing entry in the database
     *
     * @throws An std::logic_error on failure
     */
    void database_update(std::shared_ptr<SeasonEntry> entry) override;

    /*!
     * Removes an entry from the database
     *
     * @param entry_id The ID of the entry
     */
    void database_erase(uint64_t entry_id) override;

    /*!
     * Iterates through every row in the table, calling a callback
     * for each entry.
     *
     * @param callback The callback to call for each row. Should return true
     * if more rows are wanted, false if it's finished.
     */
    void for_each_season(const std::function<bool(std::shared_ptr<SeasonEntry> )> callback) override;

    /*!
     * Tries to get the ID of a season from its filepath
     *
     * @param season_filepath The filepath of the season to get the ID of
     * @return A season ID on success, NO_SUCH_ENTRY on failure.
     */
    uint64_t get_season_id_from_filepath(const std::string &season_filepath) override;

private:
    std::shared_ptr<SQLite3DB> database;
};


#endif //SFTPMEDIASTREAMER_SQLITESEASONREPOSITORY_H
