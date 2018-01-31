//
// Created by fred on 07/12/17.
//

#include <stdexcept>
#include <fcntl.h>
#include "../include/SFTPSession.h"

SFTPSession::SFTPSession(SSHConnection *ssh_)
: ssh(ssh_),
  sftp(nullptr)
{
    sftp = sftp_new(ssh->get());
    if(!sftp)
        throw std::runtime_error("sftp_new() failed: " + std::string(ssh_get_error(ssh->get())));

    int ret;
    ret = sftp_init(sftp);
    if(ret != SSH_OK)
        throw std::runtime_error("sftp_init() failed: " + std::to_string(sftp_get_error(sftp)));
}

SFTPSession::~SFTPSession()
{
    if(sftp)
        sftp_free(sftp);
}

std::vector<Attributes> SFTPSession::enumerate_directory(const std::string &filepath)
{
    std::vector<Attributes> ret;

    sftp_dir dir;
    sftp_attributes attributes;
    int error;

    dir = sftp_opendir(sftp, filepath.c_str());
    if(!dir)
        throw std::runtime_error("Failed to open remote directory: " + filepath + ". Error: " + ssh_get_error(ssh->get()));

    while((attributes = sftp_readdir(sftp, dir)) != nullptr)
    {
        Attributes object;
        object.name = attributes->name;
        object.full_name = filepath + "/" + object.name;
        object.type = static_cast<Attributes::Type>(attributes->type);
        sftp_attributes_free(attributes);
        if(object.name != "." && object.name != "..")
            ret.emplace_back(std::move(object));
    }

    if(!sftp_dir_eof(dir))
    {
        sftp_closedir(dir);
        throw std::runtime_error("Failed to completely read remote directory: " + filepath + ". Error: " + ssh_get_error(ssh->get()));
    }
    error = sftp_closedir(dir);
    if(error != SSH_OK)
        throw std::runtime_error("Failed to close directory: " + filepath + ". Error: " + ssh_get_error(ssh->get()));
    return ret;
}

SFTPFile SFTPSession::open(const std::string &filepath)
{
    sftp_file file;
    int access_type;

    access_type = O_RDONLY;
    file = sftp_open(sftp, filepath.c_str(), access_type, 0);
    if(file == nullptr)
        throw std::runtime_error("Failed to open " + filepath + ": " + ssh_get_error(ssh->get()));

    Attributes attributes = stat(filepath);

    return SFTPFile(file, attributes);
}

Attributes SFTPSession::stat(const std::string &filepath)
{
    sftp_attributes attributes;
    if((attributes = sftp_stat(sftp, filepath.c_str())) == nullptr)
        throw std::runtime_error("Failed to stat " + filepath + ": " + ssh_get_error(ssh->get()));

    Attributes attr;
    attr.name = filepath;
    attr.full_name = filepath;
    attr.size = attributes->size;
    attr.type = static_cast<Attributes::Type>(attributes->type);

    sftp_attributes_free(attributes);
    return attr;
}
