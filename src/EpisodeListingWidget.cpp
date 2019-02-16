//
// Created by fred on 16/12/17.
//

#include "EpisodeListingWidget.h"

Glib::RefPtr<Gdk::Pixbuf> EpisodeListingWidget::watched_image = {};
Glib::RefPtr<Gdk::Pixbuf> EpisodeListingWidget::unwatched_image = {};

EpisodeListingWidget::EpisodeListingWidget(std::shared_ptr<EpisodeEntry> episode_entry_)
: episode_entry(std::move(episode_entry_))
{
    //Load watched/unwatched images if not done
    if(!watched_image)
        watched_image = Gdk::Pixbuf::create_from_file("resources/watched_icon.png");
    if(!unwatched_image)
        unwatched_image = Gdk::Pixbuf::create_from_file("resources/unwatched_icon.png");

    //Set widget data to represent stored episode
    update();

    //Setup nested widgets
    box.set_orientation(Gtk::Orientation::ORIENTATION_HORIZONTAL);
    box.add(watched_icon);
    box.add(label);

    //Display everything
    add(box);
    show_all();
}

void EpisodeListingWidget::update()
{
    watched_icon.set(episode_entry->get_watched() ? watched_image : unwatched_image);
    label.set_text(episode_entry->get_name());

    set_search_string(episode_entry->get_name());
    set_tooltip_text(episode_entry->get_name());
}
