//
// Created by fred on 14/04/18.
//

#ifndef SFTPMEDIASTREAMER_MISCREPOSITORY_H
#define SFTPMEDIASTREAMER_MISCREPOSITORY_H
#include <memory>
#include <functional>
#include <database/season/SeasonEntry.h>

class MiscRepository
{
public:
    virtual ~MiscRepository()= default;

    /*!
     * Iterate through seasons which have been recently added, and
     * seasons which have recently had new episodes added.
     *
     * @param callback The callback to call for each entry. Should return true if it wants more
     * entries, false if it has had enough.
     */
    virtual void for_each_recently_added(const std::function<bool(std::shared_ptr<SeasonEntry>)> &callback) =0;

    /*!
     * Iterate through seasons which have been recently watched.
     *
     * @param callback The callback to call for each entry. Should return true if it wants more
     * entries, false if it has had enough.
     */
    virtual void for_each_recently_watched(const std::function<bool(std::shared_ptr<SeasonEntry>)> &callback) =0;
};


#endif //SFTPMEDIASTREAMER_MISCREPOSITORY_H
