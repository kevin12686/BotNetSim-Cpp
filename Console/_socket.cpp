#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "_socket.h"

_socket::_socket(char *ip, char *port, int bufsize) {
    this->IP_Address = ip;
    this->Port = port;
    this->Buffersize = bufsize;
    this->init_();
    if (Debug) {
        printf("[Debug INFO] Socket initialized.\n");
    }
}

_socket::~_socket() {
    this->clean_();
}

char *_socket::recv_() {
    if (Debug) {
        printf("[Debug INFO] Socket receive.\n");
    }
    char *data = NULL;
    int iResult = recv(this->ConnectSocket, this->RecvBuf, this->Buffersize, 0);
    if (iResult > 0)
        data = this->RecvBuf;
    else if (iResult == 0)
        data = NULL;
    else {
        printf("Recv failed with error: %d\n", WSAGetLastError());
        data = NULL;
    }
    return data;
}

int _socket::send_(char *data) {
    if (Debug) {
        printf("[Debug INFO] Socket send.\n");
    }
    int data_len = strlen(data) + 1;
    if (data_len > this->Buffersize) {
        printf("Send data is out of size(%d).\n", this->Buffersize);
        return -1;
    }
    int iResult = send(this->ConnectSocket, data, data_len, 0);
    if (iResult == SOCKET_ERROR) {
        printf("Send failed with error: %d\n", WSAGetLastError());
        this->close_();
        return -1;
    }
    return iResult;
}

bool _socket::shutdown_(short option) {
    if (Debug) {
        printf("[Debug INFO] Socket shutdown.\n");
    }
    int iResult;
    switch (option) {
        case _socket::BOTH:
            iResult = shutdown(this->ConnectSocket, SD_BOTH);
            break;
        case _socket::SEND:
            iResult = shutdown(this->ConnectSocket, SD_SEND);
            break;
        case _socket::RECV:
            iResult = shutdown(this->ConnectSocket, SD_RECEIVE);
            break;
    }
    if (iResult == SOCKET_ERROR) {
        printf("Shutdown failed with error: %d\n", WSAGetLastError());
        this->close_();
        return false;
    }
    return true;
}

int _socket::close_() {
    if (Debug) {
        printf("[Debug INFO] Socket close.\n");
    }
    int result = closesocket(this->ConnectSocket);
    this->clean_();
    return result;
}

int _socket::init_() {
    int iResult;
    struct addrinfo *result = NULL;
    this->RecvBuf = new char[this->Buffersize]();

    iResult = WSAStartup(MAKEWORD(2, 2), &(this->Wsadata));
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 0;
    }

    ZeroMemory(&(this->hints), sizeof(hints));
    this->hints.ai_family = AF_UNSPEC;
    this->hints.ai_socktype = SOCK_STREAM;
    this->hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(this->IP_Address, this->Port, &(this->hints), &result);
    if (iResult != 0) {
        printf("Getaddrinfo failed with error: %d\n", iResult);
        this->clean_();
        return 0;
    }

    for (struct addrinfo *ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        this->ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (this->ConnectSocket == INVALID_SOCKET) {
            printf("Socket failed with error: %ld\n", WSAGetLastError());
            this->clean_();
            return 0;
        }

        // Connect to server.
        iResult = connect(this->ConnectSocket, ptr->ai_addr, (int) ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            this->close_();
            this->ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (this->ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        this->clean_();
        return 0;
    }
    return 1;
}

void _socket::clean_() {
    delete (this->RecvBuf);
    WSACleanup();
}