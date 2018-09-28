#include <iostream>
#include <string>
#include <tchar.h>
#include "_socket.h"
#include "_socketserver.h"

using namespace std;

int main() {

    WSADATA wsadata;
    _socket::Debug = false;

    _socket::wsastartup_(&wsadata);

    _socket f((char *) "127.0.0.1", (char *) "1999", 1024);
    f.send_((char *) "S8000");
    cout << f.recv_() << endl;
    f.shutdown_(_socket::BOTH);
    f.close_();


    _socket s((char *) "127.0.0.1", (char *) "1999", 1024);
    s.send_((char *) "R127.0.0.1:7777");
    s.shutdown_(_socket::BOTH);
    s.close_();

    _socket d((char *) "127.0.0.1", (char *) "1999", 1024);
    d.send_((char *) "R127.0.0.1:8888");
    d.shutdown_(_socket::BOTH);
    d.close_();

    _socket c((char *) "127.0.0.1", (char *) "1999", 1024);
    c.send_((char *) "R127.0.0.1:9999");
    c.shutdown_(_socket::BOTH);
    c.close_();

    _socketserver::Debug = true;
    _socket::Debug = true;

    HANDLE hFileMap;
    BOOL bResult;
    PCHAR lpBuffer = nullptr;

    hFileMap = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            _T("BotNetShareDateTime")
    );

    if (!hFileMap) {
        printf("OpenFileMapping Failed Error: %d.\n", GetLastError());
        exit(-1);
    } else {
        printf("OpenFileMapping Success!\n");
    }

    while(TRUE){
        lpBuffer = (PCHAR) MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 256);

        if (lpBuffer == nullptr) {
            printf("MapViewOfFile Failed Error: %d.\n", GetLastError());
            break;
        } else {
            printf("MapViewOfFile Success!\n");
        }

        cout << "Get DateTime From Controller: " << lpBuffer << endl;
    }

    bResult = UnmapViewOfFile(lpBuffer);
    if (!bResult) {
        printf("UnmapViewOfFile Failed Error: %d.\n", GetLastError());
        exit(-1);
    }
    else {
        printf("UnmapViewOfFile Success!\n");
    }

    CloseHandle(hFileMap);

    _socket::wsacleanup_();

    return 0;
}