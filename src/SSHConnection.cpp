//
// Created by fred on 07/12/17.
//

#include <libssh/libssh.h>
#include <iostream>
#include <cstring>
#include "../include/SSHConnection.h"

SSHConnection::SSHConnection()
: session(nullptr)
{

}

SSHConnection::~SSHConnection()
{
    if(session)
    {
        if(connected())
            disconnect();
        ssh_free(session);
    }
}


void SSHConnection::connect(const std::string &hostname, int port, const std::string &username)
{

    session = ssh_new();
    if(session == nullptr)
        throw std::runtime_error("ssh_new() failed.");

    ssh_options_set(session, SSH_OPTIONS_HOST, hostname.data());
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);

    int ret = ssh_connect(session);
    if(ret != SSH_OK)
    {
        throw std::runtime_error("ssh_connect() failed: " + std::string(ssh_get_error(session)));
    }

    if(!verify_hostname(session))
    {
        throw std::runtime_error("Host name verification failed");
    }

    if(!authenticate_session(session, username))
    {
        throw std::runtime_error("Authentication failed");
    }
}

bool SSHConnection::connected()
{
    if(!session)
        return false;
    return ssh_is_connected(session) == 1;
}

void SSHConnection::disconnect()
{
    if(connected())
        ssh_disconnect(session);
}


bool SSHConnection::authenticate_session(ssh_session session, const std::string &username)
{
    int ret;
    int method;
    char *banner;

    ret = ssh_userauth_none(session, username.c_str());
    if(ret == SSH_AUTH_ERROR)
        throw std::runtime_error("Session authentication failed: " + std::string(ssh_get_error(session)));

    method = ssh_userauth_list(session, username.c_str());

    //Try to use public key
    if(method & SSH_AUTH_METHOD_PUBLICKEY)
    {
        ret = ssh_userauth_publickey_auto(session, username.c_str(), nullptr);
        if(ret == SSH_AUTH_ERROR)
            throw std::runtime_error("Session authentication failed: " + std::string(ssh_get_error(session)));;
    }
    //Try to use GSSAPI MIC
    else if(method & SSH_AUTH_METHOD_GSSAPI_MIC)
    {
        ret = ssh_userauth_gssapi(session);
        if(ret == SSH_AUTH_ERROR)
            throw std::runtime_error("Session authentication failed: " + std::string(ssh_get_error(session)));
    }
    else
    {
        throw std::runtime_error("Unsupported authentication method. Please use GSSAPI_MIC/Public Key.");
    }

    if(ret != SSH_AUTH_SUCCESS)
        throw std::runtime_error("Unexpected authentication state: " + std::to_string(ret));

    banner = ssh_get_issue_banner(session);
    if(banner)
    {
        std::cout << "MOTD: " << banner << std::endl;
        ssh_string_free_char(banner);
    }

    return true;
}

bool SSHConnection::verify_hostname(ssh_session session)
{
    char *hexa = nullptr;
    int state;
    size_t hlen;
    unsigned char *hash = nullptr;
    ssh_key pub_key;
    int ret;

    state = ssh_is_server_known(session);
    ret = ssh_get_publickey(session, &pub_key);
    if(ret < 0)
        throw std::runtime_error("ssh_get_publickey() failed: " + std::string(ssh_get_error(session)));

    ret = ssh_get_publickey_hash(pub_key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
    ssh_key_free(pub_key);
    if(ret < 0)
        throw std::runtime_error("ssh_get_publickey_hash() failed: " + std::string(ssh_get_error(session)));

    switch(state)
    {
        case SSH_SERVER_KNOWN_OK:
            break;
        case SSH_SERVER_KNOWN_CHANGED:
            std::cout << "Host key for server has changed. Now: ";
            ssh_print_hexa("Public Key Hash", hash, hlen);
            ssh_clean_pubkey_hash(&hash);
            return false;
        case SSH_SERVER_FOUND_OTHER:
            std::cout << "The key for this server has changed. This could be an attack. "
                    "Please remove the public key from your known hosts file if you know the host to be good."
                      << std::endl;
            return false;
        case SSH_SERVER_FILE_NOT_FOUND:
        case SSH_SERVER_NOT_KNOWN:
            hexa = ssh_get_hexa(hash, hlen);
            std::cout << "The server is unknown. Do you want to add this host to you're hosts file? (y/n)" << std::endl;
            std::cout << std::hex << "Hash: " << hexa << std::endl;
            ssh_string_free_char(hexa);
            char add;
            std::cin >> add;
            if(add == 'y')
            {
                if(ssh_write_knownhost(session) < 0)
                {
                    ssh_clean_pubkey_hash(&hash);
                    throw std::runtime_error(
                            "Failed to add public key to known hosts: " + std::string(strerror(errno)));
                }
                std::cout << "Host added successfully." << std::endl;
            }
            else
            {
                ssh_clean_pubkey_hash(&hash);
                return false;
            }
            break;
        case SSH_SERVER_ERROR:
            ssh_clean_pubkey_hash(&hash);
            throw std::runtime_error("Server error: " + std::string(ssh_get_error(session)));
        default:
            ssh_clean_pubkey_hash(&hash);
            throw std::runtime_error("ssh_is_server_known() returned unexpected value: " + std::to_string(state));
    }
    ssh_clean_pubkey_hash(&hash);
    return true;
}

ssh_session SSHConnection::get()
{
    return session;
}
