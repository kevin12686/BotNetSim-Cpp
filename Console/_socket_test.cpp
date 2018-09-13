#include <iostream>
#include "_socket.h"

using namespace std;

int main() {
    WSADATA wsadata;
    _socket::Debug = true;

    _socket::wsastartup_(&wsadata);

    string data;
    cin >> data;

    for (int i = 0; i < 1; i++) {
        _socket s((char *) "127.0.0.1", (char *) "6666", 1024);

        cout << s.send_((char *) data.c_str()) << endl;
        if (s.check_recv_(1000)) {
            cout << s.recv_() << endl;
        }

        s.shutdown_(_socket::BOTH);
        s.close_();
    }
    _socket::wsacleanup_();
    return 0;
}