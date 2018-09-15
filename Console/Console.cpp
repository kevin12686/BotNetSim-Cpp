#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <algorithm>
#include <ctime>
#include "_socketserver.h"
#include "Timer.h"

#define PORT "6666"
#define BUFSIZE 1024
// Mini Seconds
#define IDLE 10000
// Max Sensor Per Message
#define SensorPerMsg 50

using namespace std;

typedef struct MYTHREAD {
    HANDLE handle;
    DWORD id;
} MyThread;

typedef struct MYTHOST {
    string ip;
    string port;
} HOST;

struct HOSTPtrComp {
    bool operator()(const HOST *lhs, const HOST *rhs) const {
        return lhs->ip + lhs->port < rhs->ip + rhs->port;
    }
};

// Virtual Timer Thread
DWORD WINAPI virtual_time(LPVOID);

// Server Accept Connection Thread
DWORD WINAPI server_accept(LPVOID);

// Handle The Socket Which Accepted By Server
DWORD WINAPI handle_client(LPVOID);

// Sending Virtual Time To Controler
DWORD WINAPI virtual_broadcast(LPVOID);

// Spreading Bot
DWORD WINAPI bot_spreading(LPVOID);

vector<string> split(const string &, char);

int classify_msg(string);

int handle_msg(_socket *, string, HOST *);

int handle_msg(_socket *, string);

int infection(void);

// Setting
// Growing rate(minutes-VT)
int GROWRATE = 30;
// Growing Number
int GROWNUM = 10;
// Start Growing Threshold
int GROWT = 15;
// Initial PeerList Size
int PLSIZE = 3;
// Time Spreading Delay(mini seconds-RT)
short TSD = 200;
// MsgType Define
string msg_token[] = {"Request", "HOST", "CTRL", "EXIT", "R"};

// Global Variables
// init
Timer v_t(100.0);
set<HOST *, HOSTPtrComp> host_set;
set<HOST *, HOSTPtrComp> bot_set;
set<HOST *, HOSTPtrComp> crawler_set;
set<HOST *, HOSTPtrComp> sensor_set;
set<HOST *, HOSTPtrComp> controler_set;
set<HOST *, HOSTPtrComp> getcha_set;

// random

int main() {
    // init
    v_t.setUpdateRate(TSD);
    MyThread timer;
    MyThread server;
    MyThread time_broadcast;
    MyThread spreading;
    srand(unsigned(time(NULL)));

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

    // bot_spreading
    spreading.handle = CreateThread(NULL, 0, bot_spreading, (LPVOID) &console_on, 0, &(spreading.id));
    if (!spreading.handle) {
        printf("[Error] Bot Spreading Thread Started.\n");
        console_on = false;
        WaitForSingleObject(server.handle, INFINITE);
        CloseHandle(server.handle);
        WaitForSingleObject(time_broadcast.handle, INFINITE);
        CloseHandle(time_broadcast.handle);
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        return 1;
    }

    // Make Sure Threads Are On.
    Sleep(1000);

    // User Interface
    printf("\n");
    printf("quit : Stop the Application\ntime_rate : Get Current Time Rate\n");
    printf("update_rate : Get Current Time Update Rate\nlist_host : List Host\n");
    printf("list_bot : List Bot\nlist_ctrl : List Controler\n");
    printf("list_crawler : List Crawler\nlist_sensor : List Sensor\n");
    printf("list_getcha : List Getcha\nglobal : Global Status\n");
    printf("timestamp : Current Timestamp\n");
    string UserCommand = "";
    while (UserCommand != "quit") {
        cin >> UserCommand;
        transform(UserCommand.begin(), UserCommand.end(), UserCommand.begin(), ::tolower);
        if (UserCommand == "time_rate") {
            printf("Current Rate: %f\n", v_t.getRate());
        } else if (UserCommand == "update_rate") {
            printf("Current Rate: %d\n", TSD);
        } else if (UserCommand == "list_host") {
            printf("Host List\n");
            for (auto i : host_set) {
                printf("IP: %s, Port: %s\n", (i->ip).c_str(), (i->port).c_str());
            }
        } else if (UserCommand == "list_bot") {
            printf("Bot List\n");
            for (auto i : bot_set) {
                printf("IP: %s, Port: %s\n", (i->ip).c_str(), (i->port).c_str());
            }
        } else if (UserCommand == "list_ctrl") {
            printf("Controler List\n");
            for (auto i : controler_set) {
                printf("IP: %s, Port: %s\n", (i->ip).c_str(), (i->port).c_str());
            }
        } else if (UserCommand == "list_crawler") {
            printf("Crawler List\n");
            for (auto i : crawler_set) {
                printf("IP: %s, Port: %s\n", (i->ip).c_str(), (i->port).c_str());
            }
        } else if (UserCommand == "list_sensor") {
            printf("Sensor List\n");
            for (auto i : sensor_set) {
                printf("IP: %s, Port: %s\n", (i->ip).c_str(), (i->port).c_str());
            }
        } else if (UserCommand == "list_getcha") {
            printf("Getcha List\n");
            for (auto i : getcha_set) {
                printf("IP: %s, Port: %s\n", (i->ip).c_str(), (i->port).c_str());
            }
        } else if (UserCommand == "global") {
            printf("Host Number: %d\nBot Number: %d\nControler Number: %d\nCrawler Number: %d\nSensor Number: %d\nGetcha Number: %d\n",
                   host_set.size(), bot_set.size(), controler_set.size(), crawler_set.size(), sensor_set.size(),
                   getcha_set.size());
        } else if (UserCommand == "timestamp") {
            printf("timestamp: %s\n", v_t.timestamp().c_str());
        }
    }

    // exit
    console_on = false;
    WaitForSingleObject(server.handle, INFINITE);
    CloseHandle(server.handle);
    WaitForSingleObject(spreading.handle, INFINITE);
    CloseHandle(spreading.handle);
    WaitForSingleObject(time_broadcast.handle, INFINITE);
    CloseHandle(time_broadcast.handle);
    v_t.stop();
    WaitForSingleObject(timer.handle, INFINITE);
    CloseHandle(timer.handle);
    _socket::wsacleanup_();

    set<HOST *, HOSTPtrComp>::iterator it_i;
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
    vector<MyThread *> t_v;
    _socketserver server((char *) PORT, BUFSIZE);
    if (server.get_status()) {
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
    } else
        printf("[Error] Server Can Not Start.");

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
    delete client;
    printf("[INFO] Client Thread Stop.\n");
    return 1;
}

