//
// Created by fred on 10/12/17.
//

#include <iostream>
#include <fstream>
#include <utility>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/box.h>
#include <VideoPlayer.h>
#include <SFTPStream.h>
#include <SystemUtilities.h>
#include <thread>
#include <gdkmm.h>
#include <Log.h>
#include <set>
#include "Application.h"
#include "SeasonListingWidget.h"

Application::Application(BaseObjectType *cobject,
                         const Glib::RefPtr<Gtk::Builder> &refBuilder,
                         Gtk::Window *window_,
                         std::shared_ptr<Library> library_,
                         std::shared_ptr<SFTPSession> sftp_)
: Gtk::Window(cobject),
  builder(refBuilder),
  library(std::move(library_)),
  sftp(std::move(sftp_)),
  window(window_)
{
    //Load results box from Glade file to fill it in
    builder->get_widget("flow_box", results_list);
    builder->get_widget("home_button", home_button);
    builder->get_widget("history_button", history_button);
    builder->get_widget("recently_added", recently_added_button);
    builder->get_widget("results_window", results_viewport);
    builder->get_widget("window_box", window_box);
    builder->get_widget("search_bar", search_bar);

    //Connect widgets
    home_button->signal_clicked().connect(sigc::mem_fun(*this, &Application::load_home));
    history_button->signal_clicked().connect(sigc::mem_fun(*this, &Application::load_history));
    recently_added_button->signal_clicked().connect(sigc::mem_fun(*this, &Application::load_recently_added));
    search_bar->signal_search_changed().connect(sigc::mem_fun(*this, &Application::signal_search_changed));

    //Load home
    load_home();
}

Application::~Application()
{
    video_player = nullptr;
    video_stream = nullptr;
    video_source = nullptr;
}

void Application::load_home()
{
    clear();
    frlog << Log::info << "Loading home screen" << Log::end;

    //Fill the flow box in with season entries
    library->for_each_season([&](std::shared_ptr<SeasonEntry> season) -> bool {

        //Create a season entry tile, and connect it to a season display handler
        auto season_listing = std::make_shared<SeasonListingWidget>(season);
        season_listing->signal_button_press_event().connect(
                sigc::bind<std::shared_ptr<SeasonListingWidget>>(sigc::mem_fun(*this,
                                                                               &Application::signal_library_listing_clicked), season_listing));

        //Add it to the global UI
        results_list->add(*season_listing);
        listed_results.emplace_back(std::move(season_listing));

        return true;
    });

    results_list->show_all();
}

void Application::load_history()
{
    clear();
    frlog << Log::info << "Loading history screen" << Log::end;

    library->for_each_recently_watched([&](std::shared_ptr<SeasonEntry> season) -> bool {

        //Create a season entry tile, and connect it to a season display handler
        auto season_listing = std::make_shared<SeasonListingWidget>(season);
        season_listing->signal_button_press_event().connect(
                sigc::bind<std::shared_ptr<SeasonListingWidget>>(
                        sigc::mem_fun(*this, &Application::signal_library_listing_clicked), season_listing));

        //Add it to the global UI
        results_list->add(*season_listing);
        listed_results.emplace_back(std::move(season_listing));

        return listed_results.size() < 50;
    });

    results_list->show_all();
}

void Application::load_recently_added()
{
    clear();
    frlog << Log::info << "Loading recently added screen" << Log::end;

    std::set<uint64_t> seasons_shown;
    library->for_each_recently_added([&](std::shared_ptr<SeasonEntry> season) -> bool {

        //Skip adding a tile if this season is already present
        if(seasons_shown.find(season->id) != seasons_shown.end())
            return true;
        seasons_shown.emplace(season->id);

        //Create a season entry tile, and connect it to a season display handler
        auto season_listing = std::make_shared<SeasonListingWidget>(season);
        season_listing->signal_button_press_event().connect(
                sigc::bind<std::shared_ptr<SeasonListingWidget>>(
                        sigc::mem_fun(*this, &Application::signal_library_listing_clicked), season_listing));

        //Add it to the global UI
        results_list->add(*season_listing);
        listed_results.emplace_back(std::move(season_listing));

        return listed_results.size() < 50;
    });

    results_list->show_all();
}

