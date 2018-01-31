//
// Created by fred on 07/12/17.
//

#ifndef SFTPMEDIASTREAMER_SFTPSESSION_H
#define SFTPMEDIASTREAMER_SFTPSESSION_H

#include <libssh/sftp.h>
#include <vector>
#include "SSHConnection.h"
#include "SFTPFile.h"
#include "Types.h"

class SFTPSession
{
public:
    /*!
     * Starts an SFTP session through an open SSH connection
     *
     * @throws An std::exception on failure
     * @param ssh The SSH connection to use
     */
    explicit SFTPSession(SSHConnection *ssh);
    ~SFTPSession();

    /*!
     * Gets a list of all file names within a remote directory
     *
     * @throws An std::exception on failure
     * @param filepath The filepath of the directory to enumerate
     * @return A list of enumerated objects
     */
    std::vector<Attributes> enumerate_directory(const std::string &filepath);

    /*!
     * Opens a remote file
     *
     * @throws An std::exception on failure
     * @param filepath The filepath to open
     * @return The file on success, throws an std::exception on failure
     */
    SFTPFile open(const std::string &filepath);

    /*!
     * Stats a filepath
     *
     * @throws An std::exception on failure
     * @param filepath Filepath to stat
     * @return Attributes on success. Throws an std::runtime_error on failure
     */
    Attributes stat(const std::string &filepath);
private:
    SSHConnection *ssh;
    sftp_session sftp;
};

#endif //SFTPMEDIASTREAMER_SFTPSESSION_H
