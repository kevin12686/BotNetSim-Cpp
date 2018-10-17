#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <queue>
#include <stdlib.h>
#include <pthread.h>
#include "_socketserver.h"
#include "Timer.h"

#define PORT "6666"
#define BUFSIZE 1024
// Mini Seconds
#define IDLE 1000
// Max Sensor Per Message
#define SensorPerMsg 50
#define ServentPerClient 5
// ?% Become Check Bot
#define Check_Bot_Persent 30

#define doc_name "Record.csv"
#define record_rate 1000

using namespace std;

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
void *record(LPVOID);

// Virtual Timer Thread
void *virtual_time(LPVOID);

// Server Accept Connection Thread
void *server_accept(LPVOID);

// Handle The Socket Which Accepted By Server
void *handle_client(LPVOID);

// Sending Virtual Time To Controler
void *virtual_broadcast(LPVOID);

// Spreading Bot
void *bot_spreading(LPVOID);

void *handle_bot_spreading(LPVOID);

void *change_sensor(LPVOID);

void *change_crawler(LPVOID);

void *ban_broadcast(LPVOID);

vector<string> split(const string &, char);

int delay_choose(void);

int promotion_choose(void);

int classify_msg(string);

int handle_msg(_socket *, string, HOST *);

int handle_msg(_socket *, string);

int servent_infection(bool *);

int servent_first_infection(bool *);

// Setting
bool show_debug_msg = false;
// Growing rate(minutes-VT)
int GROWRATE = 60;
// Growing Number
int GROWNUM = 10;
// Start Growing Threshold
int GROWT = 10;
// Initial PeerList Size
int PLSIZE = 3;
// Sending Virtual Time
bool time_flag = true;
// Spreading Bot
bool spreading_flag = false;
// Time Spreading Delay(mini seconds-RT)
short TSD = 500;
// MsgType Define
string msg_token[] = {"ChangeCheckBot", "Promotion", "Request", "HOST", "CTRL", "EXIT", "Ban:", "R"};

// Global Variables
// init
Timer v_t(100.0);
chrono::steady_clock::time_point start_time;
pthread_mutex_t action_lock = PTHREAD_MUTEX_INITIALIZER, data_lock = PTHREAD_MUTEX_INITIALIZER, ban_lock = PTHREAD_MUTEX_INITIALIZER;
set<HOST *, HOSTPtrComp> host_set;
set<HOST *, HOSTPtrComp> bot_set;
set<HOST *, HOSTPtrComp> servent_bot_set;
set<HOST *, HOSTPtrComp> sleep_bot_set;
set<HOST *, HOSTPtrComp> check_bot_set;
set<HOST *, HOSTPtrComp> crawler_set;
set<HOST *, HOSTPtrComp> sensor_set;
set<HOST *, HOSTPtrComp> controler_set;
set<HOST *, HOSTPtrComp> getcha_set;
set<HOST *, HOSTPtrComp> ban_set;

queue<string> ban_queue;

