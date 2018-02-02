//
// Created by fred on 10/12/17.
//

#ifndef SFTPMEDIASTREAMER_APPLICATION_H
#define SFTPMEDIASTREAMER_APPLICATION_H

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/window.h>
#include <gtkmm/grid.h>
#include <gtkmm/iconview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/viewport.h>
#include <gtkmm/searchbar.h>
#include <gtkmm/searchentry.h>
#include <database/watch_history/WatchHistoryRepository.h>
#include "SFTPSession.h"
#include "Thumbnailer.h"
#include "Library.h"
#include "SeasonListingWidget.h"
#include "EpisodeListingWidget.h"
#include "VideoWidget.h"
#include "VideoPlayerWidget.h"
#include "SFTPStream.h"

class Application : public Gtk::Window
{
public:
    /*!
     * Application constructor
     *
     * @param cobject GTK internal
     * @param refBuilder GTK internal
     * @param window The window to display to
     * @param library The library to source things from
     * @param sftp The SFTP connection to receive data over (abstract?)
     */
    Application(BaseObjectType* cobject,
                const Glib::RefPtr<Gtk::Builder>& refBuilder,
                Gtk::Window *window,
                std::shared_ptr<Library> library,
                std::shared_ptr<SFTPSession> sftp);
    ~Application() override;
private:

    /*!
     * Loads the home window
     */
    void load_home();

    /*!
     * Loads the history window
     */
    void load_history();

    /*!
     * Clears all currently displayed UI entries within the
     * main scroll box.
     */
    void clear();

    /*!
     * Signal called when a library entry is clicked
     */
    bool signal_library_listing_clicked(GdkEventButton *button, std::shared_ptr<SeasonListingWidget> entry);

    /*!
     * Signal called when an episode is clicked
     */
    bool signal_episode_listing_clicked(GdkEventButton *button, std::shared_ptr<EpisodeListingWidget> entry);

    /*!
     * Signal called to refine search results
     */
    void signal_search_changed();

    //Widgets
    Gtk::FlowBox *results_list;
    Gtk::Button *home_button;
    Gtk::Button *history_button;
    Gtk::Viewport *results_viewport;
    Gtk::SearchEntry *search_bar;
    std::unique_ptr<VideoPlayerWidget> video_player;
    std::unique_ptr<SFTPFile> video_source;
    std::unique_ptr<SFTPStream> video_stream;
    Gtk::Box *window_box;
    Gtk::EventBox video_box;

    //State
    const Glib::RefPtr<Gtk::Builder> &builder;
    std::vector<std::shared_ptr<Gtk::Widget>> listed_results;

    //Dependencies
    std::shared_ptr<Library> library;
    std::shared_ptr<SFTPSession> sftp;
    Gtk::Window *window;

    void signal_play_state_changed(VideoWidget::SignalType state);
};


#endif //SFTPMEDIASTREAMER_APPLICATION_H
