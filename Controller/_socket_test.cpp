#include <iostream>
#include <string>
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

    _socketserver *ss = new _socketserver((char *) "8000", 1024);
    while (true) {
        _socket *client = ss->accept_();
        string sss = client->recv_();
        cout << "From " << client->getIPAddr() << " : " << sss << endl;
    }

    _socket::wsacleanup_();

    return 0;
}