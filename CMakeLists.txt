cmake_minimum_required(VERSION 3.9)
project(SFTPMediaStreamer)

link_directories(/usr/local/lib/)

# Set CMake modules path
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

#Find libraries
FIND_PACKAGE(SFML)
INCLUDE_DIRECTORIES(${SFML_INCLUDE_DIR})
find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)

include_directories(include)
set(CMAKE_CXX_STANDARD 17)

set(CORE_CXX_FLAGS "-m64 -pthread -Wstrict-aliasing -msse -msse2 -Wall -Werror -Wextra -pedantic-errors -lssh -lvlc -rdynamic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CORE_CXX_FLAGS} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CORE_CXX_FLAGS} -O2")


link_directories(
        ${GTKMM_LIBRARY_DIRS}
)

include_directories(
        ${GTKMM_INCLUDE_DIRS}
)

        add_executable(SFTPMediaStreamer main.cpp src/SSHConnection.cpp include/SSHConnection.h src/SFTPSession.cpp include/SFTPSession.h src/SFTPFile.cpp include/SFTPFile.h src/SFTPStream.cpp include/SFTPStream.h include/Types.h src/VideoPlayer.cpp include/VideoPlayer.h src/Application.cpp include/Application.h src/SeasonListingWidget.cpp include/SeasonListingWidget.h src/SystemUtilities.cpp include/SystemUtilities.h src/Thumbnailer.cpp include/Thumbnailer.h src/Library.cpp include/Library.h src/EpisodeListingWidget.cpp include/EpisodeListingWidget.h src/VideoWidget.cpp include/VideoWidget.h src/VideoPlayerWidget.cpp include/VideoPlayerWidget.h src/database/SQLite3DB.cpp include/database/SQLite3DB.h include/database/DBType.h src/VideoControlWidget.cpp include/VideoControlWidget.h include/ISearchable.h include/database/episode/EpisodeEntry.h include/database/season/SeasonEntry.h include/database/watch_history/WatchHistoryEntry.h include/database/DatabaseRepository.h include/database/episode/EpisodeRepository.h include/database/season/SeasonRepository.h include/database/watch_history/WatchHistoryRepository.h include/database/episode/SQLiteEpisodeRepository.cpp include/database/episode/SQLiteEpisodeRepository.h include/database/season/SQLiteSeasonRepository.cpp include/database/season/SQLiteSeasonRepository.h include/database/watch_history/SQLiteWatchHistoryRepository.cpp include/database/watch_history/SQLiteWatchHistoryRepository.h src/Config.cpp include/Config.h include/Log.h src/SignalHandler.cpp include/SignalHandler.h include/database/MiscRepository.h src/database/SQLiteMiscRepository.cpp include/database/SQLiteMiscRepository.h)

#Link against libraries
TARGET_LINK_LIBRARIES(SFTPMediaStreamer ${SFML_LIBRARIES} -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-network -lsfml-system -lX11 -lsqlite3 ${GTKMM_LIBRARIES})