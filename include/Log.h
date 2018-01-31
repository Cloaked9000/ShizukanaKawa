//
// Created by fred on 31/01/18.
//

#ifndef SFTPMEDIASTREAMER_LOG_H
#define SFTPMEDIASTREAMER_LOG_H


#include <cstdint>
#include <fstream>
#include <atomic>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <chrono>
#include "SystemUtilities.h"
#include "Config.h"

#define LOG_DIR_PATH "logs/"

//Set frlog define shortcut
#define frlog Log::logger

//Define static members
#define frlog_define() Log Log::logger; std::string Log::log_levels[Log::count]{"Info", "Warn", "Crit"};

class Log
{
public:
    //Log importance levels
    enum Level
    {
        info = 0,
        warn = 1,
        crit = 2,
        count = 3,
    };
    enum End
    {
        end = 0,
    };

    //Constructor/destructor
    Log()
            : lock(ATOMIC_FLAG_INIT){}

    //Disable copying/moving and whatnot
    void operator=(const Log &l)=delete;
    Log(const Log &l)=delete;
    Log(const Log &&l)=delete;

    /*!
     * Initialises the logging class
     *
     * @param replicate_to_stdout Should the log be repliacted to the standard output too?
     * @return True on successful initialisation, false on failure
     */
    static bool init(bool replicate_to_stdout = true)
    {
        Log::logger.max_log_size = 0;
        Log::logger.log_retention = 0;

        //Create logs directory
        if(!SystemUtilities::does_filepath_exist(LOG_DIR_PATH))
        {
            if(!SystemUtilities::create_directory(LOG_DIR_PATH))
            {
                std::cout << "Failed to create '" << LOG_DIR_PATH << "' directory. Exiting." << std::endl;
                return false;
            }
        }

        //Open a new log file
        Log::logger.logpath = LOG_DIR_PATH + suggest_log_filename();
        Log::logger.logstream.open(Log::logger.logpath);
        Log::logger.stdout_replication = replicate_to_stdout;
        if(!Log::logger.logstream.is_open())
        {
            std::cout << "Failed to open " << Log::logger.logpath << " for logging!" << std::endl;
            return false;
        }
        frlog << " -- Logging initialised -- " << Log::end;

        //Read config
        auto &config = Config::get_instance();
        try
        {
            //Read disk settings
            Log::logger.max_log_size = config.get<uint64_t>(CONFIG_MAX_LOG_SIZE);
            Log::logger.log_retention = config.get<uint32_t>(CONFIG_LOG_RETENTION);
        }
        catch(const std::exception &e)
        {
            throw std::runtime_error("Failed to load Log settings from config: " + std::string(e.what()));
        }

        //Delete old logs
        Log::logger.purge_old_logs();
        return true;
    }

    /*!
     * Returns the current timestamp in a string format
     *
     * @return The timestamp in format YYYY-MM-DD HH:MM:SS
     */
    static std::string get_current_timestamp()
    {
        //Get the current timestamp
        std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm now = *std::localtime(&time);
        now.tm_mon++; //Months start at 0 instead of 1 for some reason. But not days or years.

        //Convert the required bits of information into strings, padding with 0's where needed
        std::string day = now.tm_mday > 9 ?  std::to_string(now.tm_mday) : "0" + std::to_string(now.tm_mday);
        std::string month = now.tm_mon > 9 ?  std::to_string(now.tm_mon) : "0" + std::to_string(now.tm_mon);
        std::string year = now.tm_year > 9 ?  std::to_string(1900 + now.tm_year) : "0" + std::to_string(1900 + now.tm_year);

        std::string hour = now.tm_hour > 9 ?  std::to_string(now.tm_hour) : "0" + std::to_string(now.tm_hour);
        std::string min = now.tm_min > 9 ?  std::to_string(now.tm_min) : "0" + std::to_string(now.tm_min);
        std::string sec = now.tm_sec > 9 ?  std::to_string(now.tm_sec) : "0" + std::to_string(now.tm_sec);

        //Format data and return
        return year + "-" + month + "-" + day + " " + hour + ":" + min + ":" + sec;
    }

    /*!
     * Generates a suggested log name, with characters
     * safe on both Linux and Windows.
     *
     * @return A suitable log name
     */
    static std::string suggest_log_filename()
    {
        std::string timestamp = get_current_timestamp();
        std::replace(timestamp.begin(), timestamp.end(), ':', '-');
        return timestamp;
    }

