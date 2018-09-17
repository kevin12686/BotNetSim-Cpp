#ifndef BOTNETSIM_CPP_SOCKET_H
#define BOTNETSIM_CPP_SOCKET_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

class _socket {
public:
    static bool Debug;
    static bool result_mes;
    static const short BOTH = 0, SEND = 1, RECV = 2;

    bool static wsastartup_(WSADATA *Wsadata) {
        int iResult = WSAStartup(MAKEWORD(2, 2), Wsadata);

        if (iResult != 0) {
            printf("WSAStartup failed with error: %d\n", iResult);
            return false;
        } else {
            return true;
        }
    };

    void static wsacleanup_() {
        WSACleanup();
    };

    _socket(SOCKET, char *, int);

    _socket(char *, char *, int);

    ~_socket();

    char *recv_();

    int send_(char *);

    char *getIPAddr();

    // 0: both, 1: send, 2: recv
    bool shutdown_(short);

    int close_();

private:
    bool New_char_status;
    int Buffersize;
    char *IP_Address = NULL, *Port = NULL, *RecvBuf = NULL;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo hints;

    int init_();

    void clean_();
};

#endif //BOTNETSIM_CPP_SOCKET_H
