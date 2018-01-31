//
// Created by fred on 08/12/17.
//

#ifndef SFTPMEDIASTREAMER_SFTPSTREAM_H
#define SFTPMEDIASTREAMER_SFTPSTREAM_H


#include <SFML/System/InputStream.hpp>
#include "SFTPFile.h"

class SFTPStream : public sf::InputStream
{
public:

    explicit SFTPStream(SFTPFile *file);

    ////////////////////////////////////////////////////////////
    /// \brief Read data from the stream
    ///
    /// After reading, the stream's reading position must be
    /// advanced by the amount of bytes read.
    ///
    /// \param data Buffer where to copy the read data
    /// \param size Desired number of bytes to read
    ///
    /// \return The number of bytes actually read, or -1 on error
    ///
    ////////////////////////////////////////////////////////////
    sf::Int64 read(void* data, sf::Int64 size) override;

    ////////////////////////////////////////////////////////////
    /// \brief Change the current reading position
    ///
    /// \param position The position to seek to, from the beginning
    ///
    /// \return The position actually sought to, or -1 on error
    ///
    ////////////////////////////////////////////////////////////
    sf::Int64 seek(sf::Int64 position) override;

    ////////////////////////////////////////////////////////////
    /// \brief Get the current reading position in the stream
    ///
    /// \return The current position, or -1 on error.
    ///
    ////////////////////////////////////////////////////////////
    sf::Int64 tell() override;

    ////////////////////////////////////////////////////////////
    /// \brief Return the size of the stream
    ///
    /// \return The total number of bytes available in the stream, or -1 on error
    ///
    ////////////////////////////////////////////////////////////
    sf::Int64 getSize() override;
private:
    SFTPFile *file;
};


#endif //SFTPMEDIASTREAMER_SFTPSTREAM_H