DWORD WINAPI virtual_broadcast(LPVOID console) {
    printf("[INFO] Time Broadcast Thread Started.\n");
    bool *console_on = (bool *) console;
    while (*console_on) {
        set<HOST *, HOSTPtrComp>::iterator it_i;
        if (!controler_set.empty()) {
            for (it_i = controler_set.begin(); it_i != controler_set.end(); it_i++) {
                _socket client((char *) ((*it_i)->ip).c_str(), (char *) ((*it_i)->port).c_str(), BUFSIZE);
                if (client.get_status()) {
                    string time_msg = "T" + v_t.timestamp();
                    if (client.send_((char *) time_msg.c_str()) == -1) {
                        printf("[Warning] Controler %s:%s Time Broadcast Failed.\n", ((*it_i)->ip).c_str(),
                               ((*it_i)->port).c_str());
                    }
                    client.shutdown_(_socket::BOTH);
                } else {
                    printf("[Warning] Controler %s:%s Time Broadcast Failed.\n", ((*it_i)->ip).c_str(),
                           ((*it_i)->port).c_str());
                }
                client.close_();
            }
        }
        Sleep(TSD);
    }
    printf("[INFO] Time Broadcast Thread Stop.\n");
    return 1;
}

DWORD WINAPI bot_spreading(LPVOID console) {
    printf("[INFO] Bot Spreading Thread Started.\n");
    srand(unsigned(time(NULL)));
    bool *console_on = (bool *) console;
    bool letgo = false;
    int time_pass = 0;
    int temp = 0;
    while (*console_on) {
        if (letgo) {
            time_pass += (int) (TSD * v_t.getRate());
            if (time_pass >= GROWRATE * 60000) {
                temp = time_pass / GROWRATE / 60000;
                time_pass %= GROWRATE * 60000;
                for (int i = 0; i < temp; i++) {
                    infection();
                }
            }
        } else if (host_set.size() > GROWT) {
            printf("[INFO] Starting Inflection.\n");
            letgo = true;
            infection();
        }
        Sleep(TSD);
    }
    printf("[INFO] Bot Spreading Thread Stop.\n");
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

int handle_msg(_socket *client, string msg_data, HOST *this_host) {
    vector<string> arr;
    HOST *create;
    int msg_type_no = classify_msg(msg_data);
    if (msg_type_no < 0)
        printf("[Warning] Message Token Invalid. Msg: %s\n", msg_data.c_str());
    else {
        int random_num;
        string output;
        msg_data.assign(msg_data, msg_token[msg_type_no].length(), msg_data.length() - msg_token[msg_type_no].length());
        switch (msg_type_no) {
            // Request
            case 0:
                msg_data.assign(msg_data, 1, msg_data.length() - 1);
                if (true) {
                    set<HOST *, HOSTPtrComp> random_list;
                    if (this_host && msg_data == "Peerlist") {
                        set<HOST *, HOSTPtrComp> my_list(bot_set.begin(), bot_set.end());
                        my_list.erase(my_list.find(this_host));
                        set<HOST *, HOSTPtrComp>::iterator bot_i;
                        output = "Peerlist";
                        while (random_list.size() < PLSIZE) {
                            random_num = rand() % my_list.size();
                            bot_i = my_list.begin();
                            advance(bot_i, random_num);
                            random_list.insert(*bot_i);
                            my_list.erase(bot_i);
                        }
                        for (auto host : random_list) {
                            output += ":" + host->ip + ":" + host->port;
                        }
                        random_list.clear();
                        printf("[INFO] Sending PeerList...(%s)\n", output.c_str());
                        if (client->send_((char *) output.c_str()) == -1) {
                            printf("[Warning] Host %s:%s Sending PeerList Failed.\n", (this_host->ip).c_str(),
                                   (this_host->port).c_str());
                        }
                    } else if (msg_data == "Sensorlist") {
                        set<HOST *, HOSTPtrComp>::iterator sensor_i;
                        output = "Sensorlist";
                        while (random_list.size() < sensor_set.size() > SensorPerMsg ? SensorPerMsg
                                                                                     : sensor_set.size()) {
                            random_num = rand() % sensor_set.size();
                            sensor_i = sensor_set.begin();
                            advance(sensor_i, random_num);
                            random_list.insert(*sensor_i);
                        }

                        for (auto host : random_list) {
                            output += ":" + host->ip + ":" + host->port;
                        }
                        random_list.clear();
                        printf("[INFO] Sending SensorList...(%s)\n", output.c_str());
                        if (client->send_((char *) output.c_str()) == -1) {
                            printf("[Warning] Sending SensorList Failed.\n");
                        }
                    }
                }
                break;

                // HOST
            case 1:
                arr = split(msg_data, ':');
                create = new HOST;
                create->ip = arr.at(0);
                create->port = arr.at(1);
                host_set.insert(create);
                if (client->send_((char *) "OK") == -1) {
                    printf("[Warning] Host %s:%s Register Failed.\n", (create->ip).c_str(), (create->port).c_str());
                }
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

int handle_msg(_socket *client, string msg_data) {
    HOST *this_host = NULL;
    return handle_msg(client, msg_data, this_host);
}

int infection(void) {
    int random_num;
    set<HOST *, HOSTPtrComp>::iterator host_i;
    vector<HOST *> target_list;
    vector<_socket *> client_list;
    HOST *target = NULL;
    string recv_data;
    if (host_set.size() > GROWT && host_set.size() > GROWNUM) {
        printf("[INFO] Infecting...\n");
        for (int i = 0; i < GROWNUM;) {
            random_num = rand() % host_set.size();
            host_i = host_set.begin();
            advance(host_i, random_num);
            target = *host_i;
            _socket *client = new _socket((char *) ((target->ip).c_str()), (char *) ((target->port).c_str()), BUFSIZE);
            if (client->get_status()) {
                if (client->send_((char *) "Change:Bot") == -1) {
                    printf("[Warning] HOST %s:%s Change Bot Failed.\n", (target->ip).c_str(), (target->port).c_str());
                    delete client;
                } else {
                    client_list.push_back(client);
                    target_list.push_back(target);
                    bot_set.insert(target);
                    host_set.erase(host_i);
                    i++;
                }
            } else {
                delete client;
            }
        }

        vector<HOST *>::iterator target_i;
        vector<_socket *>::iterator client_i;

        for (target_i = target_list.begin(), client_i = client_list.begin();
             target_i != target_list.end() && client_i != client_list.end();
             target_list.erase(target_i), client_list.erase(client_i)) {

            // process
            bool recv_loop = true;
            char *msg_ptr = NULL;
            while (recv_loop) {
                if ((*client_i)->check_recv_(IDLE)) {
                    msg_ptr = (*client_i)->recv_();
                    if (msg_ptr) {
                        printf("MSG: %s\n", msg_ptr);
                        int token_id = handle_msg(*client_i, msg_ptr, *target_i);
                        if (token_id != 0) {
                            recv_loop = false;
                            if (token_id != 3) {
                                printf("[Warning] Bot Unable To Start Up.\n");
                                host_set.insert(*target_i);
                                bot_set.erase(bot_set.find(*target_i));
                            }
                        }
                    } else {
                        recv_loop = false;
                        printf("[Warning] Bot Unable To Start Up.\n");
                        host_set.insert(*target_i);
                        bot_set.erase(bot_set.find(*target_i));
                    }
                } else {
                    recv_loop = false;
                    printf("[Warning] Socket Timeout or Error.\n");
                    printf("[Warning] Bot Unable To Start Up.\n");
                    host_set.insert(*target_i);
                    bot_set.erase(bot_set.find(*target_i));
                }
            }

            (*client_i)->shutdown_(_socket::BOTH);
            (*client_i)->close_();
            delete *client_i;
        }
    }
    return 1;
}