//
// Created by fred on 08/12/17.
//

#ifndef SFTPMEDIASTREAMER_SFTPFILE_H
#define SFTPMEDIASTREAMER_SFTPFILE_H


#include <libssh/sftp.h>
#include <string>
#include "Types.h"

class SFTPFile
{
public:
    friend class SFTPSession;
    ~SFTPFile();
    SFTPFile(SFTPFile &&other) noexcept;
    SFTPFile(const SFTPFile &)=delete;
    void operator=(const SFTPFile &)=delete;
    void operator=(SFTPFile &&)=delete;

    /*!
     * Enables async transfers for the file with a given buffer size
     *
     * @param buffersz The buffer size to use
     */
    void enable_async(size_t buffersz = 16384);

    /*!
     * Should be called if enable_async is set to
     * get the next chunk of data. If buffer is empty, and
     * file is not closed, then wait a bit and call again.
     *
     * @param buff The buffer to read in. Should be at least as large as the value passed to enable_async
     * @return The number of bytes read
     */
    size_t read_async(void *buff);

    /*!
     * Reads into a buffer
     *
     * @param buff Buffer to read into
     * @param buffsz Your buffer size
     * @return Number of bytes actually read
     */
    ssize_t read(void *buff, size_t buffsz);

    /*!
     * Checks to see if the file is open or not
     *
     * @return True if it is, false otherwise
     */
    bool is_open();

    /*!
     * Closes the file
     */
    void close();

    /*!
     * Returns read cursor position
     *
     * @return Read cursor offset from beginning of file in bytes
     */
    size_t tellg();

    /*!
     * Seeks to a specific offset within the file
     *
     * @param offset Offset from the beginning of the file to seek to
     * @return True on success, false on failure.
     */
    bool seekg(size_t offset);

    /*!
     * Gets the size in bytes of the file
     *
     * @return The length of the file in bytes
     */
    size_t size();

    /*!
     * Gets the async buffer size. When calling async_read, your
     * buffer needs to be this large.
     *
     * @return Async buffer size
     */
    size_t get_async_buffer_size();
private:
    explicit SFTPFile(sftp_file file, Attributes attributes);
    sftp_file file;
    Attributes attributes;
    std::string async_buffer;
    bool open;
    int async_request;
};


#endif //SFTPMEDIASTREAMER_SFTPFILE_H
