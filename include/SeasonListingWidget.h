//
// Created by fred on 15/12/17.
//

#ifndef SFTPMEDIASTREAMER_TILEDENTRY_H
#define SFTPMEDIASTREAMER_TILEDENTRY_H


#include <gtkmm/eventbox.h>
#include <database/season/SeasonEntry.h>
#include "ISearchable.h"

class SeasonListingWidget : public Gtk::EventBox, public ISearchable
{
public:
    /*!
     * Constructor
     *
     * @param season The season to represent. The tooltip and search tag is set to the name of the season.
     */
    explicit SeasonListingWidget(std::shared_ptr<SeasonEntry> season);
    ~SeasonListingWidget() final =default;

    /*!
     * Should be called if the season entry that this widget
     * references is changed, to show any changes visually.
     */
    void update();

    /*!
     * Gets the season entry that this listing widget
     * references.
     *
     * @return The internal episode entry
     */
    inline std::shared_ptr<SeasonEntry> get_season_entry()
    {
        return season_entry;
    }
private:

    //Widget stuff
    Gtk::Image season_cover;
    Gtk::Label entry_label;
    Gtk::Box entry_box;

    //Dependencies
    std::shared_ptr<SeasonEntry> season_entry;
};


#endif //SFTPMEDIASTREAMER_TILEDENTRY_H
