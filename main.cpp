#include <thread>
#include <iostream>
#include <mutex>
#include <SFML/Graphics.hpp>
#include <SSHConnection.h>
#include <SFTPSession.h>
#include <cstring>
#include <atomic>
#include <gtkmm-3.0/gtkmm.h>
#include <Application.h>
#include <database/SQLite3DB.h>
#include <database/season/SQLiteSeasonRepository.h>
#include <database/episode/SQLiteEpisodeRepository.h>
#include <database/watch_history/SQLiteWatchHistoryRepository.h>
#include <Log.h>

//Main
frlog_define()

int main(int argc, char** argv)
{
    //Initialise logging
    if(!Log::init())
    {
        std::cerr << "Failed to initialise logging. Exiting." << std::endl;
        return EXIT_FAILURE;
    }

    //Open config
    Config &config = Config::get_instance();

    //Initialise database
    auto database = std::make_shared<SQLite3DB>();
    if(!database->open("database.db"))
    {
        std::cerr << "Failed to open internal database, database.db. Exiting." << std::endl;
        return EXIT_FAILURE;
    }

    //Start SFTP connection. Note: Only keyring is supported at the moment. So identity should be loaded prior to starting.
    SSHConnection connection;
    connection.connect(config.get<std::string>(CONFIG_SFTP_IP), config.get<uint32_t>(CONFIG_SFTP_PORT), config.get<std::string>(CONFIG_SFTP_USERNAME));
    auto sftp = std::make_shared<SFTPSession>(&connection);

    //Initialise database repositories
    auto season_table = std::make_shared<SQLiteSeasonRepository>(database);
    auto episode_table = std::make_shared<SQLiteEpisodeRepository>(database);
    auto watch_history_table = std::make_shared<SQLiteWatchHistoryRepository>(database);
    auto library = std::make_shared<Library>(sftp, config.get<std::string>(CONFIG_LIBRARY_LOCATION), season_table, episode_table, watch_history_table);

    //Build GUI from glade file
    auto glade_builder = Gtk::Builder::create();
    auto application = Gtk::Application::create(argc, argv, "fred.ssh.streamer");
    glade_builder->add_from_file("gui.glade");

    //Start application
    Application *window;
    glade_builder->get_widget_derived("main_window", window, window, library, sftp);
    window->set_title(WINDOW_TITLE);
    application->run(*window);
}