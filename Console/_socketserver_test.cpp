#include <iostream>
#include <queue>
#include "_socketserver.h"
#include "_thread.h"

using namespace std;

void accept_thread(void);

void client_thread(void);

// Lock
HANDLE QMutex;

// Collect parameters for client_thread
queue<_socket *> ClientQ;

// Collect _thread
vector<_thread *> ClientT;

_socketserver *s;
bool server_status = false;

int main() {
    // 初始化 Mutex
    QMutex = CreateMutex(NULL, false, NULL);
    if (QMutex == NULL) {
        cout << "CreateMutex Error. ErrorCode=" << GetLastError() << endl;
        return 1;
    }

    // 使用Socket一定要先做
    WSADATA wsadata;
    _socket::wsastartup_(&wsadata);

    server_status = true;

    s = new _socketserver((char *) "6666", 1024);

    _thread accept_t((int (*)()) accept_thread);
    accept_t.start();

    Sleep(1000);

    cout << "Press Enter To Exit." << endl;
    getchar();

    server_status = false;
    accept_t.join();

    // 清理垃圾
    delete s;
    _socket::wsacleanup_();

    CloseHandle(QMutex);
    return 0;
}

void accept_thread() {
    bool flag;
    cout << "Accepting..." << endl;
    _thread *t = NULL;
    while (server_status) {
        flag = s->check_connect_(1000);
        if (flag) {
            cout << "Somebody connected." << endl;
            WaitForSingleObject(QMutex, INFINITE);
            ClientQ.push(s->accept_());
            ReleaseMutex(QMutex);
            t = new _thread((int (*)()) client_thread);
            t->start();
            ClientT.push_back(t);
        }
    }
    vector<_thread *>::iterator it_i;
    for (it_i = ClientT.begin(); it_i != ClientT.end(); it_i++) {
        (*it_i)->join();
        delete (*it_i);
    }
    cout << "Stop accepting..." << endl;
}

void client_thread() {
    WaitForSingleObject(QMutex, INFINITE);
    _socket *client = ClientQ.front();
    ClientQ.pop();
    ReleaseMutex(QMutex);
    string s = client->recv_();
    cout << "From " << client->getIPAddr() << " : " << s << endl;
    //client->send_((char *) "Good.");
    client->shutdown_(_socket::BOTH);
    client->close_();
    delete client;
}