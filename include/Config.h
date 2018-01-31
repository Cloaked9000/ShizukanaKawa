//
// Created by fred on 31/01/18.
//

#ifndef SFTPMEDIASTREAMER_CONFIG_H
#define SFTPMEDIASTREAMER_CONFIG_H
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <strings.h>

#define CONFIG_FILEPATH "config.ini"

#define CONFIG_LOG_RETENTION "logging.retention"
#define CONFIG_MAX_LOG_SIZE "logging.max_log_size"
#define CONFIG_SFTP_IP "sftp.ip"
#define CONFIG_SFTP_PORT "sftp.port"
#define CONFIG_SFTP_USERNAME "sftp.username"
#define CONFIG_SFTP_PASSWORD "sftp.password"
#define CONFIG_SFTP_KEYFILE "sftp.keyfile"
#define CONFIG_LIBRARY_LOCATION "library.location"

class Log;
class Config
{
public:
    //Disable moving/copying
    Config(const Config&) = delete;
    Config(Config&&) = delete;

    //Allow loading of specific files
    explicit Config(const std::string &filepath);

    /*!
     * Gets the Config singleton instance.
     *
     * @return The Config instance
     */
    inline static Config &get_instance()
    {
        static Config instance;
        return instance;
    }

    /*!
     * Gets the config value 'key' if T is an std::string
     *
     * Throws an std::logic_error if the config value cannot be found
     *
     * @tparam T The type to get the config value as
     * @param key The name of the config value to fetch
     * @return The config value behind 'key'
     */
    template<typename T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
    T get(const std::string &key)
    {
        std::lock_guard<std::mutex> guard(lock);
        auto fPos = settings.find(key);
        if(fPos == settings.end())
            throw std::logic_error("Setting '" + key + "' requested from Config. But no setting with that name exists.");
        return fPos->second;
    }
    /*!
     * Gets the config value 'key' if T is integral, but not boolean.
     *
     * Throws an std::logic_error if the config value cannot be found
     *
     * @tparam T The type to get the config value as
     * @param key The name of the config value to fetch
     * @return The config value behind 'key'
     */
    template<typename T, typename std::enable_if<!std::is_same<T, bool>::value && std::is_integral<T>::value, int>::type* = nullptr>
    T get(const std::string &key)
    {
        std::lock_guard<std::mutex> guard(lock);
        auto fPos = settings.find(key);
        if(fPos == settings.end())
            throw std::logic_error("Setting '" + key + "' requested from Config. But no setting with that name exists.");

        return std::stoull(fPos->second);
    }

    /*!
     * Gets the config value 'key' if T is bool.
     *
     * Throws an std::logic_error if the config value cannot be found,
     * or the config value cannot be converted to bool.
     *
     * @tparam T The type to get the config value as
     * @param key The name of the config value to fetch
     * @return The config value behind 'key'
     */
    template<typename T, typename std::enable_if<std::is_same<T, bool>::value>::type* = nullptr>
    T get(const std::string &key)
    {
        std::lock_guard<std::mutex> guard(lock);
        auto fPos = settings.find(key);
        if(fPos == settings.end())
            throw std::logic_error("Setting '" + key + "' requested from Config. But no setting with that name exists.");

        if(fPos->second == "1")
            return true;
        else if(fPos->second == "0")
            return false;

        if(fPos->second.size() == 4 && strncasecmp(fPos->second.c_str(), "true", 4) == 0)
            return true;
        else if(fPos->second.size() == 5 && strncasecmp(fPos->second.c_str(), "false", 5) == 0)
            return false;

        throw std::logic_error("Setting '" + key + "' was requested as a boolean, but the value '" + fPos->second + "' is not boolean.");
    }

    /*!
     * Sets the config value 'key' if T is an std::string
     * This can also be used to create new sections.
     *
     * @tparam T The type to set the config value as
     * @param key The name of the config value to set
     * @param value The value to set it to
     */
    template<typename T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
    void set(const std::string &key, const T &value)
    {
        if(key.find('.') == std::string::npos)
            throw std::logic_error("New config setting '" + key + "' does not specify a section name.");

        std::lock_guard<std::mutex> guard(lock);
        settings[key] = value;
    }

    /*!
     * Sets the config value 'key' if T is an integer
     * This can also be used to create new sections.
     *
     * @tparam T The type to set the config value as
     * @param key The name of the config value to set
     * @param value The value to set it to
     */
    template<typename T, typename std::enable_if<!std::is_same<T, bool>::value && std::is_integral<T>::value, int>::type* = nullptr>
    void set(const std::string &key, const T &value)
    {
        if(key.find('.') == std::string::npos)
            throw std::logic_error("New config setting '" + key + "' does not specify a section name.");

        std::lock_guard<std::mutex> guard(lock);
        settings[key] = std::to_string(value);
    }

    /*!
     * Sets the config value 'key' if T is a boolean.
     * This can also be used to create new sections.
     *
     * @tparam T The type to set the config value as
     * @param key The name of the config value to set
     * @param value The value to set it to
     */
    template<typename T, typename std::enable_if<std::is_same<T, bool>::value>::type* = nullptr>
    void set(const std::string &key, T value)
    {
        if(key.find('.') == std::string::npos)
            throw std::logic_error("New config setting '" + key + "' does not specify a section name.");

        std::lock_guard<std::mutex> guard(lock);
        settings[key] = value ? "true" : "false";
    }

    /*!
     * Loads a config file from a specific location on disk.
     *
     * @param filepath The filepath of the config file to parse
     * @return True on success, false on failure.
     */
    bool load_from_file(const std::string &filepath);

    /*!
     * Saves the config back to file
     *
     * @param filepath File location to save it to
     * @return True on success, false on failure.
     */
    bool save_to_file(const std::string &filepath);

    /*!
     * Deletes a section in the config file
     *
     * @param name The name of the section to delete
     */
    void delete_section(const std::string &name);

    /*!
     * Renames a section
     *
     * @param current_name The current name of the section
     * @param new_name The new name of the section
     */
    void rename_section(const std::string &current_name, const std::string &new_name);

    /*!
     * Deletes a setting
     *
     * @param name The name of the setting to delete
     */
    void delete_setting(const std::string &name);

    /*!
     * Renames a setting
     *
     * @param current_name The current name of the setting
     * @param new_name The new name of the setting
     */
    void rename_setting(const std::string &current_name, const std::string &new_name);
private:
    Config();

    /*!
     * Erases leading spaces in a string
     *
     * @param str The string to remove leading spaces from
     */
    inline void erase_leading_spaces(std::string &str);

    /*!
     * Erases trailing spaces in a string
     *
     * @param str The string to remove trailing spaces from
     */
    inline void erase_trailing_spaces(std::string &str);

    //Where the settings themselves are stored
    std::unordered_map<std::string, std::string> settings;
    std::unordered_map<std::string, std::vector<std::string>> comments;
    std::mutex lock;
};

#endif //SFTPMEDIASTREAMER_CONFIG_H
