//
// Created by fred on 16/12/17.
//

#ifndef SFTPMEDIASTREAMER_LIBRARY_H
#define SFTPMEDIASTREAMER_LIBRARY_H

#include <vector>
#include <string>
#include <database/season/SeasonRepository.h>
#include <database/episode/EpisodeRepository.h>
#include <database/watch_history/WatchHistoryRepository.h>
#include <database/MiscRepository.h>
#include "SFTPSession.h"
#include "Thumbnailer.h"

class Library
{
public:

    Library(std::shared_ptr<SFTPSession> sftp,
            std::string library_root,
            std::shared_ptr<SeasonRepository> season_table,
            std::shared_ptr<EpisodeRepository> episode_table,
            std::shared_ptr<WatchHistoryRepository> watch_history_table,
            std::shared_ptr<MiscRepository> misc_table);

    /*!
     * Syncs the season table with what's actually on disk
     */
    void sync();

    /*!
     * Deletes a season with a given ID
     * (in database, not disk)
     *
     * @param season_id The ID of the season to delete
     */
    void delete_season(uint64_t season_id);

    /*!
     * Deletes a episode with a given ID
     * (in database, not disk)
     *
     * @param season_id The ID of the episode to delete
     */
    void delete_episode(uint64_t episode_id);

    /*!
     * Iterate through every season in the library
     *
     * @param callback The callback to call for each entry. Should return true if it wants more
     * entries, false if it has had enough.
     */
    inline void for_each_season(const std::function<bool(std::shared_ptr<SeasonEntry>)> &callback)
    {
        season_table->for_each_season(callback);
    }

    /*!
     * Iterate through every recently watched season
     *
     * @param callback The callback to call for each entry. Should return true if it wants more
     * entries, false if it has had enough.
     */
    inline void for_each_recently_watched(const std::function<bool(std::shared_ptr<SeasonEntry>)> &callback)
    {
        misc_table->for_each_recently_watched(callback);
    }

    /*!
     * Iterate through every episode within a season
     *
     * @param season_id The ID of the season to iterate through
     * @param callback The callback to call for each entry. Should return true if it wants more
     * entries, false if it has had enough.
     */
    inline void for_each_episode_in_season(uint64_t season_id, const std::function<bool(std::shared_ptr<EpisodeEntry>)> &callback)
    {
        episode_table->for_each_episode_in_season(season_id, callback);
    }

    /*!
     * Iterate through every watch history entry.
     * in the order of most recent to oldest.
     *
     * @param unique True if duplicate seasons should be removed. False otherwise.
     * @param season_id The ID of the season to iterate through
     * @param callback The callback to call for each entry. Should return true if it wants more
     * entries, false if it has had enough.
     */
    inline void for_each_watch_history_entry(bool unique, const std::function<bool(std::shared_ptr<WatchHistoryEntry>)> &callback)
    {
        watch_history_table->for_each_entry(unique, callback);
    }

    /*!
     * Iterate through seasons which have been recently added, and
     * seasons which have recently had new episodes added.
     *
     * @param callback The callback to call for each entry. Should return true if it wants more
     * entries, false if it has had enough.
     */
    inline void for_each_recently_added(const std::function<bool(std::shared_ptr<SeasonEntry>)> &callback)
    {
        misc_table->for_each_recently_added(callback);
    }

    /*!
     * Adds an episode to the watch history list
     *
     * @param episode_id The episode ID to add to the watch history
     */
    void add_to_watched(uint64_t episode_id);

    /*!
     * Loads the season entry of a given episode
     *
     * @throws An std::exception on failure
     * @param episode_id The ID of the episode to get the season of
     * @return The season entry associated with the episode on success.
     */
    std::shared_ptr<SeasonEntry> get_episode_season(uint64_t episode_id);

private:

    std::string generate_season_thumbnail(const std::string &remote_filepath);

    //State
    std::string library_root;
    Thumbnailer thumbnailer;

    //Dependencies
    std::shared_ptr<SFTPSession> sftp;
    std::shared_ptr<SeasonRepository> season_table;
    std::shared_ptr<EpisodeRepository> episode_table;
    std::shared_ptr<WatchHistoryRepository> watch_history_table;
    std::shared_ptr<MiscRepository> misc_table;
};


#endif //SFTPMEDIASTREAMER_LIBRARY_H
