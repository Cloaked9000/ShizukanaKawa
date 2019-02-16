//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_SQLITEEPISODEREPOSITORY_H
#define SFTPMEDIASTREAMER_SQLITEEPISODEREPOSITORY_H


#include <database/SQLite3DB.h>
#include "EpisodeRepository.h"

class SQLiteEpisodeRepository : public EpisodeRepository
{
public:
    explicit SQLiteEpisodeRepository(std::shared_ptr<SQLite3DB> database_);
    ~SQLiteEpisodeRepository() override {flush();};

    /*!
     * Creates a new entry and saves it to the database
     *
     * @throws An std::logic_error on failure
     * @returns The ID of the newly created object
     */
    uint64_t database_create(EpisodeEntry *entry) override;

    /*!
     * Loads an existing entry from the database
     *
     * @throws An std::logic_error on failure.
     * @param entry_id The ID of the entry to load
     */
    std::shared_ptr<EpisodeEntry> database_load(uint64_t entry_id) override;

    /*!
     * Updates the entry if it's already
     * an existing entry in the database
     *
     * @throws An std::logic_error on failure
     */
    void database_update(std::shared_ptr<EpisodeEntry> entry) override;

    /*!
     * Removes an entry from the database
     *
     * @param entry_id The ID of the entry
     */
    void database_erase(uint64_t entry_id) override;

    /*!
     * Iterates through every episode in a given season.
     *
     * @param season_id The ID of the season to get the episodes of
     * @param callback The callback to call for each row. Should return true
     * if more rows are wanted, false if it's finished.
     */
    void for_each_episode_in_season(uint64_t season_id, const std::function<bool(std::shared_ptr<EpisodeEntry> )> callback) override;

    /*!
     * Tries to get the ID of an episode from its filepath
     *
     * @param episode_filepath The filepath of the episode to get the ID of
     * @return An episode ID on success, NO_SUCH_ENTRY on failure.
     */
    uint64_t get_episode_id_from_filepath(const std::string &episode_filepath) override;

private:
    std::shared_ptr<SQLite3DB> database;
};


#endif //SFTPMEDIASTREAMER_SQLITEEPISODEREPOSITORY_H
