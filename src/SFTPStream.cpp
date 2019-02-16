//
// Created by fred on 08/12/17.
//

#include <iostream>
#include <cstring>
#include "SFTPStream.h"

SFTPStream::SFTPStream(std::unique_ptr<SFTPFile>file_)
: file(std::move(file_))
{

}

sf::Int64 SFTPStream::read(void *data, sf::Int64 size)
{
    return file->read(data, static_cast<size_t>(size));
}

sf::Int64 SFTPStream::seek(sf::Int64 position)
{
    return file->seekg(static_cast<size_t>(position)) ? 0 : -1;
}

sf::Int64 SFTPStream::tell()
{
    return static_cast<sf::Int64>(file->tellg());
}

sf::Int64 SFTPStream::getSize()
{
    return static_cast<sf::Int64>(file->size());
}