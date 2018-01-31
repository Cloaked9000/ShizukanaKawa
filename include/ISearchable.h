//
// Created by fred on 14/01/18.
//

#ifndef SFTPMEDIASTREAMER_ISEARCHEABLE_H
#define SFTPMEDIASTREAMER_ISEARCHEABLE_H
#include <string>
#include <algorithm>

class ISearchable
{
public:
    virtual ~ISearchable()=default;

    /*!
     * Sets the search string of the object
     *
     * @param search_string_ The object's search string
     */
    void set_search_string(std::string search_string_)
    {
        search_string = std::move(search_string_);
    }

    /*!
     * Checks to see if the search_string contains a given
     * substring. This is case insensitive.
     *
     * @param comparing_string The string to compare against
     * @return True if search_string contains comparing_string. False otherwise.
     */
    bool matches(const std::string &comparing_string)
    {
        return std::search(search_string.begin(), search_string.end(), comparing_string.begin(), comparing_string.end(), [](char c1, char c2) {
            return std::toupper(c1) == std::toupper(c2);}) != search_string.end();
    }

private:
    std::string search_string;
};


#endif //SFTPMEDIASTREAMER_ISEARCHEABLE_H
