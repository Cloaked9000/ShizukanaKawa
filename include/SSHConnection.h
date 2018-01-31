//
// Created by fred on 07/12/17.
//

#ifndef SFTPMEDIASTREAMER_SSHCONNECTION_H
#define SFTPMEDIASTREAMER_SSHCONNECTION_H
#include <string>
#include <libssh/libssh.h>

class SSHConnection
{
public:
    SSHConnection();
    ~SSHConnection();

    /*!
     * Connects to a remote server over SSH
     *
     * @param hostname The hostname of the ssh server
     * @param port The port of the ssh server
     * @param username The username to authenticate as
     */
    void connect(const std::string &hostname, int port, const std::string &username);

    /*!
     * Checks to see if the connection is open
     *
     * @return True if it is, false otherwise
     */
    bool connected();

    /*!
     * Disconnects from the SSH server, ending the session.
     * Does nothing if the session is already disconnected/not valid.
     */
    void disconnect();

    /*!
     * Gets the internal session object
     *
     * @return The session object
     */
    ssh_session get();
private:

    /*!
     * Authenticates the session
     *
     * @param session The session object to authenticate
     * @return True on success, false on failure
     */
    bool authenticate_session(ssh_session session, const std::string &username);

    /*!
     * Verifies the hostname of the session
     *
     * @param session The session to verify the hostname of
     * @return True on success, false on failure
     */
    bool verify_hostname(ssh_session session);

    ssh_session session;
};


#endif //SFTPMEDIASTREAMER_SSHCONNECTION_H
