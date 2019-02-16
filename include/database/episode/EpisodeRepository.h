//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_EPISODEREPOSITORY_H
#define SFTPMEDIASTREAMER_EPISODEREPOSITORY_H


#include <database/DatabaseRepository.h>
#include "EpisodeEntry.h"

class EpisodeRepository : public DatabaseRepository<EpisodeEntry>
{
public:
    /*!
     * Iterates through every episode in a given season.
     *
     * @param season_id The ID of the season to get the episodes of
     * @param callback The callback to call for each row. Should return true
     * if more rows are wanted, false if it's finished.
     */
    virtual void for_each_episode_in_season(uint64_t season_id, const std::function<bool(std::shared_ptr<EpisodeEntry> )> callback) =0;

    /*!
     * Tries to get the ID of an episode from its filepath
     *
     * @param episode_filepath The filepath of the episode to get the ID of
     * @return An episode ID on success, NO_SUCH_ENTRY on failure.
     */
    virtual uint64_t get_episode_id_from_filepath(const std::string &episode_filepath)=0;
};


#endif //SFTPMEDIASTREAMER_EPISODEREPOSITORY_H
