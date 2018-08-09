#include <iostream>
#include "_socket.h"

using namespace std;

int main() {
    _socket s((char *)"127.0.0.1", (char *)"6666", 1024);
    cout << s.send_((char *)"hi, i am testing my code.") << endl;
    cout << s.recv_() << endl;
    s.shutdown_(_socket::BOTH);
    s.close_();
    return 0;
}