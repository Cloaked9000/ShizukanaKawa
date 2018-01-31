//
// Created by fred on 15/12/17.
//

#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/box.h>
#include <gdkmm.h>
#include "SeasonListingWidget.h"

SeasonListingWidget::SeasonListingWidget(std::shared_ptr<SeasonEntry> season_)
: season_entry(std::move(season_))
{
    //Setup widgets
    entry_label.set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_MIDDLE);
    entry_label.set_line_wrap(true);
    entry_label.set_max_width_chars(1);
    entry_label.set_line_wrap_mode(Pango::WrapMode::WRAP_WORD);

    entry_box.set_size_request(250, 250);
    entry_box.set_orientation(Gtk::Orientation::ORIENTATION_VERTICAL);
    entry_box.add(season_cover);
    entry_box.add(entry_label);

    //Set all of the widgets
    update();

    //Show all
    add(entry_box);
    show_all();
}

void SeasonListingWidget::update()
{
    //Setup search tag/tooltip and label
    set_search_string(season_entry->name);
    set_tooltip_text(season_entry->name);
    entry_label.set_text(season_entry->name);

    //Load thumbnail
    auto thumbnail_loader = Gdk::PixbufLoader::create();
    thumbnail_loader->write(reinterpret_cast<const guint8 *>(season_entry->thumbnail.data()), season_entry->thumbnail.size());
    thumbnail_loader->close();
    season_cover.set(thumbnail_loader->get_pixbuf());
}
