#include <stdio.h>
#include "_socketserver.h"

bool _socketserver::Debug = false;

_socketserver::_socketserver(char *port, int buffersize) {
    this->Buffersize = buffersize;
    this->Port = port;
    if (init_()) {
        this->status = true;
    }
    if (_socketserver::Debug) {
        printf("[Debug INFO] Socket Server initialized.\n");
    }
}

_socketserver::~_socketserver() {
    this->close_();
}

bool _socketserver::get_status() {
    return this->status;
}

bool _socketserver::check_connect_(int mini_sec) {
    int temp = -1;
    FD_ZERO(&this->readfds);
    FD_SET(this->ListenSocket, &this->readfds);
    this->tv.tv_usec = mini_sec % 1000 * 1000;
    this->tv.tv_sec = (int) (mini_sec / 1000);
    temp = select(this->ListenSocket + 1, &this->readfds, NULL, NULL, &this->tv);
    if (temp == -1) {
        printf("Select error. code:%d\n", GetLastError());
        return false;
    } else if (temp == 0) {
        if (_socketserver::Debug) {
            printf("[Debug INFO] Select timeout.\n");
        }
        return false;
    } else {
        return FD_ISSET(this->ListenSocket, &this->readfds) ? true : false;
    }
}

_socket *_socketserver::accept_() {
    SOCKADDR_IN clientinfo;
    int infosize = sizeof(clientinfo);
    _socket *client = NULL;
    SOCKET ClientSocket = accept(this->ListenSocket, (struct sockaddr *) &clientinfo, &infosize);
    if (ClientSocket == INVALID_SOCKET) {
        //printf("Accept failed with error: %d\n", WSAGetLastError());
        this->close_();
    } else {
        client = new _socket(ClientSocket, inet_ntoa(clientinfo.sin_addr), this->Buffersize);
    }
    return client;
}

int _socketserver::close_() {
    if(this->ListenSocket!=INVALID_SOCKET)
    {
        closesocket(this->ListenSocket);
        this->ListenSocket = INVALID_SOCKET;
    }
    if (_socketserver::Debug) {
        printf("[Debug INFO] Socket Server closed.\n");
    }
    return 1;
}

int _socketserver::init_() {
    int iResult;
    struct addrinfo *result = NULL;

    ZeroMemory(&this->hints, sizeof(this->hints));
    this->hints.ai_family = AF_INET;
    this->hints.ai_socktype = SOCK_STREAM;
    this->hints.ai_protocol = IPPROTO_TCP;
    this->hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo("0.0.0.0", this->Port, &this->hints, &result);
    if (iResult != 0) {
        printf("Getaddrinfo failed with error: %d\n", iResult);
        return 0;
    }

    this->ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (this->ListenSocket == INVALID_SOCKET) {
        printf("Socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        return 0;
    }

    iResult = bind(this->ListenSocket, result->ai_addr, (int) result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        this->close_();
        return 0;
    }

    freeaddrinfo(result);

    iResult = listen(this->ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        this->close_();
        return 0;
    }
    return 1;
}