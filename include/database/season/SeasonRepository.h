//
// Created by fred on 21/01/18.
//

#ifndef SFTPMEDIASTREAMER_SEASONREPOSITORY_H
#define SFTPMEDIASTREAMER_SEASONREPOSITORY_H


#include <functional>
#include <database/DatabaseRepository.h>
#include "SeasonEntry.h"

class SeasonRepository : public DatabaseRepository<SeasonEntry>
{
public:
    /*!
     * Iterates through every row in the table, calling a callback
     * for each entry.
     *
     * @param callback The callback to call for each row. Should return true
     * if more rows are wanted, false if it's finished.
     */
    virtual void for_each_season(const std::function<bool(std::shared_ptr<SeasonEntry> )> &callback)=0;

    /*!
     * Tries to get the ID of a season from its filepath
     *
     * @param season_filepath The filepath of the season to get the ID of
     * @return A season ID on success, NO_SUCH_ENTRY on failure.
     */
    virtual uint64_t get_season_id_from_filepath(const std::string &season_filepath)=0;
};


#endif //SFTPMEDIASTREAMER_SEASONREPOSITORY_H