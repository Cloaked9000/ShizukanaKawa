//
// Created by fred on 08/12/17.
//

#include <iostream>
#include <utility>
#include <cstring>
#include <Log.h>
#include "SFTPFile.h"

SFTPFile::SFTPFile(sftp_file file_, Attributes attributes_)
: file(file_),
  attributes(std::move(attributes_)),
  open(true)
{

}


SFTPFile::~SFTPFile()
{
    close();
}

SFTPFile::SFTPFile(SFTPFile &&other) noexcept
: file(other.file),
  attributes(other.attributes),
  async_buffer(std::move(other.async_buffer)),
  open(other.open),
  async_request(other.async_request)
{
    other.open = false;
    other.file = nullptr;
}


size_t SFTPFile::read_async(void *buff)
{
    int32_t bytes = sftp_async_read(file,
                                    &async_buffer[0],
                                    static_cast<uint32_t>(async_buffer.size()),
                                    static_cast<uint32_t>(async_request));

    if((bytes != SSH_AGAIN && bytes <= 0) || bytes == SSH_EOF)
    {
        close();
    }
    else if(bytes > 0)
    {
        if(buff == nullptr)
            return 0;
        memcpy(buff, async_buffer.c_str(), bytes);
        async_request = sftp_async_read_begin(file, static_cast<uint32_t>(async_buffer.size()));
        return bytes;
    }

    return 0;
}

void SFTPFile::enable_async(size_t buffersz)
{
    async_buffer.resize(buffersz);
    sftp_file_set_nonblocking(file);

    async_request = sftp_async_read_begin(file, static_cast<uint32_t>(buffersz));
}


bool SFTPFile::is_open()
{
    return open;
}

void SFTPFile::close()
{
    if(file)
        sftp_close(file);
    file = nullptr;
    open = false;
}

size_t SFTPFile::tellg()
{
    if(!open)
        return 0;
    return sftp_tell64(file);
}

bool SFTPFile::seekg(size_t offset)
{
    if(!open)
        return false;
    bool ret = sftp_seek64(file, offset) == SSH_OK;
    async_request = sftp_async_read_begin(file, static_cast<uint32_t>(async_buffer.size()));
    return ret;
}

size_t SFTPFile::size()
{
    return attributes.size;
}

size_t SFTPFile::get_async_buffer_size()
{
    return async_buffer.size();
}

ssize_t SFTPFile::read(void *buff, size_t buffsz)
{
    if(!is_open())
        return -1;

    ssize_t actual = sftp_read(file, buff, buffsz);
    if(actual < 0)
    {
        frlog << Log::crit << "Error while reading file: " + std::string(ssh_get_error(file)) << Log::end;
    }

    return actual;
}