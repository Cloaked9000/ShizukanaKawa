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
                         std::shared_ptr<Library> library_,
                         std::shared_ptr<SFTPSession> sftp_)
: Gtk::Window(cobject),
  builder(refBuilder),
  library(std::move(library_)),
  sftp(std::move(sftp_))
{
    //Load icon
    auto icon = Gdk::Pixbuf::create_from_file("resources/icon.png");
    set_icon(icon);

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
        if(seasons_shown.find(season->get_id()) != seasons_shown.end())
            return true;
        seasons_shown.emplace(season->get_id());

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

bool Application::signal_library_listing_clicked(GdkEventButton *button,
                                                 std::shared_ptr<SeasonListingWidget> season_listing)
{
    //If it's a right click then it's a 'set unwatched' request
    if(button->button == RIGHT_CLICK)
    {
        frlog << Log::info << "Regenerating season thumbnail for: " << season_listing->get_season_entry()->get_name() << Log::end;
        season_listing->get_season_entry()->set_thumbnail(library->generate_season_thumbnail(season_listing->get_season_entry()->get_filepath()));
        season_listing->update();
        return true;
    }


    //Else load episode selector
    clear();
    frlog << Log::info << "Loading season screen" << Log::end;

    //Add in library tile's entries
    library->for_each_episode_in_season(season_listing->get_season_entry()->get_id(), [&](std::shared_ptr<EpisodeEntry> episode) -> bool {

        //Check that this episode still exists on the server
        try
        {
            sftp->stat(episode->get_filepath());
        }
        catch(...)
        {
            //It no longer exists, erase it
            frlog << Log::info << "Deleting removed episode: " << episode->get_name() << Log::end;
            library->delete_episode(episode->get_id());
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
        frlog << Log::info << "Marked " << episode_listing->get_episode_entry()->get_name() << " as unwatched at user request" << Log::end;
        episode_listing->get_episode_entry()->set_watched(false);
        episode_listing->get_episode_entry()->set_watch_offset(0);
        episode_listing->update();
        return true;
    }

    //Else, play this episode
    frlog << Log::info << "Playing " << episode_listing->get_episode_entry()->get_name() << Log::end;
    episode_listing->get_episode_entry()->set_watched(true);
    episode_listing->update();
    library->add_to_watched(episode_listing->get_episode_entry()->get_id());

    Gtk::Container::remove(*window_box);
    add(video_box);

    //Setup the video widget and file stream
    current_playing = episode_listing->get_episode_entry();
    auto video_source = std::make_unique<SFTPFile>(sftp->open(current_playing->get_filepath()));
    auto video_stream = std::make_unique<SFTPStream>(std::move(video_source)); //todo: abstract, accept sf::InputStream from library instead
    video_player = std::make_unique<VideoPlayerWidget>(GDK_WINDOW_XID(get_window()->gobj()), std::move(video_stream));
    video_player->signal_playback_state_changed().connect(sigc::mem_fun(this, &Application::signal_play_state_changed));
    video_player->set_playback_offset(current_playing->get_watch_offset());
    video_player->set_audio_track(current_playing->get_audio_track());
    video_player->set_subtitle_track(current_playing->get_sub_track());
    video_box.add(*video_player);
    video_box.show_all();

    get_window()->set_title(std::string(WINDOW_TITLE) + " - " + current_playing->get_name());
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
        //Update episode object with new playback offset/subtrack/audio track etc
        if(current_playing)
        {
            frlog << Log::info << "Unloading video: " << current_playing->get_name() << Log::end;
            current_playing->set_audio_track(video_player->get_audio_track());
            current_playing->set_sub_track(video_player->get_subtitle_track());
            current_playing->set_watch_offset(video_player->get_playback_offset());
        }

        //Update UI
        Gtk::Container::remove(video_box);
        add(*window_box);
        video_player = nullptr;
        current_playing = nullptr;
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
