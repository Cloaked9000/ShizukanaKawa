//
// Created by fred on 16/12/17.
//

#include <SFTPStream.h>
#include <SFML/Graphics/Image.hpp>
#include <SystemUtilities.h>
#include <database/episode/EpisodeEntry.h>
#include <iostream>
#include <Log.h>
#include "Library.h"

Library::Library(std::shared_ptr<SFTPSession> sftp_,
                 std::string library_root_,
                 std::shared_ptr<SeasonRepository> season_table_,
                 std::shared_ptr<EpisodeRepository> episode_table_,
                 std::shared_ptr<WatchHistoryRepository> watch_history_table_,
                 std::shared_ptr<MiscRepository> misc_table_)

: library_root(std::move(library_root_)),
  sftp(std::move(sftp_)),
  season_table(std::move(season_table_)),
  episode_table(std::move(episode_table_)),
  watch_history_table(std::move(watch_history_table_)),
  misc_table(std::move(misc_table_))
{
    sync();
}


void Library::sync()
{
    frlog << Log::info << "Syncing library... " << Log::end;

    //Get a list of files physically in the library
    std::vector<Attributes> root_list = sftp->enumerate_directory(library_root);

    //Delete any database entries which are no longer on disk
    season_table->for_each_season([&](std::shared_ptr<SeasonEntry> season) -> bool {

        //Check if this season is actually on disk
        auto season_iter = std::find_if(root_list.begin(), root_list.end(), [&](const Attributes &attr) {
            return attr.full_name == season->filepath;
        });

        //Erase from the database if it couldn't be found on disk
        if(season_iter == root_list.end())
        {
            frlog << Log::info << "Deleting removed season: " << season->name << Log::end;
            delete_season(season->id);
        }
        return true;
    });

    //Now ensure that each season on disk has a related database entry
    for(auto &season : root_list)
    {
        //Check if the season is in the database, or not a directory
        uint64_t season_id = season_table->get_season_id_from_filepath(season.full_name);
        if(season_id != NO_SUCH_ENTRY || season.type != Attributes::Directory)
            continue;

        //It's not, so generate an entry and store it.
        std::string thumbnail = generate_season_thumbnail(season.full_name);
        if(thumbnail.empty())
            continue;

        frlog << Log::info << "Found new season: " << season.name << Log::end;
        SeasonEntry new_season(0, season.full_name, season.name, std::move(thumbnail), time(nullptr));
        season_table->create(&new_season);
    }

    //Now ensure that each episode in each season has a related database entry
    season_table->for_each_season([&](std::shared_ptr<SeasonEntry> season) -> bool {

        //Enumerate the season's directory
        std::vector<Attributes> episode_list = sftp->enumerate_directory(season->filepath);
        for(auto &episode : episode_list)
        {
            //Skip anything that isn't an episode (regular video file)
            if(episode.type != Attributes::Regular)
                continue;

            //Skip it if it's already in the database
            uint64_t episode_id = episode_table->get_episode_id_from_filepath(episode.full_name);
            if(episode_id != NO_SUCH_ENTRY)
                continue;

            //Create the entry
            frlog << Log::info << "Found new episode for " << season->name << ": " << episode.name << Log::end;
            EpisodeEntry new_episode(0, season->id, episode.full_name, episode.name, false, 0, AUDIO_TRACK_UNSET, SUB_TRACK_UNSET, time(nullptr));
            episode_table->create(&new_episode);
        }

        return true;
    });

    frlog << Log::info << "Finished library sync" << Log::end;
}

std::string Library::generate_season_thumbnail(const std::string &remote_filepath)
{
    //If not, try and generate a thumbnail
    Attributes attributes = sftp->stat(remote_filepath);
    std::string cover_source;
    if(attributes.type == Attributes::Directory)
    {
        //It's a directory, so find a media file which we can base a cover image on
        auto media_list = sftp->enumerate_directory(attributes.full_name);
        auto iter = std::find_if(media_list.begin(), media_list.end(), [](const Attributes &attr) {
            return attr.type == Attributes::Regular && (attr.name.find(".mkv") != std::string::npos || attr.name.find(".mp4") != std::string::npos);
        });
        if(media_list.empty() || iter == media_list.end())
            return "";
        cover_source = iter->full_name;
    }

    if(cover_source.empty())
        return "";

    //Generate a thumbnail.
    SFTPFile file = sftp->open(cover_source);
    SFTPStream video_stream(&file);
    sf::Image thumbnail = thumbnailer.generate_thumbnail(video_stream, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
    thumbnail.saveToFile("tmp_thumbnail.jpg");
    return SystemUtilities::read_binary_file("tmp_thumbnail.jpg");
}

void Library::delete_season(uint64_t season_id)
{
    //Iterate through all watch history and erase
    watch_history_table->for_each_entry(false, [&](const std::shared_ptr<WatchHistoryEntry> &episode) -> bool {
        watch_history_table->erase(episode->id);
        return true;
    });

    //Iterate through each episode of the season and delete those
    episode_table->for_each_episode_in_season(season_id, [&](const std::shared_ptr<EpisodeEntry> &episode) -> bool {
        episode_table->erase(episode->id);
        return true;
    });

    //Now the season itself
    season_table->erase(season_id);
}

std::shared_ptr<SeasonEntry> Library::get_episode_season(uint64_t episode_id)
{
    std::shared_ptr<EpisodeEntry> episode = episode_table->load(episode_id);
    return season_table->load(episode->season_id);
}


void Library::add_to_watched(uint64_t episode_id)
{
    WatchHistoryEntry entry(0, episode_id, std::time(nullptr));
    watch_history_table->create(&entry);
}