bool Application::signal_library_listing_clicked(GdkEventButton * /*button*/,
                                                 std::shared_ptr<SeasonListingWidget> season_listing)
{
    clear();
    frlog << Log::info << "Loading season screen" << Log::end;

    //Add in library tile's entries
    library->for_each_episode_in_season(season_listing->get_season_entry()->id, [&](std::shared_ptr<EpisodeEntry> episode) -> bool {

        //Check that this episode still exists on the server
        try
        {
            sftp->stat(episode->filepath);
        }
        catch(...)
        {
            //It no longer exists, erase it
            frlog << Log::info << "Deleting removed episode: " << episode->name << Log::end;
            library->delete_episode(episode->id);
            return true;
        }


        //Create a widget to display the episode
        auto episode_listing = std::make_shared<EpisodeListingWidget>(episode);
        episode_listing->signal_button_press_event().connect(
                sigc::bind<std::shared_ptr<EpisodeListingWidget>>(sigc::mem_fun(*this,
                                                                                &Application::signal_episode_listing_clicked), episode_listing));

        //Add it to the global UI
        results_list->add(*episode_listing);
        listed_results.emplace_back(std::move(episode_listing));

        return true;
    });

    results_list->show_all();
    return true;
}

bool Application::signal_episode_listing_clicked(GdkEventButton *button,
                                                 std::shared_ptr<EpisodeListingWidget> episode_listing)
{
    //Only react on click, not release
    if(button->type == GDK_BUTTON_RELEASE)
        return false;

    //If it's a right click then it's a 'set unwatched' request
    if(button->button == RIGHT_CLICK)
    {
        frlog << Log::info << "Marked " << episode_listing->get_episode_entry()->name << " as unwatched at user request" << Log::end;
        episode_listing->get_episode_entry()->watched = false;
        episode_listing->update();
        return true;
    }

    //Else, play this episode
    frlog << Log::info << "Playing " << episode_listing->get_episode_entry()->name << Log::end;
    episode_listing->get_episode_entry()->watched = true;
    episode_listing->update();
    library->add_to_watched(episode_listing->get_episode_entry()->id);

    Gtk::Container::remove(*window_box);
    add(video_box);

    //Setup the video widget and file stream
    video_source = std::make_unique<SFTPFile>(sftp->open(episode_listing->get_episode_entry()->filepath));
    video_stream = std::make_unique<SFTPStream>(video_source.get()); //todo: abstract, accept sf::InputStream from library instead
    video_player = std::make_unique<VideoPlayerWidget>(GDK_WINDOW_XID(get_window()->gobj()), video_stream.get());
    video_player->signal_playback_state_changed().connect(sigc::mem_fun(this, &Application::signal_play_state_changed));
    video_box.add(*video_player);
    video_box.show_all();

    get_window()->set_title(std::string(WINDOW_TITLE) + " - " + episode_listing->get_episode_entry()->name);
    return true;
}

void Application::signal_search_changed()
{
    //Remove all current entries, in preparation for adding only those that match
    std::string text = search_bar->get_text();
    frlog << Log::info << "Refining search to '" << text << "'" << Log::end;
    auto children = results_list->get_children();
    for(auto &iter : children)
        results_list->remove(*iter);

    //Add in any that match
    for(auto &entry : listed_results)
    {
        auto parent = entry->get_parent();
        if(parent)
            parent->remove(*entry);

        auto *search_entry = dynamic_cast<ISearchable*>(entry.get());
        if(!search_entry)
            continue;

        if(search_entry->matches(text))
            results_list->add(*entry);
    }

    //Reset scroll viewport to top
    results_viewport->get_vadjustment()->set_value(0.0);
}

void Application::signal_play_state_changed(VideoWidget::SignalType state)
{
    //If the video is stopped, then restore the episode select menu, and the original window title
    if(state == VideoWidget::Stopped)
    {
        frlog << Log::info << "Unloading video" << Log::end;
        Gtk::Container::remove(video_box);
        add(*window_box);
        video_player = nullptr;
        video_stream = nullptr;
        video_source = nullptr;
        get_window()->set_title(WINDOW_TITLE);
    }
}


void Application::clear()
{
    //Remove all library tiles
    frlog << Log::info << "Clearing screen entries" << Log::end;
    auto children = results_list->get_children();
    for(auto &iter : children)
        results_list->remove(*iter);
    listed_results.clear();
    search_bar->set_text("");

    //Reset scrolled window container scroll
    results_viewport->get_vadjustment()->set_value(0.0);
}