    //Required overloads for the '<<' operator
    template<typename T>
    inline Log &operator<<(const T &data)
    {
        commit_log(std::to_string(data));
        return Log::logger;
    }

    inline Log &operator<<(const std::basic_string<char> &data)
    {
        commit_log(data);
        return Log::logger;
    }

    Log &operator<<(const char *data)
    {
        commit_log(data);
        return Log::logger;
    }

    Log &operator<<(Level loglevel)
    {
        //Spinlock using atomic flag, the inline asm is used to pause slightly so we don't waste too many CPU cycles
        while(lock.test_and_set(std::memory_order_acquire))
                asm volatile("pause\n": : :"memory");

        commit_log("[" + get_current_timestamp() + " " + log_levels[loglevel] + "]: ");

        return Log::logger;
    }

    Log &operator<<(End)
    {
        //End log and release lock
        commit_log("\n", true);
        lock.clear(std::memory_order_release);
        return Log::logger;
    }

    /*!
     * Flush the log out stream
     */
    static void flush()
    {
        Log::logger.logstream.flush();
    }

    /*!
     * Sets the number of bytes that may be written
     * before the log is cycled.
     *
     * @param val 0 for no limit. Otherwise the number of bytes before cycling.
     */
    static void set_max_log_size(uint64_t val)
    {
        Log::logger.max_log_size = val;
    }

    //The single instance of the class which will be used
    static Log logger;
private:
    /*!
     * Deletes logs which are out of retention
     */
    void purge_old_logs()
    {
        if(log_retention != 0)
        {
            //Get a list of logs
            std::vector<std::string> log_files;
            if(!SystemUtilities::list_files(LOG_DIR_PATH, log_files))
                throw std::runtime_error("Failed to enumerate files in: " + std::string(LOG_DIR_PATH));

            //If there's more than retention allows, delete some
            if(log_files.size() >= log_retention)
            {
                //Sort files by modification date
                std::sort(log_files.begin(), log_files.end(), [](const auto &f1, const auto &f2){
                    return SystemUtilities::get_modification_date(LOG_DIR_PATH + f1) < SystemUtilities::get_modification_date(LOG_DIR_PATH + f2);
                });

                //Erase oldest ones
                for(size_t a = 0; a < log_files.size() - log_retention; a++)
                {
                    if(std::remove(std::string(LOG_DIR_PATH + log_files[a]).c_str()) != 0)
                        throw std::runtime_error("Failed to erase old log file: " + std::string(LOG_DIR_PATH) + log_files[a] + ". Errno: " + std::to_string(errno));
                    else
                        std::cout << "Erased log file: " << LOG_DIR_PATH << log_files[a] << std::endl;
                }

            }
        }
    }

    /*!
     * Internal logging function, it's what all of the '<<' overloads feed into.
     * It will automatically cycle the log if it gets too big.
     *
     * @param data The data to log
     * @param may_cycle True of the logs should be cycled if over limit
     */
    inline void commit_log(const std::string &data, bool may_cycle = false)
    {
        //Print it
        if(stdout_replication)
            std::cout << data;
        logstream << data;
        flush();

        //Cycle logs if needed
        if(may_cycle && max_log_size != 0 && (size_t)logstream.tellp() > max_log_size)
            do_log_cycle();
    }

    /*!
     * Closes the current log,
     * opens a new one.
     */
    void do_log_cycle()
    {
        //Close current log file and open new one
        logstream.close();
        logpath = LOG_DIR_PATH + suggest_log_filename();
        logstream.open(Log::logger.logpath);
        if(!Log::logger.logstream.is_open())
            throw std::runtime_error("Failed to open new log file: " + logpath);

        //Delete old logs if needbe
        purge_old_logs();
    }

    static std::string log_levels[]; //String log levels
    std::atomic_flag lock; //Atomic flag used for thread safe logging using a spinlock
    std::ofstream logstream; //Out output stream
    bool stdout_replication{true}; //Should logs also be sent to stdout?
    std::atomic<uint64_t> max_log_size{}; //Max allowed log size in bytes. 0 means infinite.
    std::string logpath; //The log filepath
    std::atomic<uint32_t> log_retention{}; //Number of log iterations to keep
};




#endif //SFTPMEDIASTREAMER_LOG_H
