//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_SQLITEWATCHHISTORYREPOSITORY_H
#define SFTPMEDIASTREAMER_SQLITEWATCHHISTORYREPOSITORY_H


#include <database/SQLite3DB.h>
#include "WatchHistoryRepository.h"

class SQLiteWatchHistoryRepository : public WatchHistoryRepository
{
public:
    explicit SQLiteWatchHistoryRepository(std::shared_ptr<SQLite3DB> database_);
    ~SQLiteWatchHistoryRepository() override {flush();};

    /*!
     * Creates a new entry and saves it to the database
     *
     * @throws An std::logic_error on failure
     * @returns The ID of the newly created object
     */
    uint64_t database_create(WatchHistoryEntry *entry) override;

    /*!
     * Loads an existing entry from the database
     *
     * @throws An std::logic_error on failure.
     * @param entry_id The ID of the entry to load
     */
    std::shared_ptr<WatchHistoryEntry> database_load(uint64_t entry_id) override;

    /*!
     * Updates the entry if it's already
     * an existing entry in the database
     *
     * @throws An std::logic_error on failure
     */
    void database_update(std::shared_ptr<WatchHistoryEntry> entry) override;

    /*!
     * Removes an entry from the database
     *
     * @param entry_id The ID of the entry
     */
    void database_erase(uint64_t entry_id) override;

    /*!
     * Iterates through each unique watch history entry,
     * in the order of most recent to least recent.
     * @param unique True if duplicate seasons should be removed. False otherwise.
     * @param callback The functor to call for each unique watch history entry.
     * It should return true if it wants more entries, or false if it has had enough.
     */
    void for_each_entry(bool unique, const std::function<bool(std::shared_ptr<WatchHistoryEntry>)> &callback) override;

    /*!
     * Erases watch history for a given episode ID
     *
     * @param episode_id The ID of the episode to delete history for
     */
    void erase_for_episode(uint64_t episode_id) override;

private:
    std::shared_ptr<SQLite3DB> database;
};


#endif //SFTPMEDIASTREAMER_SQLITEWATCHHISTORYREPOSITORY_H
