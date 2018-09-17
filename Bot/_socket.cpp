#include "_socket.h"

bool _socket::Debug = false;
bool _socket::result_mes = false;
_socket::_socket(SOCKET socket, char *ip, int bufsize) {
    if (_socket::Debug) {
        printf("[Debug INFO] %s connected.\n", ip);
    }
    this->Buffersize = bufsize;
    int len = strlen(ip);
    this->ConnectSocket = socket;
    this->RecvBuf = new char[this->Buffersize]();
    this->New_char_status = true;
    this->IP_Address = new char[len + 1]();
    strcpy_s(this->IP_Address, len + 1, ip);
}

_socket::_socket(char *ip, char *port, int bufsize) {
    this->New_char_status = false;
    this->IP_Address = ip;
    this->Port = port;
    this->Buffersize = bufsize;
    this->init_();
    if (_socket::Debug) {
        printf("[Debug INFO] Socket initialized.\n");
    }
}

_socket::~_socket() {
    this->clean_();
}

char *_socket::recv_() {
    if (_socket::Debug) {
        printf("[Debug INFO] Socket receive.\n");
    }
    char *data = NULL;
    int iResult = recv(this->ConnectSocket, this->RecvBuf, this->Buffersize, 0);
    if (iResult > 0)
        data = this->RecvBuf;
    else if (iResult == 0)
        data = NULL;
    else {
        if(_socket::result_mes)
            printf("Recv failed with error: %d\n", WSAGetLastError());
        data = NULL;
    }


    return data;
}

int _socket::send_(char *data) {
    if (_socket::Debug) {
        printf("[Debug INFO] Socket send.\n");
    }
    int data_len = strlen(data) + 1;
    if (data_len > this->Buffersize) {
        printf("Send data is out of size(%d).\n", this->Buffersize);
        return -1;
    }
    int iResult = send(this->ConnectSocket, data, data_len, 0);
    if (iResult == SOCKET_ERROR) {
        if(_socket::result_mes)
            printf("Send failed with error: %d\n", WSAGetLastError());
        this->close_();
        return -1;
    }
    return iResult;
}

char *_socket::getIPAddr() {
    return this->IP_Address;
}

bool _socket::shutdown_(short option) {
    if (_socket::Debug) {
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
    if (_socket::Debug) {
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
    if (this->RecvBuf != NULL) {
        delete[] this->RecvBuf;
        this->RecvBuf = NULL;
    }

    if (this->New_char_status && this->IP_Address != NULL) {
        delete[] this->IP_Address;
        this->IP_Address = NULL;
    }
}
