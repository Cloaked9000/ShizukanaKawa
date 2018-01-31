//
// Created by fred on 31/01/18.
//

#include <map>
#include <thread>
#include <SystemUtilities.h>
#include <Log.h>
#include "Config.h"

static std::string default_config = ""
        "[sftp]\n"
        "ip=\"127.0.0.1\"\n"
        "port=22\n"
        "username=\"user\"\n"
        "password=\"\"\n"
        "keyfile=\"\"\n"
        "\n"
        "[library]\n"
        "location=\"/remote/sftp/location\"\n"
        "\n"
        "[logging]\n"
        "retention=14\n"
        "max_log_size=0x20000000\n";

Config::Config()
{
    //Load settings file
    if(!SystemUtilities::does_filepath_exist(CONFIG_FILEPATH))
    {
        frlog << Log::warn << "Failed to read configurations file '" << CONFIG_FILEPATH << "'. Creating a blank one now." << Log::end;

        //Write template file
        if(!SystemUtilities::write_file(CONFIG_FILEPATH, {default_config}))
        {
            frlog << Log::crit << "Failed to write blank config file to '" << CONFIG_FILEPATH << "'. Exiting." << Log::end;
            exit(0);
        }
        frlog << Log::info << "A blank config file has been created at '" << CONFIG_FILEPATH << "'. Please fill it in before starting the client again." << Log::end;
        exit(0);
    }

    if(!load_from_file(CONFIG_FILEPATH))
        throw std::runtime_error("Failed to parse config file: " + std::string(CONFIG_FILEPATH));
}

Config::Config(const std::string &filepath)
{
    if(!load_from_file(filepath))
        throw std::runtime_error("Failed to parse config file: " + filepath);
}

void Config::erase_trailing_spaces(std::string &str)
{
    //Erase trailing spaces
    auto space_pos = str.find_last_not_of(' ');
    if(space_pos != std::string::npos && space_pos != str.size() - 1)
        str.erase(space_pos + 1, str.size() - space_pos - 1);
}

void Config::erase_leading_spaces(std::string &str)
{
    auto space_pos = str.find_first_not_of(' ');
    if(space_pos > 0)
        str.erase(0, space_pos);
}

bool Config::load_from_file(const std::string &filepath)
{
    frlog << Log::info << "Parsing config file: " << filepath << Log::end;
    std::lock_guard<std::mutex> guard(lock);
    settings.clear();
    comments.clear();

    //Read into memory
    std::vector<std::string> config;
    if(!SystemUtilities::read_file(CONFIG_FILEPATH, config))
    {
        frlog << Log::crit << "Failed to read configurations file '" << filepath << "'" << Log::end;
        return false;
    }

    size_t lineno = 0;
    try
    {
        //Parse the file into the settings map
        std::string current_section = " ";
        for(; lineno < config.size(); lineno++)
        {
            std::string &line = config[lineno];

            //Skip if empty line or just spaces
            if(line.empty() || line.find_first_not_of(' ') == std::string::npos)
                continue;

            //Store comments and skip
            if(line.front() == ';')
            {
                comments[current_section].emplace_back(line);
                continue;
            }

            //If it's a section name, read it
            if(line.front() == '[')
            {
                //Find section name end
                size_t section_end = line.find_last_of(']');
                if(section_end == std::string::npos)
                {
                    throw std::logic_error("End of section definition not found");
                }
                if(section_end != line.size() - 1)
                {
                    throw std::logic_error("'" + line.substr(section_end + 1, std::string::npos) + "' was found after end of section definition");
                }

                current_section = line.substr(1, section_end - 1);
                if(current_section == " " || current_section.empty())
                {
                    throw std::logic_error("Section names may not be empty or a single space");
                }

                //Convert section name to lower case
                std::transform(current_section.begin(), current_section.end(), current_section.begin(), ::tolower);
                continue;
            }

            if(current_section == " ")
            {
                throw std::logic_error("Setting does not belong to a section");
            }

            //Find equal pos, splitting key and data
            size_t equal_pos = line.find('=');
            if(equal_pos == std::string::npos)
            {
                throw std::logic_error("Expected '=' after setting name");
            }

            //Extract key & data
            std::string key = line.substr(0, equal_pos);
            std::string data = line.substr(equal_pos + 1, line.size() - equal_pos - 1);

            if(!data.empty())
            {
                //Convert key to lower case
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                //Remove leading & trailing spaces from both
                erase_leading_spaces(key);
                erase_leading_spaces(data);
                erase_trailing_spaces(key);
                erase_trailing_spaces(data);

                //If it's in hex, convert it to decimal
                if(data.size() > 1 && strncasecmp(data.c_str(), "0x", 2) == 0)
                {
                    data = std::to_string(strtol(data.c_str(), nullptr, 0));
                }

                //Erase surrounding quotes if any
                if(data.front() == '"' || data.front() == '\'')
                {
                    if(data.back() == data.front() && data.size() > 1)
                    {
                        data = data.substr(1, data.size() - 2);
                    }
                    else
                    {
                        throw std::logic_error("Malformed string");
                    }
                }
                else if(!(data[0] >= '0' && data[0] <= '9') && !(strncasecmp(data.c_str(), "true", 4) == 0 || strncasecmp(data.c_str(), "false", 4) == 0))
                {
                    throw std::logic_error("String value must be wrapped in quotes");
                }
            }

            //Store in settings
            settings.emplace(current_section + '.' + key, std::move(data));
        }
    }
    catch(const std::exception &e)
    {
        frlog << Log::info << "Config parse failed on line " << lineno + 1 << ": " << e.what() << Log::end;
        return false;
    }

    frlog << Log::info << "Finished parsing config file without issue" << Log::end;
    return true;
}

