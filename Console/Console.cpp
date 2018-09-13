#include <iostream>
#include <vector>
#include <set>
#include "_socketserver.h"
#include "Timer.h"

#define PORT "6666"
#define BUFSIZE 1024
// Mini Seconds
#define IDLE 10000

using namespace std;

// Virtual Timer Thread
DWORD WINAPI virtual_time(LPVOID);

// Server Accept Connection Thread
DWORD WINAPI server_accept(LPVOID);

// Handle The Socket Which Accepted By Server
DWORD WINAPI handle_client(LPVOID);

// Sending Virtual Time To Controler
DWORD WINAPI virtual_broadcast(LPVOID);

int classify_msg(string);

int handle_msg(_socket *);

typedef struct MYTHREAD {
    HANDLE handle;
    DWORD id;
} MyThread;

typedef struct MYTHOST {
    string ip;
    string port;
} HOST;

// Setting
// Growing rate(minutes-VT per 20 bots)
int GROWRATE = 90;
// Time Spreading Delay(mini seconds-RT)
short TSD = 2000;
// MsgType Define
string msg_token[] = {"Request", "HOST", "CTRL", "EXIT", "R"};

// Global Variables
// init
Timer v_t(10.0);
set<HOST *> host_set;
set<HOST *> bot_set;
set<HOST *> crawler_set;
set<HOST *> sensor_set;
set<HOST *> controler_set;

int main() {
    // init
    v_t.setUpdateRate(TSD);
    MyThread timer;
    MyThread server;
    MyThread time_broadcast;

    // debug
    v_t.Debug = true;

    // main
    bool console_on = true;

    // timer start
    timer.handle = CreateThread(NULL, 0, virtual_time, NULL, 0, &(timer.id));
    if (timer.handle) {
        cout << "[INFO] Timer Thread Started." << endl;
        Sleep(500);
    } else {
        cout << "[Error] Timer Thread Unable To Start." << endl;
        console_on = false;
        return 1;
    }

    // socket server
    WSADATA wsadata;
    if (!_socket::wsastartup_(&wsadata)) {
        cout << "[Error] WSAStartup Failed." << endl;
        console_on = false;
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        return 1;
    }
    server.handle = CreateThread(NULL, 0, server_accept, (LPVOID) &console_on, 0, &(server.id));
    if (server.handle) {
        cout << "[INFO] Socket Server Thread Started." << endl;
        Sleep(500);
    } else {
        cout << "[Error] Socket Server Thread Unable To Start." << endl;
        console_on = false;
        _socket::wsacleanup_();
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        return 1;
    }

    // virtual_broadcast
    time_broadcast.handle = CreateThread(NULL, 0, virtual_broadcast, (LPVOID) &console_on, 0, &(time_broadcast.id));
    if (time_broadcast.handle) {
        cout << "[INFO] Time Broadcasting Thread Started." << endl;
        Sleep(500);
    } else {
        cout << "[Error] Time Broadcasting Thread Started." << endl;
        console_on = false;
        WaitForSingleObject(server.handle, INFINITE);
        CloseHandle(server.handle);
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        return 1;
    }

    cout << "Press Enter To Exit." << endl;
    getchar();
    v_t.stop();

    // exit
    _socket::wsacleanup_();
    WaitForSingleObject(server.handle, INFINITE);
    CloseHandle(server.handle);
    WaitForSingleObject(time_broadcast.handle, INFINITE);
    CloseHandle(time_broadcast.handle);
    WaitForSingleObject(timer.handle, INFINITE);
    CloseHandle(timer.handle);
}

DWORD WINAPI virtual_time(LPVOID null) {
    v_t.run();
    return 1;
}

DWORD WINAPI server_accept(LPVOID console) {
    bool *console_on = (bool *) console;
    _socketserver server((char *) PORT, BUFSIZE);
    vector<MyThread *> t_v;
    while (*console_on) {
        if (server.check_connect_(500)) {
            _socket *client = server.accept_();
            if (client) {
                MyThread *temp = new MyThread;
                temp->handle = CreateThread(NULL, 0, handle_client, (LPVOID) client, 0, &(temp->id));
                if (temp->handle) {
                    cout << "[INFO] Client Accepted." << endl;
                    t_v.push_back(temp);
                }
            }
        }
    }

    // exit
    server.close_();
    _socket::wsacleanup_();
    vector<MyThread *>::iterator it_i;
    for (it_i = t_v.begin(); it_i != t_v.end(); it_i++) {
        WaitForSingleObject((*it_i)->handle, INFINITE);
        CloseHandle((*it_i)->handle);
        delete (*it_i);
        t_v.erase(it_i);
    }
    return 1;
}

DWORD WINAPI handle_client(LPVOID s) {
    _socket *client = (_socket *) s;

    // exit
    client->shutdown_(_socket::BOTH);
    client->close_();
    return 1;
}

DWORD WINAPI virtual_broadcast(LPVOID console) {
    bool *console_on = (bool *) console;
    while (*console_on) {
        set<HOST *>::iterator it_i;
        if (!controler_set.empty()) {
            for (it_i = controler_set.begin(); it_i != controler_set.end(); it_i++) {
                _socket client((char *) ((*it_i)->ip).c_str(), (char *) ((*it_i)->port).c_str(), BUFSIZE);
                string time_msg = "T" + v_t.timestamp();
                client.send_((char *) time_msg.c_str());
                client.shutdown_(_socket::BOTH);
                client.close_();
            }
        }
        Sleep(TSD);
    }
    return 1;
}

// Classify The Message Type
int classify_msg(string msg) {
    string from_msg;
    for (int i = 0; i < msg_token->length(); i++) {
        from_msg.assign(msg, 0, msg_token[i].length());
        if (from_msg == msg_token[i])
            return i;
    }
    return -1;
}

int handle_msg(_socket *client) {
    char *msg_ptr = NULL;
    string msg_data;
    if (client->check_recv_(IDLE)) {
        msg_ptr = client->recv_();
        if (msg_ptr) {
            msg_data = msg_ptr;
        }
    } else {
        cout << "[Warning] Socket Timeout or Error." << endl;
    }
    return 1;
}