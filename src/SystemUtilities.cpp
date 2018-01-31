//
// Created by fred on 15/12/17.
//

#include <iostream>
#include <dirent.h>
#include <cstring>
#include "SystemUtilities.h"

std::string SystemUtilities::read_binary_file(const std::string &filepath)
{
    std::ifstream file(filepath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!file.is_open())
        throw std::runtime_error("Couldn't open '" + filepath + "'. Errno: " + std::to_string(errno));

    file.exceptions(std::ios::failbit | std::ios::badbit);
    std::streampos length = file.tellg();
    file.seekg(0, file.beg);
    std::string buff(static_cast<unsigned long>(length), '\0');
    file.read(buff.data(), length);
    return buff;
}

bool SystemUtilities::write_file(const std::string &filepath, const std::vector<std::string> &lines, std::ios_base::openmode mode)
{
    std::ofstream file(filepath, mode);
    if(!file.is_open())
        return false;

    for(const auto &line : lines)
    {
        if(!(file << line << "\n"))
            return false;
    }

    return true;
}

bool SystemUtilities::read_file(const std::string &filepath, std::vector<std::string> &lines, std::ios_base::openmode mode)
{
    //Ensure that it's a file, not a folder
    if(SystemUtilities::get_file_type(filepath) != Attributes::Regular)
        return false;

    //Open the file
    std::ifstream file(filepath, mode);
    if(!file.is_open())
        return false;

    //Read line by line
    std::string line;
    while(std::getline(file, line))
    {
        lines.push_back(std::move(line));
        line.clear();
    }

    return true;
}

Attributes::Type SystemUtilities::get_file_type(const std::string &filepath)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributes(filepath.c_str());
    if(attr == INVALID_FILE_ATTRIBUTES)
        return FilesystemType::Unknown;

    if(attr & FILE_ATTRIBUTE_REPARSE_POINT)
        return FilesystemType::Symlink;
    if(attr & FILE_ATTRIBUTE_DIRECTORY)
        return FilesystemType::Directory;
    return Attributes::Regular;
#else
    struct stat info{};
    int ret = 0;

    ret = lstat(filepath.c_str(), &info);
    if(ret != 0)
        return Attributes::Unknown;

    switch(info.st_mode & S_IFMT)
    {
        case S_IFDIR:
            return Attributes::Directory;
        case S_IFREG:
            return Attributes::Regular;
        case S_IFLNK:
            return Attributes::Symlink;
        default:
            return Attributes::Unknown;
    }
#endif
}

bool SystemUtilities::list_files(const std::string &filepath, std::vector<std::string> &results)
{
    DIR *dir;
    struct dirent *ent;
    if((dir = opendir(filepath.c_str())) != nullptr)
    {
        while((ent = readdir(dir)) != nullptr)
        {
            if(strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
            {
                results.emplace_back(ent->d_name);
            }
        }
        closedir(dir);
    }
    else
    {
        //Couldn't open directory
        return false;
    }
    return true;
}
