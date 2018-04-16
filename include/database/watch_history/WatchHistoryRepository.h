//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_WATCHHISTORYREPOSITORY_H
#define SFTPMEDIASTREAMER_WATCHHISTORYREPOSITORY_H


#include <database/DatabaseRepository.h>
#include "WatchHistoryEntry.h"

class WatchHistoryRepository : public DatabaseRepository<WatchHistoryEntry>
{
public:
    /*!
     * Iterates through each watch history entry,
     * in the order of most recent to least recent.
     *
     * @param unique True if duplicate seasons should be removed. False otherwise.
     * @param callback The functor to call for each unique watch history entry.
     * It should return true if it wants more entries, or false if it has had enough.
     */
    virtual void for_each_entry(bool unique, const std::function<bool(std::shared_ptr<WatchHistoryEntry>)> &callback) =0;

    /*!
     * Erases watch history for a given episode ID
     *
     * @param episode_id The ID of the episode to delete history for
     */
    virtual void erase_for_episode(uint64_t episode_id)=0;
};


#endif //SFTPMEDIASTREAMER_WATCHHISTORYREPOSITORY_H