bool Config::save_to_file(const std::string &filepath)
{
    std::vector<std::string> lines;
    std::map<std::string, std::vector<std::string>> split_config;

    //Split the loaded config back up into multiple sections
    std::lock_guard<std::mutex> guard(lock);
    for(auto &setting : settings)
    {
        std::string name = setting.first.substr(0, setting.first.find('.'));
        split_config[name].emplace_back(setting.first + "=" + setting.second);
    }

    //Write global comments if any
    auto global_comments = comments.find(" ");
    if(global_comments != comments.end())
    {
        for(auto &comment : global_comments->second)
        {
            lines.emplace_back(comment);
        }
        lines.emplace_back();
    }

    //Write sections out with headers
    for(auto &section_name : split_config)
    {
        //Write section name
        lines.emplace_back("[" + section_name.first + "]");

        //Write section comments
        auto section_comments = comments.find(section_name.first);
        if(section_comments != comments.end())
        {
            for(auto &comment : section_comments->second)
            {
                lines.emplace_back(comment);
            }
        }

        //Write section config values
        for(auto &section_entry : section_name.second)
        {
            lines.emplace_back(section_entry.substr(section_name.first.size() + 1, std::string::npos));
            //Insert quotes if needed
            auto equal_pos = lines.back().find('=');
            if(equal_pos == std::string::npos)
                continue;

            std::string setting_value = lines.back().substr(equal_pos + 1, std::string::npos);
            if(setting_value.empty())
                continue;

            if(!(setting_value[0] >= '0' && setting_value[0] <= '9') && !(strncasecmp(setting_value.c_str(), "true", 4) == 0 || strncasecmp(setting_value.c_str(), "false", 4) == 0))
            {
                lines.back().insert(equal_pos + 1, "\"", 1);
                lines.back() += '"';
            }

        }

        //Add a new line before next section
        lines.emplace_back();
    }
    lines.pop_back();

    //Write to file
    return SystemUtilities::write_file(filepath, lines);
}

void Config::delete_section(const std::string &name)
{
    std::lock_guard<std::mutex> guard(lock);
    //Erase section
    for(auto iter = settings.begin(); iter != settings.end();)
    {
        if(iter->first.find(name) == 0)
        {
            iter = settings.erase(iter);
            continue;
        }
        ++iter;
    }

    //Erase section comments
    auto comments_iter = comments.find(name);
    if(comments_iter != comments.end())
        comments.erase(comments_iter);
}

void Config::rename_section(const std::string &current_name, const std::string &new_name)
{
    std::lock_guard<std::mutex> guard(lock);

    //Copy comments
    std::vector<std::string> comment_copy;
    auto comments_iter = comments.find(current_name);
    if(comments_iter != comments.end())
    {
        comment_copy = std::move(comments_iter->second);
        comments.erase(comments_iter);
    }

    //Re-add renamed comments
    if(!comment_copy.empty())
        comments.emplace(new_name, comment_copy);

    //Store old config values, and delete from container
    std::vector<std::pair<std::string, std::string>> renamed_settings;
    for(auto iter = settings.begin(); iter != settings.end();)
    {
        if(iter->first.find(current_name) == 0)
        {
            //Copy it
            renamed_settings.emplace_back(iter->first, std::move(iter->second));

            //Erase old
            iter = settings.erase(iter);
            continue;
        }
        ++iter;
    }

    //Re-insert renamed settings
    for(auto &setting : renamed_settings)
    {
        setting.first.replace(0, current_name.size(), new_name);
        settings.emplace(setting);
    }
}

void Config::delete_setting(const std::string &name)
{
    std::lock_guard<std::mutex> guard(lock);
    auto iter = settings.find(name);
    if(iter != settings.end())
        settings.erase(iter);
}

void Config::rename_setting(const std::string &current_name, const std::string &new_name)
{
    std::lock_guard<std::mutex> guard(lock);
    auto iter = settings.find(current_name);
    if(iter == settings.end())
        return;
    std::string value = iter->second;
    settings.erase(iter);
    settings[new_name] = value;
}
