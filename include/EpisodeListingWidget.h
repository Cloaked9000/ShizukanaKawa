//
// Created by fred on 16/12/17.
//

#ifndef SFTPMEDIASTREAMER_MEDIAENTRY_H
#define SFTPMEDIASTREAMER_MEDIAENTRY_H


#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/box.h>
#include <database/season/SeasonEntry.h>
#include <database/episode/EpisodeEntry.h>
#include "ISearchable.h"

class EpisodeListingWidget : public Gtk::EventBox, public ISearchable
{
public:
    /*!
     * Constructor
     *
     * @param episode_entry the episode to represent. The search tag/tooltip will be set to season name.
     */
    explicit EpisodeListingWidget(std::shared_ptr<EpisodeEntry> episode_entry);
    ~EpisodeListingWidget() final =default;

    /*!
     * Should be called if the episode entry that this widget
     * references is changed, to show any changes visually.
     */
    void update();

    /*!
     * Gets the episode entry that this listing widget
     * references.
     *
     * @return The internal episode entry
     */
    inline std::shared_ptr<EpisodeEntry> get_episode_entry()
    {
        return episode_entry;
    }
private:
    //Widget stuff
    Gtk::Box box;
    Gtk::Image watched_icon;
    Gtk::Label label;

    static Glib::RefPtr<Gdk::Pixbuf> watched_image;
    static Glib::RefPtr<Gdk::Pixbuf> unwatched_image;

    //Dependencies
    std::shared_ptr<EpisodeEntry> episode_entry;
};


#endif //SFTPMEDIASTREAMER_MEDIAENTRY_H
