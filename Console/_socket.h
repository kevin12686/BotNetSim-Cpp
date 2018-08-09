#ifndef BOTNETSIM_CPP_SOCKET_H
#define BOTNETSIM_CPP_SOCKET_H

#include <winsock2.h>
#include <ws2tcpip.h>

class _socket {
public:
    const static short BOTH = 0, SEND = 1, RECV = 2;

    _socket(char *, char *, int);

    ~_socket();

    char *recv_();

    int send_(char *);

    // 0: both, 1: send, 2: recv
    bool shutdown_(short);

    int close_();

private:
    bool Debug = false;
    int Buffersize;
    char *IP_Address, *Port, *RecvBuf;
    WSADATA Wsadata;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo hints;

    int init_();

    void clean_();

};

#endif //BOTNETSIM_CPP_SOCKET_H
