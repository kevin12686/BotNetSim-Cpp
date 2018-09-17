#include <iostream>
#include <vector>
#include <set>
#include <sstream>
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

vector<string> split(const string &, char);

int classify_msg(string);

int handle_msg(_socket *, string);

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
short TSD = 200;
// MsgType Define
string msg_token[] = {"Request", "HOST", "CTRL", "EXIT", "R"};

// Global Variables
// init
Timer v_t(100.0);
set<HOST *> host_set;
set<HOST *> bot_set;
set<HOST *> crawler_set;
set<HOST *> sensor_set;
set<HOST *> controler_set;
set<HOST *> getcha_set;

int main() {
    // init
    v_t.setUpdateRate(TSD);
    MyThread timer;
    MyThread server;
    MyThread time_broadcast;

    // main
    bool console_on = true;

    // timer start
    timer.handle = CreateThread(NULL, 0, virtual_time, NULL, 0, &(timer.id));
    if (!timer.handle) {
        printf("[Error] Timer Thread Unable To Start.\n");
        console_on = false;
        return 1;
    }

    // socket server
    WSADATA wsadata;
    if (!_socket::wsastartup_(&wsadata)) {
        printf("[Error] WSAStartup Failed.\n");
        console_on = false;
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        return 1;
    }
    server.handle = CreateThread(NULL, 0, server_accept, (LPVOID) &console_on, 0, &(server.id));
    if (!server.handle) {
        printf("[Error] Socket Server Thread Unable To Start.\n");
        console_on = false;
        _socket::wsacleanup_();
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        return 1;
    }

    // virtual_broadcast
    time_broadcast.handle = CreateThread(NULL, 0, virtual_broadcast, (LPVOID) &console_on, 0, &(time_broadcast.id));
    if (!time_broadcast.handle) {
        printf("[Error] Time Broadcasting Thread Started.\n");
        console_on = false;
        WaitForSingleObject(server.handle, INFINITE);
        CloseHandle(server.handle);
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        return 1;
    }

    // Make Sure Threads Are On.
    Sleep(1000);

    // User Interface
    printf("Press Enter To Exit.\n");
    getchar();

    // exit
    console_on = false;
    v_t.stop();
    WaitForSingleObject(server.handle, INFINITE);
    CloseHandle(server.handle);
    WaitForSingleObject(time_broadcast.handle, INFINITE);
    CloseHandle(time_broadcast.handle);
    WaitForSingleObject(timer.handle, INFINITE);
    CloseHandle(timer.handle);
    _socket::wsacleanup_();

    set<HOST *>::iterator it_i;
    if (!host_set.empty()) {
        for (it_i = host_set.begin(); it_i != host_set.end(); host_set.erase(it_i++)) {
            delete *it_i;
        }
    }
    if (!bot_set.empty()) {
        for (it_i = bot_set.begin(); it_i != bot_set.end(); bot_set.erase(it_i++)) {
            delete *it_i;
        }
    }
    if (!crawler_set.empty()) {
        for (it_i = crawler_set.begin(); it_i != crawler_set.end(); crawler_set.erase(it_i++)) {
            delete *it_i;
        }
    }
    if (!sensor_set.empty()) {
        for (it_i = sensor_set.begin(); it_i != sensor_set.end(); sensor_set.erase(it_i++)) {
            delete *it_i;
        }
    }
    if (!controler_set.empty()) {
        for (it_i = controler_set.begin(); it_i != controler_set.end(); controler_set.erase(it_i++)) {
            delete *it_i;
        }
    }
    if (!getcha_set.empty()) {
        for (it_i = getcha_set.begin(); it_i != getcha_set.end(); getcha_set.erase(it_i++)) {
            delete *it_i;
        }
    }
}

DWORD WINAPI virtual_time(LPVOID null) {
    printf("[INFO] Timer Thread Started.\n");
    v_t.run();
    printf("[INFO] Timer Thread Stop.\n");
    return 1;
}

DWORD WINAPI server_accept(LPVOID console) {
    printf("[INFO] Server Thread Started.\n");
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
                    printf("[INFO] Client Accepted.\n");
                    t_v.push_back(temp);
                }
            }
        }
    }

    // exit
    server.close_();
    if (!t_v.empty()) {
        vector<MyThread *>::iterator it_i;
        for (it_i = t_v.begin(); it_i != t_v.end(); t_v.erase(it_i)) {
            WaitForSingleObject((*it_i)->handle, INFINITE);
            CloseHandle((*it_i)->handle);
            delete (*it_i);
        }
    }
    printf("[INFO] Server Thread Stop.\n");
    return 1;
}

DWORD WINAPI handle_client(LPVOID s) {
    printf("[INFO] Client Thread Started.\n");
    _socket *client = (_socket *) s;
    bool recv_loop = true;

    // process
    char *msg_ptr = NULL;
    while (recv_loop) {
        if (client->check_recv_(IDLE)) {
            msg_ptr = client->recv_();
            if (msg_ptr) {
                printf("MSG: %s\n", msg_ptr);
                if (handle_msg(client, msg_ptr) != 0) {
                    recv_loop = false;
                }
            } else {
                recv_loop = false;
            }
        } else {
            recv_loop = false;
            printf("[Warning] Socket Timeout or Error.\n");
        }
    }

    // exit
    client->shutdown_(_socket::BOTH);
    client->close_();
    printf("[INFO] Client Thread Stop.\n");
    return 1;
}

DWORD WINAPI virtual_broadcast(LPVOID console) {
    printf("[INFO] Time Broadcast Thread Started.\n");
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
    printf("[INFO] Time Broadcast Thread Stop.\n");
    return 1;
}

vector<string> split(const string &str, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenstream(str);
    while (getline(tokenstream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
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

int handle_msg(_socket *client, string msg_data) {
    vector<string> arr;
    HOST *create;
    int msg_type_no = classify_msg(msg_data);
    if (msg_type_no < 0)
        printf("[Warning] Message Token Invalid. Msg: %s\n", msg_data.c_str());
    else {
        msg_data.assign(msg_data, msg_token[msg_type_no].length(), msg_data.length() - msg_token[msg_type_no].length());
        switch (msg_type_no) {
            // Request
            case 0:
                break;

                // HOST
            case 1:
                arr = split(msg_data, ':');
                create = new HOST;
                create->ip = arr.at(0);
                create->port = arr.at(1);
                host_set.insert(create);
                client->send_((char *) "OK");
                break;

                // CTRL
            case 2:
                arr = split(msg_data, ':');
                create = new HOST;
                create->ip = arr.at(0);
                create->port = arr.at(1);
                controler_set.insert(create);
                break;

                // EXIT
            case 3:
                break;

                // R
            case 4:
                vector<string> ip_arr;
                arr = split(msg_data, '#');
                arr.erase(arr.begin());
                for (auto it_i:arr) {
                    ip_arr = split(it_i, ':');
                    create = new HOST;
                    create->ip = ip_arr.at(0);
                    create->port = ip_arr.at(1);
                    getcha_set.insert(create);
                }
                break;
        }
    }
    return msg_type_no;
}