//
// Created by fred on 15/12/17.
//

#ifndef SFTPMEDIASTREAMER_SYSTEMUTILITIES_H
#define SFTPMEDIASTREAMER_SYSTEMUTILITIES_H

#ifdef _WIN32
#include <dirent.h>
#include <sys/types.h>
#include <windows.h>
#include <Aclapi.h>
#include <sddl.h>
#endif
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <vector>
#include <utime.h>
#include "Types.h"


class SystemUtilities
{
public:
    /*!
     * Portable and fast method of checking if a file OR directory exists
     *
     * @param filepath The filepath of the file/directory to check
     * @return True if the file exists, false otherwise.
     */
    static inline bool does_filepath_exist(const std::string &filepath)
    {
        struct stat buffer = {};
        return (stat(filepath.c_str(), &buffer) == 0);
    }

    /*!
     * Reads a file into memory
     *
     * @throws an std::exception on failure
     * @param filepath The filepath of the file to load
     * @return The file data
     */
    static std::string read_binary_file(const std::string &filepath);

    /*!
     * Reads a file, line by line, into a vector.
     *
     * @param filepath Filepath of the file to open
     * @param lines Where to store the read data
     * @param mode Open mode.
     * @return True on success, false on failure.
     */
    static bool read_file(const std::string &filepath, std::vector<std::string> &lines, std::ios_base::openmode mode = std::ios_base::in);

    /*!
     * Writes a vector, line by line, to the given filepath.
     *
     * @param filepath Where to write the data
     * @param lines The data to write
     * @param mode Open mode
     * @return True on success, false on failure
     */
    static bool write_file(const std::string &filepath, const std::vector<std::string> &lines, std::ios_base::openmode mode = std::ios_base::out);

    /*!
     * Creates a directory at the given filepath
     *
     * @param filepath Where to create the directory
     * @return True if the directory could be created. False otherwise.
     */
    static inline bool create_directory(const std::string &filepath)
    {
        int error = 0;
#ifdef _WIN32
        error = mkdir(filepath.c_str());
#else
        error = mkdir(filepath.c_str(), 0777);
#endif

        return (error == 0);
    }

    /*!
     * Gets the type of object from a filepath
     *
     * @param filepath The filepath to get the type of
     * @return A FilesystemType enum containing the type. Or 'Unknown' on failure.
     */
    static inline Attributes::Type get_file_type(const std::string &filepath);

    /*!
     * Gets a list of files and folders within a given filepath.
     *
     * @param filepath The filepath to get a list of files from
     * @param results Where to store the filename results
     * @return True on success, false on failure.
     */
    static bool list_files(const std::string &filepath, std::vector<std::string> &results);

    /*!
     * Gets the last modification time for a given filepath.
     *
     * @param filepath A filepath to a file to check
     * @return A UNIX timestamp of when the file was last modified. 0 on error.
     */
    static inline time_t get_modification_date(const std::string &filepath)
    {
        struct stat info{};
        if(stat(filepath.c_str(), &info) != 0)
            return 0;

        return info.st_mtime;
    }

};


#endif //SFTPMEDIASTREAMER_SYSTEMUTILITIES_H
