#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <stdlib.h>
#include "_socketserver.h"
#include "Timer.h"

#define PORT "6666"
#define BUFSIZE 1024
// Mini Seconds
#define IDLE 5000
// Max Sensor Per Message
#define SensorPerMsg 50
#define doc_name "Record.csv"
#define record_rate 1000

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

// Record The History
DWORD WINAPI record(LPVOID);

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

DWORD WINAPI handle_bot_spreading(LPVOID);

DWORD WINAPI change_sensor(LPVOID);

DWORD WINAPI change_crawler(LPVOID);

vector<string> split(const string &, char);

int delay_choose(void);

int classify_msg(string);

int handle_msg(_socket *, string, HOST *);

int handle_msg(_socket *, string);

int infection(bool *);

// Setting
bool show_debug_msg = false;
// Growing rate(minutes-VT)
int GROWRATE = 60;
// Growing Number
int GROWNUM = 20;
// Start Growing Threshold
int GROWT = 25;
// Initial PeerList Size
int PLSIZE = 3;
// Sending Virtual Time
bool time_flag = true;
// Time Spreading Delay(mini seconds-RT)
short TSD = 500;
// MsgType Define
string msg_token[] = {"Request", "HOST", "CTRL", "EXIT", "R"};

// Global Variables
// init
Timer v_t(100.0);
chrono::steady_clock::time_point start_time;
HANDLE action_lock, data_lock;
set<HOST *, HOSTPtrComp> host_set;
set<HOST *, HOSTPtrComp> bot_set;
set<HOST *, HOSTPtrComp> bot_ndelay_list;
set<HOST *, HOSTPtrComp> crawler_set;
set<HOST *, HOSTPtrComp> sensor_set;
set<HOST *, HOSTPtrComp> controler_set;
set<HOST *, HOSTPtrComp> getcha_set;