int main() {
    // init
    start_time = chrono::steady_clock::now();
    v_t.setUpdateRate(TSD);

    int result = 0;
    pthread_t timer, record_t, server, time_broadcast, spreading, ban;
    vector<pthread_t> thread_handle;

    // main
    bool console_on = true;

    // timer start
    result = pthread_create(&timer, NULL, virtual_time, NULL);
    if (result) {
        printf("[Error] Timer Thread Unable To Start.\n");
        console_on = false;
        return 1;
    }

    result = pthread_create(&record_t, NULL, record, (LPVOID) &console_on);
    if (result) {
        printf("[Error] Record Thread Unable To Start.\n");
        console_on = false;
        v_t.stop();
        pthread_join(timer, NULL);
        return 1;
    }


    // socket server
    WSADATA wsadata;
    if (!_socket::wsastartup_(&wsadata)) {
        printf("[Error] WSAStartup Failed.\n");
        console_on = false;
        v_t.stop();
        pthread_join(timer, NULL);
        pthread_join(record_t, NULL);
        return 1;
    }
    result = pthread_create(&server, NULL, server_accept, (LPVOID) &console_on);
    if (result) {
        printf("[Error] Socket Server Thread Unable To Start.\n");
        console_on = false;
        _socket::wsacleanup_();
        v_t.stop();
        pthread_join(timer, NULL);
        pthread_join(record_t, NULL);
        return 1;
    }

    // virtual_broadcast
    result = pthread_create(&time_broadcast, NULL, virtual_broadcast, (LPVOID) &console_on);
    if (result) {
        printf("[Error] Time Broadcasting Unable To Start.\n");
        console_on = false;
        pthread_join(server, NULL);
        pthread_join(record_t, NULL);
        v_t.stop();
        pthread_join(timer, NULL);
        return 1;
    }

    // bot_spreading
    result = pthread_create(&spreading, NULL, bot_spreading, (LPVOID) &console_on);
    if (result) {
        printf("[Error] Bot Spreading Unable To Start.\n");
        console_on = false;
        pthread_join(server, NULL);
        pthread_join(record_t, NULL);
        pthread_join(time_broadcast, NULL);
        v_t.stop();
        pthread_join(timer, NULL);
        return 1;
    }

    result = pthread_create(&ban, NULL, ban_broadcast, (LPVOID) &console_on);
    if (result) {
        printf("[Error] Ban Broadcast Unable To Start.\n");
        console_on = false;
        pthread_join(server, NULL);
        pthread_join(spreading, NULL);
        pthread_join(record_t, NULL);
        pthread_join(time_broadcast, NULL);
        v_t.stop();
        pthread_join(timer, NULL);
        return 1;
    }

    // Make Sure Threads Are On.
    Sleep(1000);

    // User Interface
    printf("\n");
    printf("quit : Stop the Application\ntime_rate : Get Current Time Rate\n");
    printf("update_rate : Get Current Time Update Rate\nlist_host : List Host\n");
    printf("list_bot : List Bot\nlist_ctrl : List Controler\n");
    printf("list_servent_bot : List Servent_Bot\nlist_sleep_bot : List Sleep_Bot\n");
    printf("list_check_bot : List Check_Bot\nlist_ban_bot : List Ban_Bot\n");
    printf("list_crawler : List Crawler\nlist_sensor : List Sensor\n");
    printf("list_getcha : List Getcha\nglobal : Global Status\n");
    printf("timestamp : Current Timestamp\nset_time_rate : Set Time Rate\n");
    printf("set_update_rate : Set Update Rate\nadd_sensor : Add Sensor\n");
    printf("add_crawler : Add Crawler\nsend_time: Toggle Time Sending\n");
    printf("change_bot_num : Show change_bot Number\nset_change_bot_num: Set change_bot Number\n");
    printf("debug : Show debug message\nspreading: Toggle Spreading\n");
    string UserCommand = "";
    while (UserCommand != "quit") {
        cin >> UserCommand;
        transform(UserCommand.begin(), UserCommand.end(), UserCommand.begin(), ::tolower);
        if (UserCommand == "time_rate") {
            printf("Current Rate: %d\n", v_t.getRate());
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
        } else if (UserCommand == "list_servent_bot") {
            printf("Servent_Bot List\n");
            for (auto i : servent_bot_set) {
                printf("IP: %s, Port: %s\n", (i->ip).c_str(), (i->port).c_str());
            }
        } else if (UserCommand == "list_sleep_bot") {
            printf("Sleep_Bot List\n");
            for (auto i : sleep_bot_set) {
                printf("IP: %s, Port: %s\n", (i->ip).c_str(), (i->port).c_str());
            }
        } else if (UserCommand == "list_check_bot") {
            printf("Check_Bot List\n");
            for (auto i : check_bot_set) {
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
        } else if (UserCommand == "list_ban_bot") {
            printf("Ban Bot List\n");
            for (auto i : ban_set) {
                printf("IP: %s, Port: %s\n", (i->ip).c_str(), (i->port).c_str());
            }
        } else if (UserCommand == "global") {
            printf("Host Number: %d\nBot Number: %d\nSevent_Bot Number: %d\nSleep_Bot Number: %d\nCheck_Bot Number: %d\nControler Number: %d\nCrawler Number: %d\nSensor Number: %d\nGetcha Number: %d\nBan Number: %d\n",
                   host_set.size(), bot_set.size(), servent_bot_set.size(), sleep_bot_set.size(), check_bot_set.size(),
                   controler_set.size(), crawler_set.size(), sensor_set.size(),
                   getcha_set.size(), ban_set.size());
        } else if (UserCommand == "timestamp") {
            printf("timestamp: %s\n", v_t.timestamp().c_str());
        } else if (UserCommand == "set_time_rate") {
            printf("Time Rate: ");
            int rate;
            scanf("%d", &rate);
            v_t.setRate(rate);
        } else if (UserCommand == "set_update_rate") {
            printf("Update Rate: ");
            scanf("%d", &TSD);
        } else if (UserCommand == "add_sensor") {
            int num;
            pthread_t t;
            printf("Numbers: ");
            scanf("%d", &num);
            if (num > host_set.size()) {
                printf("[Warning] Number is bigger than host.\n");
            } else {
                result = pthread_create(&t, NULL, change_sensor, (LPVOID) (new pair<bool *, int>(&console_on, num)));
                if (!result) {
                    thread_handle.push_back(t);
                } else {
                    printf("[Error] Create Pthread Failed.\n");
                }
            }
        } else if (UserCommand == "add_crawler") {
            int num;
            pthread_t t;
            printf("Numbers: ");
            scanf("%d", &num);
            if (num > host_set.size()) {
                printf("[Warning] Number is bigger than host.\n");
            } else {
                result = pthread_create(&t, NULL, change_crawler, (LPVOID) (new pair<bool *, int>(&console_on, num)));
                if (!result) {
                    thread_handle.push_back(t);
                } else {
                    printf("[Error] Create Pthread Failed.\n");
                }
            }
        } else if (UserCommand == "send_time") {
            if (time_flag) {
                time_flag = false;
            } else {
                time_flag = true;
            }
            printf("send_time: %s\n", time_flag ? "True" : "False");
        } else if (UserCommand == "spreading") {
            if (spreading_flag) {
                spreading_flag = false;
            } else {
                spreading_flag = true;
            }
            printf("spreading_flag: %s\n", spreading_flag ? "True" : "False");
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
            printf("list_servent_bot : List Servent_Bot\nlist_sleep_bot : List Sleep_Bot\n");
            printf("list_check_bot : List Check_Bot\nlist_ban_bot : List Ban_Bot\n");
            printf("list_crawler : List Crawler\nlist_sensor : List Sensor\n");
            printf("list_getcha : List Getcha\nglobal : Global Status\n");
            printf("timestamp : Current Timestamp\nset_time_rate : Set Time Rate\n");
            printf("set_update_rate : Set Update Rate\nadd_sensor : Add Sensor\n");
            printf("add_crawler : Add Crawler\nsend_time: Toggle Time Sending\n");
            printf("change_bot_num : Show change_bot Number\nset_change_bot_num: Set change_bot Number\n");
            printf("debug : Show debug message\nspreading: Toggle Spreading\n");
        }
    }

    // exit
    console_on = false;
    pthread_join(server, NULL);
    pthread_join(spreading, NULL);
    pthread_join(ban, NULL);
    pthread_join(time_broadcast, NULL);
    v_t.stop();
    pthread_join(timer, NULL);
    for (auto i : thread_handle) {
        pthread_join(i, NULL);
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
    if (!ban_set.empty()) {
        for (it_i = ban_set.begin(); it_i != ban_set.end(); ban_set.erase(it_i++)) {
            delete *it_i;
        }
    }
    servent_bot_set.clear();
}

void *record(LPVOID console_on) {
    printf("[INFO] Record Thread Started.\n");
    bool *console = (bool *) console_on;
    ofstream record;
    record.open(doc_name);
    record
            << "timestamp, host number, bot number, servent number, client number, sleep number, check number, senser number, crawler number, getcha number, ban number"
            << endl;
    while (*console) {
        record << v_t.timestamp() << ", " << host_set.size() << ", " << bot_set.size() << ", " << servent_bot_set.size()
               << ", " << bot_set.size() - servent_bot_set.size() - sleep_bot_set.size() << ", " << sleep_bot_set.size()
               << ", " << check_bot_set.size() << ", " << sensor_set.size()
               << ", " << crawler_set.size() << ", " << getcha_set.size() << ", " << ban_set.size() << endl;
        Sleep(record_rate);
    }
    record.close();
    printf("[INFO] Record Thread Stop.\n");
    pthread_exit(NULL);
    return NULL;
}

void *virtual_time(LPVOID null) {
    printf("[INFO] Timer Thread Started.\n");
    v_t.run();
    printf("[INFO] Timer Thread Stop.\n");
    pthread_exit(NULL);
    return NULL;
}

void *server_accept(LPVOID console) {
    printf("[INFO] Server Thread Started.\n");
    bool *console_on = (bool *) console;
    vector<pthread_t *> t_v;
    _socketserver server((char *) PORT, BUFSIZE);
    if (server.get_status()) {
        while (*console_on) {
            if (server.check_connect_(500)) {
                _socket *client = server.accept_();
                if (client) {
                    //MyThread *temp = new MyThread;
                    int result = 0;
                    pthread_t *temp = new pthread_t;
                    result = pthread_create(temp, NULL, handle_client, (LPVOID) client);
                    if (!result) {
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
        vector<pthread_t *>::iterator it_i;
        for (it_i = t_v.begin(); it_i != t_v.end(); t_v.erase(it_i)) {
            pthread_join(**it_i, NULL);
            delete (*it_i);
        }
    }
    printf("[INFO] Server Thread Stop.\n");
    pthread_exit(NULL);
    return NULL;
}

void *handle_client(LPVOID s) {
    srand(unsigned(chrono::duration_cast<chrono::nanoseconds>(start_time - chrono::steady_clock::now()).count()));
    if (show_debug_msg) {
        printf("[INFO] Client Thread Started.\n");
    }
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
                if (handle_msg(client, msg_ptr) != 1) {
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
    if (show_debug_msg) {
        printf("[INFO] Client Thread Stop.\n");
    }
    pthread_exit(NULL);
    return NULL;
}

void *virtual_broadcast(LPVOID console) {
    printf("[INFO] Time Broadcast Thread Started.\n");
    bool *console_on = (bool *) console;
    while (*console_on) {
        set<HOST *, HOSTPtrComp>::iterator it_i;
        if (time_flag && !controler_set.empty()) {
            for (it_i = controler_set.begin(); it_i != controler_set.end() && *console_on; it_i++) {
                _socket client((char *) ((*it_i)->ip).c_str(), (char *) ((*it_i)->port).c_str(), BUFSIZE);
                if (client.get_status()) {
                    string time_msg = "T" + v_t.timestamp() + ":" + to_string(v_t.getRate());
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
    pthread_exit(NULL);
    return NULL;
}

void *bot_spreading(LPVOID console) {
    printf("[INFO] Bot Spreading Thread Started.\n");
    srand(unsigned(chrono::duration_cast<chrono::nanoseconds>(start_time - chrono::steady_clock::now()).count()));
    bool *console_on = (bool *) console;
    bool letgo = false;
    int time_pass = 0;
    int temp = 0;
    while (*console_on) {
        if (letgo && spreading_flag) {
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
                    pthread_mutex_lock(&action_lock);
                    if (show_debug_msg) {
                        printf("[INFO] Spreading Get Lock.\n");
                    }
                    servent_infection(console_on);
                    pthread_mutex_unlock(&action_lock);
                    if (show_debug_msg) {
                        printf("[INFO] Spreading Release Lock.\n");
                    }
                }
            }
        } else if (host_set.size() > GROWT && spreading_flag) {
            letgo = true;
            if (show_debug_msg) {
                printf("[INFO] Spreading Wait Lock.\n");
            }
            pthread_mutex_lock(&action_lock);
            if (show_debug_msg) {
                printf("[INFO] Spreading Get Lock.\n");
            }
            printf("[INFO] Starting Inflection.\n");
            servent_first_infection(console_on);
            pthread_mutex_unlock(&action_lock);
            if (show_debug_msg) {
                printf("[INFO] Spreading Release Lock.\n");
            }
        }
        Sleep(TSD);
    }
    printf("[INFO] Bot Spreading Thread Stop.\n");
    pthread_exit(NULL);
    return NULL;
}

void *handle_bot_spreading(LPVOID client) {
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
                if (token_id != 2) {
                    recv_loop = false;
                    if (token_id != 5) {
                        printf("[Warning] Bot Unable To Start Up.\n");
                        pthread_mutex_lock(&data_lock);
                        host_set.insert(target_i);
                        bot_set.erase(bot_set.find(target_i));
                        it_i = servent_bot_set.find(target_i);
                        if (it_i == servent_bot_set.end()) {
                            if (*it_i == target_i) {
                                servent_bot_set.erase(it_i);
                            }
                        } else {
                            servent_bot_set.erase(it_i);
                        }
                        pthread_mutex_unlock(&data_lock);
                    }
                }
            } else {
                recv_loop = false;
                printf("[Warning] Bot Unable To Start Up.\n");
                pthread_mutex_lock(&data_lock);
                host_set.insert(target_i);
                bot_set.erase(bot_set.find(target_i));
                it_i = servent_bot_set.find(target_i);
                if (it_i == servent_bot_set.end()) {
                    if (*it_i == target_i) {
                        bot_set.erase(it_i);
                    }
                } else {
                    bot_set.erase(it_i);
                }
                pthread_mutex_unlock(&data_lock);
            }
        } else {
            recv_loop = false;
            printf("[Warning] Socket Timeout or Error.\n");
            printf("[Warning] Bot Unable To Start Up.\n");
            pthread_mutex_lock(&data_lock);
            host_set.insert(target_i);
            bot_set.erase(bot_set.find(target_i));
            it_i = servent_bot_set.find(target_i);
            if (it_i == servent_bot_set.end()) {
                if (*it_i == target_i) {
                    servent_bot_set.erase(it_i);
                }
            } else {
                servent_bot_set.erase(it_i);
            }
            pthread_mutex_unlock(&data_lock);
        }
    }

    (client_i)->shutdown_(_socket::BOTH);
    (client_i)->close_();
    delete client_i;
    delete p;
    pthread_exit(NULL);
    return NULL;
}

void *change_sensor(LPVOID console_on) {
    if (show_debug_msg) {
        printf("[INFO] Change Sensor Wait Lock.\n");
    }
    pthread_mutex_lock(&action_lock);
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
        random = (rand() * rand()) % host_set.size();
        host_i = host_set.begin();
        advance(host_i, random);
        HOST *target = *host_i;
        _socket client((char *) (target->ip).c_str(), (char *) (target->port).c_str(), BUFSIZE);
        if (client.get_status()) {
            if (client.send_((char *) "Change:Sensor") == -1) {
                printf("[Warning] %s:%s Send failed.(Sensor)\n", (target->ip).c_str(),
                       (target->port).c_str());
            } else {
                pthread_mutex_lock(&data_lock);
                sensor_set.insert(target);
                host_set.erase(host_i);
                pthread_mutex_unlock(&data_lock);
            }
        } else {
            printf("[Warning] %s:%s Connected failed.(Sensor)\n", (target->ip).c_str(), (target->port).c_str());
        }
        client.shutdown_(_socket::BOTH);
        client.close_();
    }
    pthread_mutex_unlock(&action_lock);
    if (show_debug_msg) {
        printf("[INFO] Change Sensor Release Lock.\n");
    }
    delete p;
    pthread_exit(NULL);
    return NULL;
}

void *change_crawler(LPVOID console_on) {
    if (show_debug_msg) {
        printf("[INFO] Change Crawler Wait Lock.\n");
    }
    pthread_mutex_lock(&action_lock);
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
        random = (rand() * rand()) % host_set.size();
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
                            if (token_id == 5) {
                                recv_loop = false;
                                pthread_mutex_lock(&data_lock);
                                crawler_set.insert(target);
                                host_set.erase(host_i);
                                pthread_mutex_unlock(&data_lock);
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
    pthread_mutex_unlock(&action_lock);
    if (show_debug_msg) {
        printf("[INFO] Change Crawler Release Lock.\n");
    }
    delete p;
    pthread_exit(NULL);
    return NULL;
}

void *ban_broadcast(LPVOID console_on) {
    printf("[INFO] Ban Broadcast Started.\n");
    bool *console = (bool *) console_on;
    while (*console) {
        set<HOST *, HOSTPtrComp>::iterator it_i;
        if (!ban_queue.empty() && !controler_set.empty()) {
            for (it_i = controler_set.begin(); it_i != controler_set.end() && *console; it_i++) {
                _socket client((char *) ((*it_i)->ip).c_str(), (char *) ((*it_i)->port).c_str(), BUFSIZE);
                if (client.get_status()) {
                    string time_msg = ban_queue.front();
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
                pthread_mutex_lock(&ban_lock);
                ban_queue.pop();
                pthread_mutex_unlock(&ban_lock);
            }
        } else {
            Sleep(TSD);
        }
    }
    printf("[INFO] Ban Broadcast Stop.\n");
    pthread_exit(NULL);
    return NULL;
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
    return random < 10 ? 0 : 1;
}

// Check Bot & Sleep Bot Selection
int promotion_choose(void) {
    int random = rand() % 100;
    return random < Check_Bot_Persent ? 0 : 1;
};

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
        int random_num, psize;
        string output;
        msg_data.assign(msg_data, msg_token[msg_type_no].length(), msg_data.length() - msg_token[msg_type_no].length());
        switch (msg_type_no) {
            // ChangeCheckBot
            case 0:
                arr = split(msg_data, ':');
                for (auto i:servent_bot_set) {
                    if (i->ip == arr.at(0) && i->port == arr.at(1)) {
                        pthread_mutex_lock(&data_lock);
                        check_bot_set.insert(i);
                        servent_bot_set.erase(servent_bot_set.find(i));
                        pthread_mutex_unlock(&data_lock);
                        break;
                    }
                }
                break;

                // Promotion
            case 1:
                arr = split(msg_data, ':');
                if (promotion_choose() == 0) {
                    // Check Bot
                    if (client->send_((char *) ("Accumulate")) == -1) {
                        printf("[Warning] Sending %s:%s \"Accumulate\" Failed.\n", arr.at(0).c_str(),
                               arr.at(1).c_str());
                    }
                } else {
                    // Sleep Bot
                    if (client->send_((char *) ("Change:SleepBot")) == -1) {
                        printf("[Warning] %s:%s Unable To Become Sleep Bot.\n", arr.at(0).c_str(), arr.at(1).c_str());
                    } else {
                        for (auto i:servent_bot_set) {
                            if (i->ip == arr.at(0) && i->port == arr.at(1)) {
                                pthread_mutex_lock(&data_lock);
                                sleep_bot_set.insert(i);
                                servent_bot_set.erase(servent_bot_set.find(i));
                                pthread_mutex_unlock(&data_lock);
                                break;
                            }
                        }
                    }
                }
                break;

                // Request
            case 2:
                msg_data.assign(msg_data, 1, msg_data.length() - 1);
                if (true) {
                    set<HOST *, HOSTPtrComp> random_list;
                    if (this_host && msg_data == "Peerlist") {
                        set<HOST *, HOSTPtrComp> my_list(servent_bot_set.begin(), servent_bot_set.end());
                        set<HOST *, HOSTPtrComp>::iterator it_find = my_list.find(this_host);
                        if (it_find != my_list.end() && it_find != my_list.end()) {
                            my_list.erase(my_list.find(this_host));
                        }
                        set<HOST *, HOSTPtrComp>::iterator bot_i;
                        output = "Peerlist";
                        while (random_list.size() < PLSIZE) {
                            random_num = (rand() * rand()) % my_list.size();
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
                            random_num = (rand() * rand()) % sensor_set.size();
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
                    } else if (msg_data == "Serventlist") {
                        set<HOST *, HOSTPtrComp>::iterator servent_i;
                        output = "Serventlist";
                        while (random_list.size() < (servent_bot_set.size() > ServentPerClient ? ServentPerClient
                                                                                               : servent_bot_set.size())) {
                            random_num = (rand() * rand()) % servent_bot_set.size();
                            servent_i = servent_bot_set.begin();
                            advance(servent_i, random_num);
                            random_list.insert(*servent_i);
                        }

                        for (auto host : random_list) {
                            output += ":" + host->ip + ":" + host->port;
                        }
                        random_list.clear();
                        if (show_debug_msg) {
                            printf("[INFO] Sending ServentList...(%s)\n", output.c_str());
                        }
                        if (client->send_((char *) output.c_str()) == -1) {
                            printf("[Warning] Sending ServentList Failed.\n");
                        }
                    }
                }
                break;

                // HOST
            case 3:
                arr = split(msg_data, ':');
                create = new HOST;
                create->ip = arr.at(0);
                create->port = arr.at(1);
                pthread_mutex_lock(&data_lock);
                host_set.insert(create);
                pthread_mutex_unlock(&data_lock);
                if (client->send_((char *) "OK") == -1) {
                    printf("[Warning] Host %s:%s Register Failed.\n", (create->ip).c_str(), (create->port).c_str());
                }
                break;

                // CTRL
            case 4:
                arr = split(msg_data, ':');
                create = new HOST;
                create->ip = arr.at(0);
                create->port = arr.at(1);
                pthread_mutex_lock(&data_lock);
                controler_set.insert(create);
                pthread_mutex_unlock(&data_lock);
                break;

                // EXIT
            case 5:
                break;

                // R
            case 6:
                arr = split(msg_data, ':');
                create = new HOST;
                create->ip = arr.at(0);
                create->port = arr.at(1);
                psize = ban_set.size();
                ban_set.insert(create);
                if (ban_set.size() > psize) {
                    pthread_mutex_lock(&ban_lock);
                    ban_queue.push("Ban:" + msg_data);
                    pthread_mutex_unlock(&ban_lock);
                }
                break;

                // R
            case 7:
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

int servent_infection(bool *console) {
    int random_num, delay;
    set<HOST *, HOSTPtrComp>::iterator host_i;
    set<HOST *, HOSTPtrComp> random_list;
    HOST *target = NULL;
    string send_data, recv_data;
    if (host_set.size() > GROWT && host_set.size() > GROWNUM) {
        printf("[INFO] Infecting...\n");
        for (int i = 0; i < GROWNUM && *console;) {
            random_num = (rand() * rand()) % host_set.size();
            host_i = host_set.begin();
            advance(host_i, random_num);
            target = *host_i;
            delay = delay_choose();
            if (delay == 0) {
                send_data = "Change:ServentBot";
                set<HOST *, HOSTPtrComp> my_list(servent_bot_set.begin(), servent_bot_set.end());
                set<HOST *, HOSTPtrComp>::iterator it_find = my_list.find(target);
                if (it_find != my_list.end() && it_find != my_list.end()) {
                    my_list.erase(my_list.find(target));
                }
                set<HOST *, HOSTPtrComp>::iterator bot_i;
                while (random_list.size() < PLSIZE) {
                    random_num = (rand() * rand()) % my_list.size();
                    bot_i = my_list.begin();
                    advance(bot_i, random_num);
                    random_list.insert(*bot_i);
                    my_list.erase(bot_i);
                }
                for (auto host : random_list) {
                    send_data += ":" + host->ip + ":" + host->port;
                }
                if (show_debug_msg) {
                    printf("[INFO] Sending PeerList...(%s)\n", send_data.c_str());
                }

            } else {
                send_data = "Change:ClientBot";

                set<HOST *, HOSTPtrComp>::iterator servent_i;
                while (random_list.size() < (servent_bot_set.size() > ServentPerClient ? ServentPerClient
                                                                                       : servent_bot_set.size())) {
                    random_num = (rand() * rand()) % servent_bot_set.size();
                    servent_i = servent_bot_set.begin();
                    advance(servent_i, random_num);
                    random_list.insert(*servent_i);
                }

                for (auto host : random_list) {
                    send_data += ":" + host->ip + ":" + host->port;
                }
                if (show_debug_msg) {
                    printf("[INFO] Sending ServentList...(%s)\n", send_data.c_str());
                }

            }
            _socket *client = new _socket((char *) ((target->ip).c_str()), (char *) ((target->port).c_str()), BUFSIZE);
            if (client->get_status()) {
                if (client->send_((char *) send_data.c_str()) == -1) {
                    printf("[Warning] HOST %s:%s Change Bot Failed.\n", (target->ip).c_str(), (target->port).c_str());
                } else {
                    client->shutdown_(_socket::BOTH);
                    client->close_();
                    pthread_mutex_lock(&data_lock);
                    bot_set.insert(target);
                    if (delay == 0)
                        servent_bot_set.insert(target);
                    host_set.erase(host_i);
                    pthread_mutex_unlock(&data_lock);
                    i++;
                }
            } else {
                printf("[Warning] HOST %s:%s Change Bot Failed.\n", (target->ip).c_str(), (target->port).c_str());
            }
            delete client;
            random_list.clear();
        }
    }
    return 1;
}

int servent_first_infection(bool *console) {
    int random_num;
    set<HOST *, HOSTPtrComp>::iterator host_i;
    vector<HOST *> target_list;
    vector<_socket *> client_list;
    HOST *target = NULL;
    string recv_data;
    if (host_set.size() > GROWT && host_set.size() > GROWNUM) {
        printf("[INFO] Servent Infecting...\n");
        for (int i = 0; i < GROWNUM && *console;) {
            random_num = (rand() * rand()) % host_set.size();
            host_i = host_set.begin();
            advance(host_i, random_num);
            target = *host_i;
            _socket *client = new _socket((char *) ((target->ip).c_str()), (char *) ((target->port).c_str()), BUFSIZE);
            if (client->get_status()) {
                if (client->send_((char *) ("Change:ServentBot")) == -1) {
                    printf("[Warning] HOST %s:%s Change Bot Failed.\n", (target->ip).c_str(), (target->port).c_str());
                    delete client;
                } else {
                    client_list.push_back(client);
                    target_list.push_back(target);
                    pthread_mutex_lock(&data_lock);
                    bot_set.insert(target);
                    servent_bot_set.insert(target);
                    host_set.erase(host_i);
                    pthread_mutex_unlock(&data_lock);
                    i++;
                }
            } else {
                printf("[Warning] HOST %s:%s Change Bot Failed.\n", (target->ip).c_str(), (target->port).c_str());
                delete client;
            }
        }

        vector<HOST *>::iterator target_i;
        vector<_socket *>::iterator client_i;
        vector<pthread_t> handle;
        pthread_t t;
        int result = 0;

        for (target_i = target_list.begin(), client_i = client_list.begin();
             target_i != target_list.end() && client_i != client_list.end() && *console;
             target_list.erase(target_i), client_list.erase(client_i)) {
            result = pthread_create(&t, NULL, handle_bot_spreading,
                                    (LPVOID) new pair<_socket *, HOST *>(*client_i, *target_i));
            if (!result) {
                handle.push_back(t);
                Sleep(5);
            }
        }
        for (auto i : handle) {
            pthread_join(i, NULL);
        }
    }
    return 1;
}