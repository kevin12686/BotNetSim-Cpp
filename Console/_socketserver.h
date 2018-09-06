#ifndef BOTNETSIM_CPP_SOCKETSERVER_H
#define BOTNETSIM_CPP_SOCKETSERVER_H

#include "_socket.h"

class _socketserver {
public:
    static bool Debug;

    _socketserver(char *, int);

    ~_socketserver();

    // mini second
    bool check_connect_(int);

    _socket * accept_();

    int close_();

private:
    struct timeval tv;
    fd_set readfds;
    int Buffersize;
    char *Port = NULL;
    SOCKET ListenSocket = INVALID_SOCKET;
    struct addrinfo hints;

    int init_();
};


#endif //BOTNETSIM_CPP_SOCKETSERVER_H