int main() {
    // init
    start_time = chrono::steady_clock::now();
    v_t.setUpdateRate(TSD);
    MyThread timer;
    MyThread record_t;
    MyThread server;
    MyThread time_broadcast;
    MyThread spreading;
    vector<HANDLE> thread_handle;
    vector<DWORD> thread_id;
    action_lock = CreateMutex(NULL, false, NULL);
    if (action_lock == NULL) {
        printf("[ERROR] CreateMutex Error. ErrorCode=%d", GetLastError());
        return 1;
    }
    data_lock = CreateMutex(NULL, false, NULL);
    if (data_lock == NULL) {
        printf("[ERROR] CreateMutex Error. ErrorCode=%d", GetLastError());
        CloseHandle(action_lock);
        return 1;
    }

    // main
    bool console_on = true;

    // timer start
    timer.handle = CreateThread(NULL, 0, virtual_time, NULL, 0, &(timer.id));
    if (!timer.handle) {
        printf("[Error] Timer Thread Unable To Start.\n");
        console_on = false;
        CloseHandle(action_lock);
        CloseHandle(data_lock);
        return 1;
    }

    record_t.handle = CreateThread(NULL, 0, record, (LPVOID) &console_on, 0, &(record_t.id));
    if (!record_t.handle) {
        printf("[Error] Record Thread Unable To Start.\n");
        console_on = false;
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        CloseHandle(action_lock);
        CloseHandle(data_lock);
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
        WaitForSingleObject(record_t.handle, INFINITE);
        CloseHandle(record_t.handle);
        CloseHandle(action_lock);
        CloseHandle(data_lock);
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
        WaitForSingleObject(record_t.handle, INFINITE);
        CloseHandle(record_t.handle);
        CloseHandle(action_lock);
        CloseHandle(data_lock);
        return 1;
    }

    // virtual_broadcast
    time_broadcast.handle = CreateThread(NULL, 0, virtual_broadcast, (LPVOID) &console_on, 0, &(time_broadcast.id));
    if (!time_broadcast.handle) {
        printf("[Error] Time Broadcasting Thread Started.\n");
        console_on = false;
        WaitForSingleObject(server.handle, INFINITE);
        CloseHandle(server.handle);
        WaitForSingleObject(record_t.handle, INFINITE);
        CloseHandle(record_t.handle);
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        CloseHandle(action_lock);
        CloseHandle(data_lock);
        return 1;
    }

    // bot_spreading
    spreading.handle = CreateThread(NULL, 0, bot_spreading, (LPVOID) &console_on, 0, &(spreading.id));
    if (!spreading.handle) {
        printf("[Error] Bot Spreading Thread Started.\n");
        console_on = false;
        WaitForSingleObject(server.handle, INFINITE);
        CloseHandle(server.handle);
        WaitForSingleObject(record_t.handle, INFINITE);
        CloseHandle(record_t.handle);
        WaitForSingleObject(time_broadcast.handle, INFINITE);
        CloseHandle(time_broadcast.handle);
        v_t.stop();
        WaitForSingleObject(timer.handle, INFINITE);
        CloseHandle(timer.handle);
        CloseHandle(action_lock);
        CloseHandle(data_lock);
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
    printf("timestamp : Current Timestamp\nset_time_rate : Set Time Rate\n");
    printf("set_update_rate : Set Update Rate\nadd_sensor : Add Sensor\n");
    printf("add_crawler : Add Crawler\nsend_time: Toggle Time Sending\n");
    printf("change_bot_num : Show change_bot Number\nset_change_bot_num: Set change_bot Number\n");
    printf("debug : Show debug message\n");
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
        } else if (UserCommand == "set_time_rate") {
            printf("Time Rate: ");
            float rate;
            scanf("%f", &rate);
            v_t.setRate(rate);
        } else if (UserCommand == "set_update_rate") {
            printf("Update Rate: ");
            scanf("%d", &TSD);
        } else if (UserCommand == "add_sensor") {
            int num;
            HANDLE t;
            DWORD id;
            printf("Numbers: ");
            scanf("%d", &num);
            if (num > host_set.size()) {
                printf("[Warning] Number is bigger than host.\n");
            } else {
                t = CreateThread(NULL, 0, change_sensor, new pair<bool *, int>(&console_on, num), 0, &id);
                thread_handle.push_back(t);
                thread_id.push_back(id);
            }
        } else if (UserCommand == "add_crawler") {
            int num;
            HANDLE t;
            DWORD id;
            printf("Numbers: ");
            scanf("%d", &num);
            if (num > host_set.size()) {
                printf("[Warning] Number is bigger than host.\n");
            } else {
                t = CreateThread(NULL, 0, change_crawler, new pair<bool *, int>(&console_on, num), 0, &id);
                thread_handle.push_back(t);
                thread_id.push_back(id);
            }
        } else if (UserCommand == "send_time") {
            if (time_flag) {
                time_flag = false;
            } else {
                time_flag = true;
            }
            printf("send_time: %s\n", time_flag ? "True" : "False");
        } else if (UserCommand == "set_change_bot_num") {
            printf("Number: ");
            scanf("%d", &GROWNUM);
            GROWT = GROWNUM + 5;
        } else if (UserCommand == "change_bot_num") {
            printf("Number: %d\n", GROWNUM);
        } else if (UserCommand == "clear") {
            system("CLS");
        } else if (UserCommand == "debug") {
            if (show_debug_msg) {
                show_debug_msg = false;
            } else {
                show_debug_msg = true;
            }
            printf("debug: %s\n", show_debug_msg ? "True" : "False");
        } else {
            printf("quit : Stop the Application\ntime_rate : Get Current Time Rate\n");
            printf("update_rate : Get Current Time Update Rate\nlist_host : List Host\n");
            printf("list_bot : List Bot\nlist_ctrl : List Controler\n");
            printf("list_crawler : List Crawler\nlist_sensor : List Sensor\n");
            printf("list_getcha : List Getcha\nglobal : Global Status\n");
            printf("timestamp : Current Timestamp\nset_time_rate : Set Time Rate\n");
            printf("set_update_rate : Set Update Rate\nadd_sensor : Add Sensor\n");
            printf("add_crawler : Add Crawler\nsend_time: Toggle Time Sending\n");
            printf("change_bot_num : Show change_bot Number\nset_change_bot_num: Set change_bot Number\n");
            printf("debug : Show debug message\n");
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
    WaitForMultipleObjects((DWORD) thread_handle.size(), &thread_handle[0], true, INFINITE);
    CloseHandle(action_lock);
    CloseHandle(data_lock);
    for (auto i : thread_handle) {
        CloseHandle(i);
    }
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
    bot_ndelay_list.clear();
}

DWORD WINAPI record(LPVOID console_on) {
    printf("[INFO] Record Thread Started.\n");
    bool *console = (bool *) console_on;
    ofstream record;
    record.open(doc_name);
    record << "timestamp, host number, bot number, senser number, crawler number, getcha number" << endl;
    while (*console) {
        record << v_t.timestamp() << ", " << host_set.size() << ", " << bot_set.size() << ", " << sensor_set.size()
               << ", " << crawler_set.size() << ", " << getcha_set.size() << endl;
        Sleep(record_rate);
    }
    record.close();
    printf("[INFO] Record Thread Stop.\n");
    return 1;
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
                        if (show_debug_msg) {
                            printf("[INFO] Client Accepted.\n");
                        }
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
                if (show_debug_msg) {
                    printf("MSG: %s\n", msg_ptr);
                }
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
        if (time_flag && !controler_set.empty()) {
            for (it_i = controler_set.begin(); it_i != controler_set.end() && *console_on; it_i++) {
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
        for (int i = 0; i < TSD; i += 200) {
            if (i + 200 > TSD) {
                Sleep(TSD - i);
            } else {
                Sleep(200);
            }
        }
    }
    printf("[INFO] Time Broadcast Thread Stop.\n");
    return 1;
}

DWORD WINAPI bot_spreading(LPVOID console) {
    printf("[INFO] Bot Spreading Thread Started.\n");
    srand(unsigned(chrono::duration_cast<chrono::nanoseconds>(start_time - chrono::steady_clock::now()).count()));
    bool *console_on = (bool *) console;
    bool letgo = false;
    int time_pass = 0;
    int temp = 0;
    while (*console_on) {
        if (letgo) {
            time_pass += (int) (TSD * v_t.getRate());
            if (time_pass >= GROWRATE * 60000 && host_set.size() > GROWT && host_set.size() > GROWNUM) {
                temp = time_pass / GROWRATE / 60000;
                time_pass %= GROWRATE * 60000;
                // Do it one time
                temp = 1;
                for (int i = 0; i < temp && *console_on; i++) {
                    if (show_debug_msg) {
                        printf("[INFO] Spreading Wait Lock.\n");
                    }
                    WaitForSingleObject(action_lock, INFINITE);
                    if (show_debug_msg) {
                        printf("[INFO] Spreading Get Lock.\n");
                    }
                    infection(console_on);
                    ReleaseMutex(action_lock);
                    if (show_debug_msg) {
                        printf("[INFO] Spreading Release Lock.\n");
                    }
                }
            }
        } else if (host_set.size() > GROWT) {
            letgo = true;
            if (show_debug_msg) {
                printf("[INFO] Spreading Wait Lock.\n");
            }
            WaitForSingleObject(action_lock, INFINITE);
            if (show_debug_msg) {
                printf("[INFO] Spreading Get Lock.\n");
            }
            printf("[INFO] Starting Inflection.\n");
            infection(console_on);
            ReleaseMutex(action_lock);
            if (show_debug_msg) {
                printf("[INFO] Spreading Release Lock.\n");
            }
        }
        Sleep(TSD);
    }
    printf("[INFO] Bot Spreading Thread Stop.\n");
    return 1;
}

DWORD WINAPI handle_bot_spreading(LPVOID client) {
    pair<_socket *, HOST *> *p = (pair<_socket *, HOST *> *) client;
    srand(unsigned(chrono::duration_cast<chrono::nanoseconds>(start_time - chrono::steady_clock::now()).count()));
    _socket *client_i = p->first;
    HOST *target_i = p->second;
    set<HOST *, HOSTPtrComp>::iterator it_i;

    // process
    bool recv_loop = true;
    char *msg_ptr = NULL;
    while (recv_loop) {
        if ((client_i)->check_recv_(IDLE)) {
            msg_ptr = (client_i)->recv_();
            if (msg_ptr) {
                if (show_debug_msg) {
                    printf("MSG: %s\n", msg_ptr);
                }
                int token_id = handle_msg(client_i, msg_ptr, target_i);
                if (token_id != 0) {
                    recv_loop = false;
                    if (token_id != 3) {
                        printf("[Warning] Bot Unable To Start Up.\n");
                        WaitForSingleObject(data_lock, INFINITE);
                        host_set.insert(target_i);
                        bot_set.erase(bot_set.find(target_i));
                        it_i = bot_ndelay_list.find(target_i);
                        if (it_i == bot_ndelay_list.end()) {
                            if (*it_i == target_i) {
                                bot_set.erase(it_i);
                            }
                        } else {
                            bot_set.erase(it_i);
                        }
                        ReleaseMutex(data_lock);
                    }
                }
            } else {
                recv_loop = false;
                printf("[Warning] Bot Unable To Start Up.\n");
                WaitForSingleObject(data_lock, INFINITE);
                host_set.insert(target_i);
                bot_set.erase(bot_set.find(target_i));
                it_i = bot_ndelay_list.find(target_i);
                if (it_i == bot_ndelay_list.end()) {
                    if (*it_i == target_i) {
                        bot_set.erase(it_i);
                    }
                } else {
                    bot_set.erase(it_i);
                }
                ReleaseMutex(data_lock);
            }
        } else {
            recv_loop = false;
            printf("[Warning] Socket Timeout or Error.\n");
            printf("[Warning] Bot Unable To Start Up.\n");
            WaitForSingleObject(data_lock, INFINITE);
            host_set.insert(target_i);
            bot_set.erase(bot_set.find(target_i));
            it_i = bot_ndelay_list.find(target_i);
            if (it_i == bot_ndelay_list.end()) {
                if (*it_i == target_i) {
                    bot_set.erase(it_i);
                }
            } else {
                bot_set.erase(it_i);
            }
            ReleaseMutex(data_lock);
        }
    }

    (client_i)->shutdown_(_socket::BOTH);
    (client_i)->close_();
    delete client_i;
    delete p;
    return 1;
}

DWORD WINAPI change_sensor(LPVOID console_on) {
    if (show_debug_msg) {
        printf("[INFO] Change Sensor Wait Lock.\n");
    }
    WaitForSingleObject(action_lock, INFINITE);
    if (show_debug_msg) {
        printf("[INFO] Change Sensor Get Lock.\n");
    }
    printf("[INFO] Change Sensor Start.\n");
    pair<bool *, int> *p = (pair<bool *, int> *) console_on;
    bool *console = p->first;
    int num = p->second;
    int i, random;
    set<HOST *, HOSTPtrComp>::iterator host_i;
    for (i = 0; i < num && *console; i++) {
        random = rand() % host_set.size();
        host_i = host_set.begin();
        advance(host_i, random);
        HOST *target = *host_i;
        _socket client((char *) (target->ip).c_str(), (char *) (target->port).c_str(), BUFSIZE);
        if (client.get_status()) {
            if (client.send_((char *) "Change:Sensor") == -1) {
                printf("[Warning] %s:%s Send failed.(Sensor)\n", (target->ip).c_str(),
                       (target->port).c_str());
            } else {
                WaitForSingleObject(data_lock, INFINITE);
                sensor_set.insert(target);
                host_set.erase(host_i);
                ReleaseMutex(data_lock);
            }
        } else {
            printf("[Warning] %s:%s Connected failed.(Sensor)\n", (target->ip).c_str(), (target->port).c_str());
        }
        client.shutdown_(_socket::BOTH);
        client.close_();
    }
    ReleaseMutex(action_lock);
    if (show_debug_msg) {
        printf("[INFO] Change Sensor Release Lock.\n");
    }
    delete p;
    return 1;
}

DWORD WINAPI change_crawler(LPVOID console_on) {
    if (show_debug_msg) {
        printf("[INFO] Change Crawler Wait Lock.\n");
    }
    WaitForSingleObject(action_lock, INFINITE);
    if (show_debug_msg) {
        printf("[INFO] Change Crawler Get Lock.\n");
    }
    printf("[INFO] Change Crawler Start.\n");
    pair<bool *, int> *p = (pair<bool *, int> *) console_on;
    bool *console = p->first;
    int num = p->second;
    int i, random;
    set<HOST *, HOSTPtrComp>::iterator host_i;
    for (i = 0; i < num && *console; i++) {
        random = rand() % host_set.size();
        host_i = host_set.begin();
        advance(host_i, random);
        HOST *target = *host_i;
        _socket client((char *) (target->ip).c_str(), (char *) (target->port).c_str(), BUFSIZE);
        if (client.get_status()) {
            if (client.send_((char *) "Change:Crawler") == -1) {
                printf("[Warning] %s:%s Send failed.(Crawler)\n", (target->ip).c_str(),
                       (target->port).c_str());
            } else {
                // process
                bool recv_loop = true;
                char *msg_ptr = NULL;
                while (recv_loop && *console) {
                    if (client.check_recv_(IDLE)) {
                        msg_ptr = client.recv_();
                        if (msg_ptr) {
                            if (show_debug_msg) {
                                printf("MSG: %s\n", msg_ptr);
                            }
                            int token_id = handle_msg(&client, msg_ptr, target);
                            if (token_id == 3) {
                                recv_loop = false;
                                WaitForSingleObject(data_lock, INFINITE);
                                crawler_set.insert(target);
                                host_set.erase(host_i);
                                ReleaseMutex(data_lock);
                            }
                        } else {
                            recv_loop = false;
                            printf("[Warning] Crawler Unable To Start Up.\n");
                        }
                    } else {
                        recv_loop = false;
                        printf("[Warning] Socket Timeout or Error.\n");
                        printf("[Warning] Crawler Unable To Start Up.\n");
                    }
                }
            }
        } else {
            printf("[Warning] %s:%s Connected failed.(Crawler)\n", (target->ip).c_str(), (target->port).c_str());
        }
        client.shutdown_(_socket::BOTH);
        client.close_();
    }
    ReleaseMutex(action_lock);
    if (show_debug_msg) {
        printf("[INFO] Change Crawler Release Lock.\n");
    }
    delete p;
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

// Random Delay Bot
int delay_choose() {
    int random = rand() % 100;
    return random < 50 ? 0 : 1;
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
                        set<HOST *, HOSTPtrComp>::iterator it_find = my_list.find(this_host);
                        if (it_find != my_list.end() && it_find != my_list.end()) {
                            my_list.erase(my_list.find(this_host));
                        }
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
                        if (show_debug_msg) {
                            printf("[INFO] Sending PeerList...(%s)\n", output.c_str());
                        }
                        if (client->send_((char *) output.c_str()) == -1) {
                            printf("[Warning] Host %s:%s Sending PeerList Failed.\n", (this_host->ip).c_str(),
                                   (this_host->port).c_str());
                        }
                    } else if (msg_data == "Sensorlist") {
                        set<HOST *, HOSTPtrComp>::iterator sensor_i;
                        output = "Sensorlist";
                        while (random_list.size() < (sensor_set.size() > SensorPerMsg ? SensorPerMsg
                                                                                      : sensor_set.size())) {
                            random_num = rand() % sensor_set.size();
                            sensor_i = sensor_set.begin();
                            advance(sensor_i, random_num);
                            random_list.insert(*sensor_i);
                        }

                        for (auto host : random_list) {
                            output += ":" + host->ip + ":" + host->port;
                        }
                        random_list.clear();
                        if (show_debug_msg) {
                            printf("[INFO] Sending SensorList...(%s)\n", output.c_str());
                        }
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
                WaitForSingleObject(data_lock, INFINITE);
                host_set.insert(create);
                ReleaseMutex(data_lock);
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
                WaitForSingleObject(data_lock, INFINITE);
                controler_set.insert(create);
                ReleaseMutex(data_lock);
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

int infection(bool *console) {
    int random_num, delay;
    set<HOST *, HOSTPtrComp>::iterator host_i;
    vector<HOST *> target_list;
    vector<_socket *> client_list;
    HOST *target = NULL;
    string recv_data;
    if (host_set.size() > GROWT && host_set.size() > GROWNUM) {
        printf("[INFO] Infecting...\n");
        for (int i = 0; i < GROWNUM && *console;) {
            random_num = rand() % host_set.size();
            host_i = host_set.begin();
            advance(host_i, random_num);
            target = *host_i;
            _socket *client = new _socket((char *) ((target->ip).c_str()), (char *) ((target->port).c_str()), BUFSIZE);
            if (client->get_status()) {
                delay = delay_choose();
                if (client->send_((char *) ("Change:Bot:" + to_string(delay)).c_str()) == -1) {
                    printf("[Warning] HOST %s:%s Change Bot Failed.\n", (target->ip).c_str(), (target->port).c_str());
                    delete client;
                } else {
                    client_list.push_back(client);
                    target_list.push_back(target);
                    WaitForSingleObject(data_lock, INFINITE);
                    bot_set.insert(target);
                    if (delay == 0)
                        bot_ndelay_list.insert(target);
                    host_set.erase(host_i);
                    ReleaseMutex(data_lock);
                    i++;
                }
            } else {
                delete client;
            }
        }

        vector<HOST *>::iterator target_i;
        vector<_socket *>::iterator client_i;
        vector<HANDLE> handle;
        vector<DWORD> tid;

        for (target_i = target_list.begin(), client_i = client_list.begin();
             target_i != target_list.end() && client_i != client_list.end() && *console;
             target_list.erase(target_i), client_list.erase(client_i)) {
            DWORD temp;
            HANDLE t = CreateThread(NULL, 0, handle_bot_spreading,
                                    (LPVOID) new pair<_socket *, HOST *>(*client_i, *target_i), 0, &temp);
            handle.push_back(t);
            tid.push_back(temp);
            Sleep(5);
        }
        WaitForMultipleObjects((DWORD) handle.size(), &handle[0], true, INFINITE);
        for (auto i : handle) {
            CloseHandle(i);
        }
    }
    return 1;
}