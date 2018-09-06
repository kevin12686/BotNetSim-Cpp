#include <iostream>
#include "_socket.h"

using namespace std;

int main() {
    WSADATA wsadata;
    _socket::Debug = true;

    _socket::wsastartup_(&wsadata);

    for (int i = 0; i < 100; i++) {
        _socket s((char *) "127.0.0.1", (char *) "6666", 1024);
        string data = to_string(i);
        data += " hi, testing";
        cout << s.send_((char *) data.c_str()) << endl;
        cout << s.recv_() << endl;

        s.shutdown_(_socket::BOTH);
        s.close_();
    }
    _socket::wsacleanup_();
    return 0;
}