#include <iostream>
#include <ctime>
#include <queue>
#include "_socketserver.h"
#include "_thread.h"
#include <mutex>
#include <tchar.h>
#include<stdlib.h>
#include <map>

using namespace std;

typedef struct peerlist {
    char IP[20];
    char Port[6];
    char ddos_time[15];
    int rank = 7;
    bool trust_flag = false;
    bool send_flag = false;
} PeerList;
vector<PeerList> vec_Peerlist;          // peerlist 清單
vector<PeerList> vec_Serventlist;       // serventlist 清單
vector<PeerList> vec_Ddoslist;
vector<PeerList> vec_Sensorlist;
vector<PeerList> vec_Trustlist;
vector<PeerList> vec_Banlist;
vector<PeerList> vec_UnTrustlist;   // 不信任清單
PeerList checkTarget;

bool register_console();

bool register_controller();

void accept_thread();

void client_thread();

void servent_bot();

void client_bot();

void crawler();

void sensor();

void send_mes(char);

int handle_send_mes(char, char *);

char handle_recv_mes(char []);

void time_to_int(char [], int []);

void update_time(int []);

bool add_time(int, int, int []);

bool compare_with_time(int [], int []);

int check_exit();

int check_time();

int find_min_rank(vector<PeerList>);

int find_max_rank(vector<PeerList>);

int find_unTrust(vector<PeerList>);

int find_Trust(vector<PeerList>);

int find_Sended(vector<PeerList>);

void rebirth();

void check();

void check_bot_report();

void send_check_mes(char [], bool);

void handle_check_mes(char []);

int find_repeat_list(vector<PeerList>, PeerList);

void dice_hibernate_time();

void inspect_hibernate();

bool loss_rate_choose();

#define IDLE 5000
#define change_time 3600000 // 1小時
#define buffer_size 1024
#define MAXPLNum 15
#define FTH 25          // 晉升
#define HFTH 45         // 最高粉絲數 變Check Bot
#define MAXRank 12
#define HSTDRank 10      // 重生時聲譽值(可信任)標準 High Standard Rank
#define NormalRank 7
#define DistrustRank 4  // 不信任值
#define Host_flag  -1
#define Servent_flag 1
#define Crawler_flag 2
#define Sensor_flag  3
#define Client_flag  4
#define Offline_hr  4

map<string, int> mapCheck;
HANDLE QMutex;                      // Lock
mutex change_mtx;
queue<_socket *> ClientQ;            // Collect parameters for client_thread
vector<_thread *> ClientT;          // Collect _thread

_socketserver *s;

bool server_status = false;
bool hibernate_mode = false;
bool debug_mode = false;
bool sleep_status = false;
bool check_bot_status = false;
bool check_now = false; //  是否有正在檢查中的不信任殭屍
bool promotion_flag = false;
char my_ip[15] = "127.0.0.1", my_port[15] = "8000";
char c_ip[15] = "127.0.0.1", c_port[15] = "6666";
char *Concole_IP = c_ip, *Concole_Port = c_port;
char time_stamp[15] = "20180101000000";
int wakeup_time[6] = {};
char checkTimeExpire[15] = {};
int time_rate = 2000;
int register_flag = Host_flag; // Bot = 1, Crawler = 2, Sensor = 3
int delay = 0;
int fans_count = 0;
int hibernate_time = -1;
int rand_num, loss_rate;
int check_num = 0;

HANDLE peerlist_lock;
HANDLE trustlist_lock;
HANDLE untrustlist_lock;

int main(int argc, char *argv[]) {

    change_mtx.lock();

    strcpy(my_ip, argv[0]);
    strcpy(my_port, argv[1]);
    strcpy(c_ip, argv[2]);
    strcpy(c_port, argv[3]);
    rand_num = atoi(argv[5]);
    loss_rate = atoi(argv[6]);
    /*
    PeerList p1, p2, p3, p4;
    strcpy(p1.IP,"127.0.0.1");
    strcpy(p1.Port,"6666");
    strcpy(p2.IP,"127.0.0.1");
    strcpy(p2.Port,"1999");
    strcpy(p3.IP,"140.134.27.68");
    strcpy(p3.Port,"9999");
    p3.rank = 2;
    strcpy(p4.IP,"140.134.27.68");
    strcpy(p4.Port,"50000");
    p4.rank = 3;
    vec_Trustlist.push_back(p1);
    vec_Trustlist.push_back(p2);
    vec_UnTrustlist.push_back(p3);
    vec_UnTrustlist.push_back(p4);
    check_bot_status = true;
    */
    srand((unsigned) rand_num);
    dice_hibernate_time();  // 決定幾點休眠
    if (strcmp(argv[4], "Y") == 0) {
        if (debug_mode)
            printf("[OPEN Debug Mode]\n");
        debug_mode = true;
        _socket::Debug = true;
    } else
        debug_mode = false;

    if (debug_mode) {
        cout << "Host :" << my_ip << ":" << my_port << endl;
        cout << "Console :" << c_ip << ":" << c_port << endl << endl;
    }

    _socket::Debug = false;
    // 初始化 Mutex
    QMutex = CreateMutex(NULL, false, NULL);
    if (QMutex == NULL) {
        cout << "CreateMutex Error. ErrorCode=" << GetLastError() << endl;
        return 1;
    }
    // 使用Socket一定要先做
    WSADATA wsadata;
    _socket::wsastartup_(&wsadata);

    // 建立Server
    server_status = true;
    s = new _socketserver((char *) my_port, buffer_size);
    if (!s->get_status()) // 檢查port是否可用
        return -1;

    _thread accept_t((int (*)()) accept_thread);    // accept
    accept_t.start();
    _thread exit_t((int (*)()) check_exit);       // check_exit
    exit_t.start();
    _thread time_t((int (*)()) check_time);       // check_time
    time_t.start();


    // 與Console註冊
    bool console_flag = false;
    while ((!console_flag) && server_status) {
        console_flag = register_console();
        Sleep(1000);
    }
    change_mtx.lock(); // 鎖住，等待 Change 指令

    if (register_flag == Servent_flag) {             // Servent
        _thread servent_bot_t((int (*)()) servent_bot);
        servent_bot_t.start();
    } else if (register_flag == Crawler_flag) {        // Crawler
        _thread crawler_t((int (*)()) crawler);
        crawler_t.start();
    } else if (register_flag == Sensor_flag) {        // Sensor
        _thread sensor_t((int (*)()) sensor);
        sensor_t.start();
    } else if (register_flag == Client_flag) {         // Client
        _thread client_t((int (*)()) client_bot);
        client_t.start();
    }
    // Controller註冊
    if (register_flag == Servent_flag || register_flag == Client_flag) {
        bool controller_flag = false;
        int fail_reg_count = 0;
        while ((!controller_flag) && server_status) {
            controller_flag = register_controller();
            /* if (!controller_flag) {
                 fail_reg_count++;
             }*/
            Sleep(1000);
        }
    }

    int inst = 0;
    if (debug_mode) {

        while (server_status) {
            printf("\n----------------------------------\n");
            printf("-----Input '0'  :Bot Status ------\n");
            printf("-----Input '1'  :Peer List -------\n");
            printf("-----Input '2'  :Trust List ------\n");
            printf("-----Input '3'  :UnTrust List-----\n");
            printf("-----Input '4'  :Show Time--------\n");
            printf("-----Input '5'  :Show DDOS--------\n");
            printf("-----Input '6'  :Ban List---------\n");
            printf("-----Input '7'  :Sleep Status-----\n");
            printf("-----Input '8'  :Loss Rate--------\n");
            printf("-----Input '9'  :Clear Screen-----\n");
            printf("-----Input '-1' :Exit-------------\n");
            printf("----------------------------------\n");
            scanf("%d", &inst);
            if (inst < -1 || inst > 10) {
                printf("[Input Error]\n");
                continue;
            }
            if (inst == 0) {
                if (sleep_status)
                    printf("-------Sleep Bot--------\n");
                else if (check_bot_status) {
                    printf("-------Check Bot--------\n");
                    if (check_now) {
                        printf("-------Check No.%d[%s:%s]--------\n", check_num,checkTarget.IP, checkTarget.Port);
                        printf("[Expire Time] %s\n",checkTimeExpire);
                        printf("[Map Size] = %d\n", mapCheck.size());
                    } else
                        printf("-------Not  Check--------\n");
                } else if (register_flag == Client_flag)
                    printf("-------Client Bot--------\n");
                else if (register_flag == Servent_flag) {
                    printf("-------Servent Bot--------\n");
                    printf("[Fans] :%d\n", fans_count);
                } else if (register_flag == Crawler_flag)
                    printf("-------Crawler--------\n");
                else if (register_flag == Sensor_flag)
                    printf("-------Sensor--------\n");
                printf("[IP]: %s\n", my_ip);
                printf("[Port]: %s\n", my_port);
            } else if (inst == 1) {
                if (register_flag == Client_flag) {
                    printf("\n-----------ServentList Size = %d -----------\n", vec_Serventlist.size());
                    for (int i = 0; i < vec_Serventlist.size(); i++)
                        printf("[%s:%s]\n", vec_Serventlist[i].IP, vec_Serventlist[i].Port);
                } else {
                    printf("\n-----------PeerList Size = %d -----------\n", vec_Peerlist.size());
                    for (int i = 0; i < vec_Peerlist.size(); i++)
                        printf("[%s:%s = %d]\n", vec_Peerlist[i].IP, vec_Peerlist[i].Port, vec_Peerlist[i].rank);
                }
            } else if (inst == 2) {
                printf("\n-----------TrustList Size = %d -----------\n", vec_Trustlist.size());
                if (register_flag == Servent_flag) {
                    for (int i = 0; i < vec_Trustlist.size(); i++)
                        printf("[%s:%s]\n", vec_Trustlist[i].IP, vec_Trustlist[i].Port);
                }
            } else if (inst == 3) {
                printf("\n-----------UnTrustList Size = %d -----------\n", vec_UnTrustlist.size());
                if (register_flag == Servent_flag) {
                    for (int i = 0; i < vec_UnTrustlist.size(); i++)
                        printf("[%s:%s] = %d\n", vec_UnTrustlist[i].IP, vec_UnTrustlist[i].Port,
                               vec_UnTrustlist[i].rank);
                }
            } else if (inst == 4)
                printf("[Time] :%s:%d\n", time_stamp, time_rate);
            else if (inst == 5) {
                int len = vec_Ddoslist.size();
                for (int i = 0; i < len; i++)
                    printf("[DDOS Queue No.%d] %s:%s  [Time] %s\n", i + 1, vec_Ddoslist[i].IP, vec_Ddoslist[i].Port,
                           vec_Ddoslist[i].ddos_time);
            } else if (inst == 6) {
                printf("\n-----------Ban List Size = %d -----------\n", vec_Banlist.size());
                for (int i = 0; i < vec_Banlist.size(); i++)
                    printf("[%s:%s]\n", vec_Banlist[i].IP, vec_Banlist[i].Port);
            } else if (inst == 7) {
                printf("[Goot Night at %d]\n", hibernate_time);
                if (hibernate_mode)
                    printf("[Sleep Now]\n");
                else
                    printf("[Activive]\n");
            } else if (inst == 8) {
                printf("[Loss Rate] = %d %% \n", loss_rate * 10);
            } else if (inst == 9) {
                system("CLS");
            } else if (inst == -1)
                break;
        }
    }
    while (server_status) {
        Sleep(5000);
    }
    server_status = false;
    //accept_t.join();
    // 清理垃圾
    delete (s);
    _socket::wsacleanup_();
    CloseHandle(QMutex);
    return 0;
}

bool register_console() {

    char *send_data = new char[buffer_size];
    char *rec_data = new char[buffer_size];
    const char *p = ":";
    char *buf = {};
    bool flag = false;
    // Console
    _socket c(Concole_IP, Concole_Port, buffer_size); // socket Console
    if (c.get_status()) {
        memset(rec_data, '\0', buffer_size);
        sprintf(send_data, "HOST%s:%s", my_ip, my_port);
        if (c.send_(send_data) > 0) {
            if (debug_mode)
                cout << "Send to Console :" << send_data << endl;
            rec_data = c.recv_();   // 等待Console回覆
            if (rec_data) {
                if (debug_mode)
                    cout << "[Receive] Console :" << rec_data << endl << endl;
                if (strcmp(rec_data, "OK") == 0) {
                    register_flag = Host_flag;
                    flag = true;
                } else {
                    handle_recv_mes(rec_data);
                    if (register_flag != Host_flag)
                        flag = true;
                }
            } else {
                if (debug_mode)
                    printf("[INFO] Register Console Failed\n");
                register_flag = Host_flag;
            }

        } else {
            if (debug_mode)
                printf("[INFO] Receive Console Failed\n");
        }
        //c.shutdown_(_socket::BOTH);
        c.close_();
    } else {
        c.close_();
        if (debug_mode)
            printf("[INFO] Connect Console Failed\n");
        register_flag = Host_flag;
    }

    delete[] send_data;
    delete[] rec_data;
    return flag;
}

bool register_controller() {
    char *send_data = new char[buffer_size];
    char *rec_data = new char[buffer_size];
    bool flag = false;
    // Controller
    if ((register_flag == Crawler_flag) || (register_flag == Sensor_flag)) {
        return true;
    }
    _socket ctrl((char *) "127.0.0.1", (char *) "1999", buffer_size); // socket Controller
    if (ctrl.get_status()) {
        if (register_flag == Servent_flag)
            sprintf(send_data, "SS%s", my_port);
        else if (register_flag == Client_flag)
            sprintf(send_data, "SC%s", my_port);
        if (ctrl.send_(send_data) > 0) {
            if (debug_mode)
                cout << "Send to Controller :" << send_data << endl;
            if (ctrl.check_recv_(100000)) {   // Check recv
                rec_data = ctrl.recv_();
                if (rec_data) {
                    if (strcmp(rec_data, "OK") == 0)
                        flag = true;
                    if (debug_mode)
                        printf("[Receive] Controller :%s\n", rec_data);
                } else {
                    if (debug_mode)
                        printf("[INFO] Receive Controller Failed\n");
                }
            } else {
                if (debug_mode)
                    printf("[INFO] Receive Controller Time Out\n");
            }
        }
        //ctrl.shutdown_(_socket::BOTH);
        ctrl.close_();
    } else {
        ctrl.close_();
        if (debug_mode)
            printf("[INFO] Connect Controller Failed\n");
    }

    delete[] send_data;
    delete[] rec_data;

    return flag;
}

void accept_thread() {
    bool flag;
    //cout << "Accepting..." << endl;
    _thread *t = NULL;
    while (server_status) {
        flag = s->check_connect_(1000);
        if (flag) {
            //cout << "Somebody connected." << endl;
            WaitForSingleObject(QMutex, INFINITE);
            ClientQ.push(s->accept_());
            ReleaseMutex(QMutex);
            client_thread();

        }
    }
    if (debug_mode)
        cout << "Stop accepting..." << endl;
}

void client_thread() {
    srand((unsigned) time(NULL));
    WaitForSingleObject(QMutex, INFINITE);
    _socket *client = ClientQ.front();
    ClientQ.pop();
    ReleaseMutex(QMutex);
    char *rec_data = new char[buffer_size];
    char rec_flag;
    char send_data[buffer_size] = {};
    memset(send_data, '\0', buffer_size);
    rec_data = client->recv_();

    if ((!sleep_status) && (!hibernate_mode)) {  // 不是SleepBot 也不在休眠
        if (rec_data) {
            rec_flag = handle_recv_mes(rec_data);
            /*if (register_flag == Servent_flag || register_flag == Crawler_flag) {
                if (loss_rate_choose()) {    // 封包遺失　不回訊息
                    client->shutdown_(_socket::BOTH);
                    client->close_();
                    delete client;
                    delete[] rec_data;
                    return;
                }
            }*/
            if ((rec_flag != 'T') && (delay == 1)) {
                int temp = rand() % 5000;
                Sleep(temp);
            }
            if ((rec_flag == 'E') && (register_flag != Sensor_flag)) {
                handle_send_mes(rec_flag, send_data);
                client->send_(send_data);
            } else if (rec_flag == 'D') {
                send_mes('D');
            } else if (rec_flag == 'L') {       // Release -- Servent_Bot 回傳給 Client_Bot DDOS訊息
                handle_send_mes('D', send_data);
                client->send_(send_data);
            } else if (rec_flag == 'F') {       // Servent 尚無發布之訊息
                client->send_((char *) "Fault");
            } else if ((rec_flag == 'B') || (rec_flag == 'C')) {    // 註冊 要求 List
                client->send_((char *) "Request:Peerlist");
                memset(rec_data, '\0', buffer_size);
                rec_data = client->recv_();
                if (rec_data) {
                    handle_recv_mes(rec_data);
                    if (rec_flag == 'B') {         // Servent
                        client->send_((char *) "EXIT");
                        register_flag = Servent_flag;
                        change_mtx.unlock();
                    } else if (rec_flag == 'C') {   // Crawler要求 Sensorlist
                        client->send_((char *) "Request:Sensorlist");
                        memset(rec_data, '\0', buffer_size);
                        rec_data = client->recv_();
                        if (rec_data) {
                            client->send_((char *) "EXIT");
                            register_flag = Crawler_flag;
                            handle_recv_mes(rec_data);
                            change_mtx.unlock();
                        }
                    }
                } else {
                    if (debug_mode)
                        printf("[INFO] Request Peerlist failed\n");
                    register_flag = Host_flag;
                }
            } else if (rec_flag == 'S') {   // Change to Sesnor
                register_flag = Sensor_flag;
                change_mtx.unlock();
            } else if (rec_flag == 'Z') {   // 要求 Serventlist
                client->send_((char *) "Request:Serventlist");
                memset(rec_data, '\0', buffer_size);
                rec_data = client->recv_();
                if (rec_data) {
                    handle_recv_mes(rec_data);
                    client->send_((char *) "EXIT");
                    register_flag = Client_flag;
                    change_mtx.unlock();
                }
            }
        }
    }
    //client->shutdown_(_socket::BOTH);
    client->close_();
    delete client;
    delete[] rec_data;
}

void servent_bot() {
    srand((unsigned) time(NULL));
    if (debug_mode)
        printf("-------Servent Bot--------\n");
    Sleep(3000);
    int count_time = 0;
    int quest_count = 0;
    while (server_status) {   // Server 有無啟動
        if (count_time >= change_time) {
            count_time = 0;
            if ((!sleep_status) && (!hibernate_mode)) {      // 檢查是不是正在 Sleep or 休眠
                if (vec_Peerlist.size() > 0) {
                    if (vec_Peerlist.size() <= 15 && (quest_count % 3 == 0))
                        send_mes('E');      // PeerList滿之前 3小時問一次
                    else
                        send_mes('Q');      // 一小時問一次
                    quest_count++;
                }
                if (vec_Ddoslist.size() > 0) {
                    if (strcmp(time_stamp, vec_Ddoslist[0].ddos_time) >= 0)  // Attack
                        send_mes('A');
                }
                if (check_bot_status) {    // Check Bot
                    if (check_now)
                        check_bot_report();     // 檢查時間是否到期
                    else
                        check();        // 發起檢查
                } else {
                    if (promotion_flag) {
                        if ((fans_count > HFTH)) {  // 累積達到一定數量轉變成Check Bot
                            send_mes('C');      // 告知 Console 我變成 CheckBot ， 傳送成功才變成 Check Bot
                        }
                    } else {
                        if (fans_count > FTH) {   // Promotion
                            send_mes('P');
                            promotion_flag = true;
                        }
                    }
                }
            }
        }
        Sleep(500);
        count_time += 500 * time_rate;
    }
};

void client_bot() {
    srand((unsigned) time(NULL));
    if (debug_mode)
        printf("-------Client Bot--------\n");
    Sleep(3000);
    int count_time = 0;
    while (server_status) {  // Server 有無啟動
        if (!hibernate_mode) {        // 不在休眠狀態
            if (count_time >= change_time) { // 1小時
                count_time = 0;
                if (vec_Serventlist.size() > 0)
                    send_mes('Q'); // Quest
                if (vec_Ddoslist.size() > 0) {
                    if (strcmp(time_stamp, vec_Ddoslist[0].ddos_time) >= 0)
                        send_mes('A');    // Attack
                }
            }
        }

        Sleep(500);
        count_time += 500 * time_rate;
    }
}

void crawler() {
    srand((unsigned) time(NULL));
    if (debug_mode)
        printf("-------Crawler--------\n");
    Sleep(3000);
    int count_time = 0;
    while (server_status) {
        if (count_time >= change_time) {
            count_time = 0;
            if ((vec_Peerlist.size() > 0) && (vec_Sensorlist.size() > 0))
                send_mes('E');

        }
        Sleep(500);
        count_time += 500 * time_rate;
    }
}

void sensor() {
    if (debug_mode)
        printf("-------Sensor--------\n");
    while (server_status) {
        if (vec_Peerlist.size() > 0) {
            send_mes('R');
        }
        Sleep(1000);
    }
}

void send_mes(char flag) {
    int send_num = -1;
    char *rec_data = new char[buffer_size];
    char *send_data = new char[buffer_size];
    memset(send_data, '\0', buffer_size);
    WaitForSingleObject(peerlist_lock, INFINITE);        // Lock
    WaitForSingleObject(trustlist_lock, INFINITE);       // Lock

    send_num = handle_send_mes(flag, send_data);
    switch (flag) {
        case 'E' : {
            // Exchange Peerlist
            _socket c((char *) vec_Peerlist[send_num].IP, (char *) vec_Peerlist[send_num].Port, buffer_size);
            if (c.get_status()) {
                c.send_(send_data);

                if (debug_mode)
                    cout << "[Send]--->" << vec_Peerlist[send_num].IP << ":" << vec_Peerlist[send_num].Port << ":["
                         << send_data << "]" << endl;
                if (c.check_recv_(IDLE)) {   // Check recv
                    rec_data = c.recv_();
                    if (rec_data) {
                        handle_recv_mes(rec_data);
                        if (register_flag == Servent_flag) {
                            if (!vec_Peerlist[send_num].trust_flag) { // 聲譽值還沒到達頂點
                                vec_Peerlist[send_num].rank++;  // 聲譽值增加
                                if (debug_mode)
                                    printf("[INFO] [%s:%s] [Rank + 1] \n", vec_Peerlist[send_num].IP,
                                           vec_Peerlist[send_num].Port);
                            }
                        } else if (register_flag == Crawler_flag) {     // 爬蟲傳過訊息
                            vec_Peerlist[send_num].send_flag = true;
                        }
                    } else {
                        if (register_flag == Servent_flag) {
                            if ((vec_Peerlist[send_num].rank > 0) && (!vec_Peerlist[send_num].trust_flag)) {
                                vec_Peerlist[send_num].rank--;  // 未收到回覆 聲譽值減少
                                if (debug_mode)
                                    printf("[INFO] [%s:%s] [Rank - 1] \n", vec_Peerlist[send_num].IP,
                                           vec_Peerlist[send_num].Port);
                            }
                        }
                    }
                } else {
                    if (register_flag == Servent_flag) {
                        if ((vec_Peerlist[send_num].rank > 0) && (!vec_Peerlist[send_num].trust_flag)) {
                            vec_Peerlist[send_num].rank--;  // 未收到回覆 聲譽值減少
                            if (debug_mode)
                                printf("[INFO] [%s:%s] [Rank - 1] \n", vec_Peerlist[send_num].IP,
                                       vec_Peerlist[send_num].Port);
                        }
                    }
                    if (debug_mode)
                        printf("Recv Time out\n");
                }
                //c.shutdown_(_socket::BOTH);
                c.close_();
            } else {
                if (debug_mode)
                    printf("[INFO] Connect Error [%s:%s]\n", vec_Peerlist[send_num].IP, vec_Peerlist[send_num].Port);
                if (register_flag == Servent_flag) {
                    if ((vec_Peerlist[send_num].rank > 0) && (!vec_Peerlist[send_num].trust_flag)) {
                        vec_Peerlist[send_num].rank--;  // 連接失敗 聲譽值減少
                        if (debug_mode)
                            printf("[INFO] [%s:%s] [Rank - 1] \n", vec_Peerlist[send_num].IP,
                                   vec_Peerlist[send_num].Port);
                    }
                    c.close_();
                }
            }
            if (register_flag == Servent_flag) {
                // 聲譽值到達頂點 且 尚未傳過 Fans_Add
                if ((vec_Peerlist[send_num].rank >= MAXRank) && (!vec_Peerlist[send_num].trust_flag)) {
                    peerlist p1 = vec_Peerlist[send_num];
                    if (find_repeat_list(vec_Trustlist, p1) < 0) {  // TrustList裡 無此殭屍才加入
                        _socket t(p1.IP, p1.Port, buffer_size);
                        if (t.get_status()) {
                            if (t.send_((char *) "Fans_Add") > 0) {       // Send 失敗的話這回合先不加入
                                vec_Peerlist[send_num].trust_flag = true;
                            }
                            //t.shutdown_(_socket::BOTH);
                            t.close_();
                        } else {
                            t.close_();
                        }
                    } else {
                        vec_Peerlist.erase(vec_Peerlist.begin() + send_num); // 若TrustList裡有此殭屍則直接從PeerList刪除
                    }
                }
            }
        }
            break;
        case 'D' : {
            // 傳播 DDOS 訊息
            for (int i = 0; i < vec_Peerlist.size(); i++) {
                _socket d(vec_Peerlist[i].IP, vec_Peerlist[i].Port, buffer_size); // socket
                if (d.get_status()) {
                    d.send_(send_data);
                    //d.shutdown_(_socket::BOTH);
                    d.close_();
                } else {
                    d.close_();
                }
                Sleep(500);
            }
        }
            break;
        case 'R' : {
            WaitForSingleObject(peerlist_lock, INFINITE);
            // Sensor 回報抓到的 IP
            _socket r((char *) "127.0.0.1", (char *) "1999", buffer_size);
            if (r.get_status()) {
                r.send_(send_data);
                vec_Peerlist.erase(vec_Peerlist.begin());
                if (debug_mode)
                    cout << "[Report] " << "Controller " << "--->:[" << send_data << "]" << endl;
                //r.shutdown_(_socket::BOTH);
                r.close_();
            } else {
                r.close_();
            }
            ReleaseMutex(peerlist_lock);
        }
            break;
        case 'A' : {
            // DDOS 攻擊
            _socket a(vec_Ddoslist[0].IP, vec_Ddoslist[0].Port, buffer_size); // socket
            if (a.get_status()) {
                a.send_((char *) "DDOS");
                if (debug_mode) {
                    printf("[DDOS]--->%s:%s  [Time] %s\n", vec_Ddoslist[0].IP, vec_Ddoslist[0].Port,
                           vec_Ddoslist[0].ddos_time);
                }
                vec_Ddoslist.erase(vec_Ddoslist.begin());
                //a.shutdown_(_socket::BOTH);
                a.close_();
            } else {
                a.close_();
                if (debug_mode)
                    printf("[DDOS] Can't Connect\n");
            }
        }
            break;
        case 'Q' : {
            // Client 詢問 Servent 有無事件
            if (register_flag == Servent_flag) {
                WaitForSingleObject(peerlist_lock, INFINITE);       // Lock

                _socket c((char *) vec_Peerlist[send_num].IP, (char *) vec_Peerlist[send_num].Port, buffer_size);
                if (c.get_status()) {
                    c.send_(send_data);
                    if (debug_mode)
                        cout << "[Send]--->" << vec_Peerlist[send_num].IP << ":" << vec_Peerlist[send_num].Port
                             << ":["
                             << send_data << "]" << endl;
                    if (c.check_recv_(IDLE)) {   // Check recv
                        rec_data = c.recv_();
                        if (rec_data) {
                            handle_recv_mes(rec_data);
                            if (!vec_Peerlist[send_num].trust_flag) { // 聲譽值還沒到達頂點
                                vec_Peerlist[send_num].rank++;  // 聲譽值增加
                                if (debug_mode)
                                    printf("[INFO] [%s:%s] [Rank + 1] \n", vec_Peerlist[send_num].IP,
                                           vec_Peerlist[send_num].Port);
                            }
                        } else {
                            if ((vec_Peerlist[send_num].rank > 0) && (!vec_Peerlist[send_num].trust_flag)) {
                                vec_Peerlist[send_num].rank--;  // 未收到回覆 聲譽值減少
                                if (debug_mode)
                                    printf("[INFO] [%s:%s] [Rank - 1] \n", vec_Peerlist[send_num].IP,
                                           vec_Peerlist[send_num].Port);
                            }
                        }
                    } else {
                        if ((vec_Peerlist[send_num].rank > 0) && (!vec_Peerlist[send_num].trust_flag)) {
                            vec_Peerlist[send_num].rank--;  // 未收到回覆 聲譽值減少
                            if (debug_mode)
                                printf("[INFO] [%s:%s] [Rank - 1] \n", vec_Peerlist[send_num].IP,
                                       vec_Peerlist[send_num].Port);
                        }
                        if (debug_mode)
                            printf("Recv Time out\n");
                    }
                    //c.shutdown_(_socket::BOTH);
                    c.close_();
                } else {
                    if (debug_mode)
                        printf("[INFO] Connect Error [%s:%s]\n", vec_Peerlist[send_num].IP,
                               vec_Peerlist[send_num].Port);

                    if ((vec_Peerlist[send_num].rank > 0) && (!vec_Peerlist[send_num].trust_flag)) {
                        vec_Peerlist[send_num].rank--;  // 連接失敗 聲譽值減少
                        if (debug_mode)
                            printf("[INFO] [%s:%s] [Rank - 1] \n", vec_Peerlist[send_num].IP,
                                   vec_Peerlist[send_num].Port);
                    }
                    c.close_();
                }
                // 聲譽值頂點加入Trust List
                // [注意有無互斥問題]

                // 聲譽值到達頂點 且 尚未傳過 Fans_Add
                if ((vec_Peerlist[send_num].rank >= MAXRank) && (!vec_Peerlist[send_num].trust_flag)) {
                    peerlist p1 = vec_Peerlist[send_num];
                    if (find_repeat_list(vec_Trustlist, p1) < 0) {  // TrustList裡 無此殭屍才加入
                        _socket t(p1.IP, p1.Port, buffer_size);
                        if (t.get_status()) {
                            if (t.send_((char *) "Fans_Add") > 0) {       // Send 失敗的話這回合先不加入
                                vec_Peerlist[send_num].trust_flag = true;

                            }
                            //t.shutdown_(_socket::BOTH);
                            t.close_();
                        } else {
                            t.close_();
                        }
                    } else {
                        vec_Peerlist.erase(vec_Peerlist.begin() + send_num); // 若TrustList裡有此殭屍則直接從PeerList刪除
                    }
                }
            } else if (register_flag == Client_flag) {
                _socket q((char *) vec_Serventlist[send_num].IP, (char *) vec_Serventlist[send_num].Port, buffer_size);
                if (q.get_status()) {
                    q.send_(send_data);
                    if (debug_mode)
                        printf("[Send]--->%s:%s:[%s]\n", vec_Serventlist[send_num].IP, vec_Serventlist[send_num].Port,
                               send_data);
                    if (q.check_recv_(IDLE)) {   // Check recv
                        rec_data = q.recv_();
                        if (rec_data) {
                            if (strcmp(rec_data, "Fault") == 0) {
                                if (debug_mode)
                                    printf("[Receive] %s:%s--->[%s]", vec_Serventlist[send_num].IP,
                                           vec_Serventlist[send_num].Port, rec_data);
                            } else
                                handle_recv_mes(rec_data);
                        }
                    } else {
                        if (debug_mode)
                            printf("Recv Time out\n");
                    }
                    // q.shutdown_(_socket::BOTH);
                    q.close_();
                } else {
                    if (debug_mode)
                        printf("[INFO] Connect Error\n");
                    q.close_();
                }
            }
        }
            break;
        case 'P' : {
            // 知名度夠高後要晉升
            _socket p(Concole_IP, Concole_Port, buffer_size); // socket Console
            if (p.get_status()) {
                p.send_(send_data);
                if (debug_mode)
                    printf("[Send]--->Console:[%s]\n", send_data);
                rec_data = p.recv_();
                if (rec_data) {
                    handle_recv_mes(rec_data);
                }
                //p.shutdown_(_socket::BOTH);
                p.close_();
            } else {
                p.close_();
            }
        }
            break;
        case 'C' : {
            // 告知 Console 我已轉變為Check Bot
            _socket con(Concole_IP, Concole_Port, buffer_size); // socket Console
            if (con.get_status()) {
                con.send_(send_data);
                if (debug_mode)
                    printf("[Send]--->Console:[%s]\n", send_data);
                check_bot_status = true;        // 轉變 Check Bot

                printf("----Change Check Bot [%s:%s] --------\n", my_ip, my_port);
                //con.shutdown_(_socket::BOTH);
                con.close_();
            } else {
                con.close_();
            }
        }
            break;
        default:
            break;
    }
    ReleaseMutex(peerlist_lock);    // Release
    ReleaseMutex(trustlist_lock);   // Release
    delete[] rec_data;
    delete[] send_data;
}

int handle_send_mes(char flag, char *send_buf) {
    //  格式[ 發送時間:到期時間:指令:目標IP:目標Port:發送端IP:發送端Port ]
    memset(send_buf, '\0', buffer_size);
    int send_num = 0, tar_num = 0, time_t[6] = {};
    char time_expire[15] = {};

    time_to_int(time_stamp, time_t); //   [0]年 [1]月 [2]日 [3]時 [4]分 [5]秒
    add_time(2, 3, time_t);       // + 三天

    sprintf(time_expire, "%04d%02d%02d%02d%02d%02d", time_t[0], time_t[1], time_t[2], time_t[3], time_t[4], time_t[5]);


    switch (flag) {

        case 'E':
            send_num = rand() % vec_Peerlist.size();
            if (register_flag == Servent_flag) { // Servent Bot
                if (vec_Trustlist.size() > 0) {   // 優先交換 聲譽值高的給別人
                    tar_num = rand() % vec_Trustlist.size();
                    // TrustList
                    sprintf(send_buf, "%s:%s:%s:%s:%s:%s:%s", time_stamp, time_expire,
                            "Exchange_peerlist",
                            vec_Trustlist[tar_num].IP, vec_Trustlist[tar_num].Port, my_ip, my_port);
                } else {
                    do {
                        tar_num = rand() % vec_Peerlist.size();
                    } while ((send_num == tar_num) || tar_num < 0);
                    // PeerList
                    sprintf(send_buf, "%s:%s:%s:%s:%s:%s:%s", time_stamp, time_expire, "Exchange_peerlist",
                            vec_Peerlist[tar_num].IP, vec_Peerlist[tar_num].Port, my_ip, my_port);
                }
            } else if (register_flag == Crawler_flag) {     // Crawler
                int sensor_num = rand() % vec_Sensorlist.size();
                sprintf(send_buf, "%s:%s:%s:%s:%s:%s:%s", time_stamp, time_expire, "Exchange_peerlist",
                        vec_Sensorlist[sensor_num].IP, vec_Sensorlist[sensor_num].Port, my_ip,
                        my_port);    // SensorLIst
            }
            break;
        case 'D':
            tar_num = vec_Ddoslist.size() - 1;
            sprintf(send_buf, "%s:%s:%s:%s:%s:%s:%s", time_stamp, vec_Ddoslist[tar_num].ddos_time, "DDOS",
                    vec_Ddoslist[tar_num].IP, vec_Ddoslist[tar_num].Port, my_ip, my_port);
            break;
        case 'R':            // Report
            sprintf(send_buf, "R%s:%s", vec_Peerlist[0].IP, vec_Peerlist[0].Port);
            break;
        case 'Q':            // Client or Servent 發起 Quest
            if (register_flag == Client_flag)
                send_num = rand() % vec_Serventlist.size();
            else
                send_num = rand() % vec_Peerlist.size();
            sprintf(send_buf, "%s:%s:%s", time_stamp, time_expire, "Quest");
            break;
        case 'P':            // 晉升 Promotion
            sprintf(send_buf, "%s%s:%s", "Promotion", my_ip, my_port);
            break;
        case 'C':            // Change to Check_Bot
            sprintf(send_buf, "%s%s:%s", "ChangeCheckBot", my_ip, my_port);
            break;
        default:
            return -1;
    }

    return send_num;
}

char handle_recv_mes(char data[]) {

    bool ExPL_flag = false, DDOS_flag = false, update_time_flag = false;
    int count = 0;
    char *buf = {}, tar_ip[16] = {}, tar_port[5] = {}, time_expire[15] = {}, client_ip[16] = {}, client_port[5] = {};
    char rtn_flag = 'N';
    char output[buffer_size];
    const char *p = ":";
    PeerList p1;
    memset(p1.IP, '\0', 20);
    memset(p1.Port, '\0', 6);
    strcpy(output, data);
    buf = strtok(data, p);
    memset(tar_ip, '\0', 20);
    memset(tar_port, '\0', 5);
    if (sleep_status) {       // Sleep Bot
        return 'N';
    }
    while (buf) {
        switch (count) {
            case 0 :
                if (strcmp(buf, "SWAP") == 0) {       // 關閉程式
                    if (debug_mode)
                        printf("SWAP\n");
                    send_mes('E');
                    return 'N';
                } else if (strcmp(buf, "Check") == 0) {        // Check訊息
                    if (register_flag == Servent_flag) {
                        if (debug_mode)
                            printf("[Receive] Check [%s]\n", output);
                        handle_check_mes(output);
                    }
                    return 'N';
                } else if (strcmp(buf, "Rebirth") == 0) {      // 重生
                    if (register_flag == Servent_flag) {            // 只有 Servent 重生
                        rebirth();
                    }
                } else if (strcmp(buf, "Fans_Add") == 0) {     // Fans + 1
                    fans_count++;
                    return 'N';
                } else if (strcmp(buf, "Accumulate") == 0) {// 繼續累積 Fans
                    if (debug_mode)
                        printf("[Accumulate]\n");
                    return 'N';
                } else if (strcmp(buf, "T") == 0)
                    update_time_flag = true;
                else if (strcmp(buf, "Ban") == 0) {          //   Ban 封鎖
                    buf = strtok(NULL, p);
                    strcpy(p1.IP, buf);
                    buf = strtok(NULL, p);
                    strcpy(p1.Port, buf);
                    if (find_repeat_list(vec_Banlist, p1) < 0) {    // 加入封鎖清單
                        vec_Banlist.push_back(p1);
                        if (debug_mode)
                            printf("[INFO] Ban %s:%s\n", p1.IP, p1.Port);
                    }
                    int num = -1;
                    num = find_repeat_list(vec_Peerlist, vec_Banlist[vec_Banlist.size() - 1]);
                    if (num >= 0) {     // 從PeerList中刪除
                        vec_Peerlist.erase(vec_Peerlist.begin() + num);
                        if (debug_mode)
                            printf("[INFO] Delete %s:%s\n", p1.IP, p1.Port);
                    }
                    num = find_repeat_list(vec_Trustlist, vec_Banlist[vec_Banlist.size() - 1]);
                    if (num >= 0) {     // 從TrustList中刪除
                        vec_Trustlist.erase(vec_Trustlist.begin() + num);
                        if (debug_mode)
                            printf("[INFO] Delete %s:%s\n", p1.IP, p1.Port);
                    }
                } else if (strcmp(buf, "Change") == 0) {
                    buf = strtok(NULL, p);
                    if (strcmp(buf, "ServentBot") == 0) {
                        count = 0;
                        buf = strtok(NULL, p);
                        while (buf) {
                            if (count % 2 == 0) {
                                strcpy(p1.IP, buf);
                            } else if (count % 2 == 1) {
                                strcpy(p1.Port, buf);
                                vec_Peerlist.push_back(p1);
                            }
                            count++;
                            buf = strtok(NULL, p);
                        }
                        if (count == 0)
                            rtn_flag = 'B';
                        else {
                            register_flag = Servent_flag;
                            change_mtx.unlock();
                            if (debug_mode)
                                cout << "[Receive] Console :" << output << endl << endl;
                            return 'N';
                        }
                    } else if (strcmp(buf, "Crawler") == 0)
                        rtn_flag = 'C';
                    else if (strcmp(buf, "Sensor") == 0)
                        rtn_flag = 'S';
                    else if (strcmp(buf, "SleepBot") == 0) {
                        sleep_status = true;
                        rtn_flag = 'N';
                    } else if (strcmp(buf, "CheckBot") == 0) {
                        check_bot_status = true;
                        rtn_flag = 'N';
                    } else if (strcmp(buf, "ClientBot") == 0) {
                        count = 0;
                        buf = strtok(NULL, p);
                        while (buf) {
                            if (count % 2 == 0) {
                                strcpy(p1.IP, buf);
                            } else if (count % 2 == 1) {
                                strcpy(p1.Port, buf);
                                vec_Serventlist.push_back(p1);
                            }
                            count++;
                            buf = strtok(NULL, p);
                        }
                        if (count == 0)
                            rtn_flag = 'Z';
                        else {
                            register_flag = Client_flag;
                            change_mtx.unlock();
                            if (debug_mode)
                                cout << "[Receive] Console :" << output << endl << endl;
                            return 'N';
                        }
                    }
                } else if (strcmp(buf, "Peerlist") == 0) {
                    buf = strtok(NULL, p);
                    count = 0;
                    while (buf) {
                        count = count % 2;
                        if (count == 0) {
                            memset(p1.IP, '\0', 20);
                            strcpy(p1.IP, buf);
                        } else {
                            memset(p1.Port, '\0', 5);
                            strcpy(p1.Port, buf);
                            if (find_repeat_list(vec_Peerlist, p1) < 0) // 檢查 對方傳的IP,Port有無重複
                                vec_Peerlist.push_back(p1);
                        }
                        count++;
                        buf = strtok(NULL, p);
                    }
                } else if (strcmp(buf, "Serventlist") == 0) {
                    buf = strtok(NULL, p);
                    count = 0;
                    while (buf) {
                        count = count % 2;
                        if (count == 0) {
                            memset(p1.IP, '\0', 20);
                            strcpy(p1.IP, buf);
                        } else {
                            memset(p1.Port, '\0', 5);
                            strcpy(p1.Port, buf);
                            vec_Serventlist.push_back(p1);
                        }
                        count++;
                        buf = strtok(NULL, p);
                    }
                } else if ((strcmp(buf, "Sensorlist") == 0)) {
                    buf = strtok(NULL, p);
                    count = 0;;
                    while (buf) {
                        count = count % 2;
                        if (count == 0)
                            strcpy(p1.IP, buf);
                        else {
                            strcpy(p1.Port, buf);
                            vec_Sensorlist.push_back(p1);
                        }
                        count++;
                        buf = strtok(NULL, p);
                    }
                }
                break;
            case 1 :
                if (update_time_flag) {
                    strcpy(time_stamp, buf); // 更新時間
                    // if(debug_mode)
                    // printf("[Update Time] :%s\n",time_stamp);
                    return 'T';
                } else {
                    strcpy(time_expire, buf);
                    int now_int[6], expire_int[6];
                    time_to_int(time_expire, expire_int);
                    time_to_int(time_stamp, now_int);
                    if (!compare_with_time(now_int, expire_int))
                        return 'N'; // 時間已過期
                }
                break;
            case 2 :
                if (strcmp(buf, "Exchange_peerlist") == 0) {
                    if (register_flag == Host_flag || register_flag == Client_flag)
                        return 'N';
                    ExPL_flag = true;
                } else if (strcmp(buf, "DDOS") == 0) {
                    if (register_flag == Host_flag)
                        return 'N';
                    DDOS_flag = true;
                } else if (strcmp(buf, "Quest") == 0) {
                    if (register_flag == Servent_flag) {     // Servent Bot 才做， 回覆 Client_Bot
                        if (vec_Ddoslist.size() > 0)
                            return 'L';
                        else
                            return 'F';
                    }
                }
                break;
            case 3 :
                strcpy(p1.IP, buf);
                break;
            case 4 :
                strcpy(p1.Port, buf);
                WaitForSingleObject(peerlist_lock, INFINITE);       // Lock
                WaitForSingleObject(trustlist_lock, INFINITE);
                WaitForSingleObject(untrustlist_lock, INFINITE);
                if (ExPL_flag) {

                    if (find_repeat_list(vec_Peerlist, p1) >= 0) {   // 檢查PeerList 對方傳的IP,Port有無重複
                        rtn_flag = 'E'; // 重複
                        if (debug_mode)
                            printf("[Peerlist Repeat] \n");
                    }
                    if (register_flag == Servent_flag) {
                        if (find_repeat_list(vec_Banlist, p1) >= 0) {   // 檢查有無 Ban 封鎖
                            if (debug_mode)
                                printf("[INFO] Receive Ban Bot \n");
                            return 'N';
                        }
                        if (find_repeat_list(vec_Trustlist, p1) >= 0) {   // 檢查TrustList 對方傳的IP,Port有無重複
                            rtn_flag = 'E'; // 重複
                            if (debug_mode)
                                printf("[Trustlist Repeat]\n");
                        }
                        if (find_repeat_list(vec_UnTrustlist, p1) >= 0) {   // 檢查 UnTrustList 對方傳的IP,Port有無重複
                            rtn_flag = 'E'; // 重複
                            if (debug_mode)
                                printf("[UnTrustlist Repeat] \n");
                        }
                    }
                    if (rtn_flag != 'E') {
                        //  無重複
                        if ((vec_Peerlist.size() >= MAXPLNum)) {
                            if (register_flag == Crawler_flag) {
                                int send_num = find_Sended(vec_Peerlist);
                                if (send_num >= 0) {
                                    vec_Peerlist.erase(vec_Peerlist.begin() + send_num);
                                }
                            } else if (register_flag == Servent_flag) {
                                int trust_num = find_Trust(vec_Peerlist);
                                if (trust_num >= 0) {
                                    PeerList temp = vec_Peerlist[trust_num];
                                    if (debug_mode)
                                        printf("[INFO] PeerList Overload -> Into Trust %s:%s\n",
                                               vec_Peerlist[trust_num].IP,
                                               vec_Peerlist[trust_num].Port);
                                    vec_Trustlist.push_back(temp);
                                    vec_Peerlist.erase(vec_Peerlist.begin() + trust_num);
                                }
                            }
                        }
                        if ((vec_Peerlist.size() < MAXPLNum))
                            vec_Peerlist.push_back(p1);
                        rtn_flag = 'E';
                    }
                } else if (DDOS_flag) {
                    if (find_repeat_list(vec_Ddoslist, p1) >= 0)  // 檢查 DDOS 有無重複
                        return 'N';
                    strcpy(p1.ddos_time, time_expire);
                    // 還要處理時間進位
                    vec_Ddoslist.push_back(p1);
                    if (debug_mode)
                        cout << "[Add_DDOS]--->" << p1.IP << ":" << p1.Port << ":" << time_expire << endl;
                    rtn_flag = 'N';         //  社交混合式 不推送DDOS訊息， 等人家自己詢問
                }
                ReleaseMutex(peerlist_lock);
                ReleaseMutex(trustlist_lock);
                ReleaseMutex(untrustlist_lock);     // ReleSE
                break;
            case 5 :
                strcpy(client_ip, buf);
                break;
            case 6 :
                strcpy(client_port, buf);
                break;
        }
        buf = strtok(NULL, p);
        count++;
    }
    if (debug_mode) {
        printf("[Receive] %s:%s--->[%s]\n", client_ip, client_port, output);
    }
    return rtn_flag;
}

void time_to_int(char t[], int temp[]) {
    temp[0] = (t[0] - 48) * 1000 + (t[1] - 48) * 100 + (t[2] - 48) * 10 + (t[3] - 48);  // Year
    temp[1] = (t[4] - 48) * 10 + (t[5] - 48);   // Month
    temp[2] = (t[6] - 48) * 10 + (t[7] - 48);   // Day
    temp[3] = (t[8] - 48) * 10 + (t[9] - 48);   // Hour
    temp[4] = (t[10] - 48) * 10 + (t[11] - 48);   // Minute
    temp[5] = (t[12] - 48) * 10 + (t[13] - 48);   // Second
}

void update_time(int t[]) {
    bool flag = false;
    short day31[7] = {1, 3, 5, 7, 8, 10, 12};
    short year = t[0], month = t[1], result = 0;

    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0 && year % 3200 != 0)) {
            result = 29;
        } else {
            result = 28;
        }
    } else {
        for (short i = 0; i < 7; i++) {
            if (month == day31[i]) {
                flag = true;
                break;
            }
        }
        if (flag) {
            result = 31;
        } else {
            result = 30;
        }
    }

    int temp = 0;
    if (t[5] > 59) {
        temp = t[5] / 60;
        t[5] = t[5] % 60;
        t[4] = t[4] + temp;
    }
    if (t[4] > 59) {
        temp = t[4] / 60;
        t[4] = t[4] % 60;
        t[3] = t[3] + temp;
    }
    if (t[3] > 23) {
        temp = t[3] / 24;
        t[3] = t[3] % 24;
        t[2] = t[2] + temp;
    }
    if (t[2] > result) {
        temp = t[2] / result;
        t[2] = t[2] % result;
        t[1] = t[1] + temp;
    }
    if (t[1] > 12) {
        temp = t[1] / 12;
        t[1] = t[1] % 12;
        t[0] = t[0] + temp;
    }
}

bool add_time(int i, int addNum, int temp[]) {
    if (i > 5 || i < 0)
        return false;
    temp[i] = temp[i] + addNum;
    update_time(temp);
    return true;
}

bool compare_with_time(int a[], int b[]) {  // a < b 回傳 True
    for (int i = 0; i < 6; i++) {
        if (a[i] == b[i]) {
        } else if (a[i] < b[i])
            return true;
        else
            return false;
    }
    return true;
}

int check_exit() {

    HANDLE hFileMap;
    BOOL bResult;
    PCHAR lpBuffer = nullptr;

    hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _T("Bot_Kill"));

    if (!hFileMap) {
        if (debug_mode)
            printf("[Error] OpenFileMapping Failed: %d.\n", GetLastError());
        exit(-1);
    } else {
        if (debug_mode)
            printf("[Check Exit] Success!\n");
    }
    while (true) {
        lpBuffer = (PCHAR) MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 256);

        if (lpBuffer == nullptr) {
            if (debug_mode)
                printf("[Error] MapViewOfFile Failed: %d.\n", GetLastError());
            exit(-1);
        }
        if (strcmp(lpBuffer, "Kill") == 0) {
            if (debug_mode)
                printf("\n[ EXIT ]\n");

            bResult = UnmapViewOfFile(lpBuffer);
            if (!bResult) {
                if (debug_mode)
                    printf("UnmapViewOfFile Failed Error: %d.\n", GetLastError());
                exit(-1);
            } else {
                //if(debug_mode)
                //printf("UnmapViewOfFile Success!\n");
            }
            CloseHandle(hFileMap);
            server_status = false;
            change_mtx.unlock();
            return 0;
        }
        Sleep(2000);
    }
}

int check_time() {
    const char *p = ":";
    char *buf = {};
    char data[20] = {};
    char *buffer;

    while (true) {
        if (OpenClipboard(0)) {
            buffer = (char *) GetClipboardData(CF_TEXT);
            if (buffer) {
                //if(debug_mode)
                //printf("%s\n",buffer);
                strcpy(data, buffer);

                strtok(data, p);             // T
                buf = strtok(NULL, p);       // 時間
                strcpy(time_stamp, buf);

                buf = strtok(NULL, p);       // Rate
                if (register_flag == Servent_flag || register_flag == Client_flag)
                    inspect_hibernate();    // 檢查是否休眠
                if (buf) {
                    time_rate = atoi(buf);
                    //if(debug_mode)
                    //printf("[INFO] Time rate = %d\n",time_rate);
                }
            }
        }
        Sleep(500);
    }
    CloseClipboard();
    return 0;

}

int find_min_rank(vector<PeerList> vec) {
    int pl_num = -1, size = 0, least = 12;
    size = vec.size();
    for (int i = 0; i < size; i++) {
        if (vec[i].rank < least) {   // least = 目前找到的最低聲譽值
            least = vec[i].rank;
            pl_num = i;
        }
    }
    return pl_num;
}

int find_max_rank(vector<PeerList> vec) {
    int pl_num = 0, size = 0, max = 0;
    size = vec.size();
    for (int i = 0; i < size; i++) {
        if (vec[i].rank > max) {   // max = 目前找到的最高聲譽值
            max = vec[i].rank;
            pl_num = i;
        }
    }
    return pl_num;
}

int find_unTrust(vector<PeerList> vec) {
    int pl_num = -1, size = 0, rank = DistrustRank;
    size = vec.size();
    for (int i = 0; i < size; i++) {
        if (vec[i].rank < DistrustRank) {   // 找 PeerList裡有沒有不信任的 Bot
            if (vec[i].rank < rank) { // 找最小的
                rank = vec[i].rank;
                pl_num = i;
            }
        }
    }
    return pl_num;
}

int find_Trust(vector<PeerList> vec) {
    int pl_num = -1, size = 0;
    size = vec.size();
    for (int i = 0; i < size; i++) {
        if (vec[i].rank >= MAXRank) {   // 找 PeerList裡 聲譽值頂點的 Bot
            if (vec[i].trust_flag) { // 聲譽值頂點 且已告知對方Fans_Add
                return i;
            }
        }
    }
    return pl_num;
}

int find_Sended(vector<PeerList> vec) {
    int pl_num = -1, size = 0;
    size = vec.size();
    for (int i = 0; i < size; i++) {
        if (vec[i].send_flag)  // 爬蟲找已經傳送過的刪掉
            return i;
    }
    return pl_num;
}

void rebirth() {
    sleep_status = false;
    check_bot_status = false;
    promotion_flag = false;

    vector<PeerList> temp;
    for (int i = 0; i < vec_Trustlist.size(); i++) {       // 先加 Trust List 的
        vec_Trustlist[i].rank = HSTDRank;
        temp.push_back(vec_Trustlist[i]);
        /*
        _socket c( (char*)vec_Trustlist[i].IP,  (char*)vec_Trustlist[i].Port, buffer_size);    // 測試對方有無存活
        if(c.get_status()){
            c.send_((char*)"Test_Alive");
            if(c.check_recv_(10000)) {   // Check recv
                c.recv_();
                temp.push_back(vec_Trustlist[i]);
            }
            c.close_();
        }*/
    }
    for (int i = 0; i < vec_Peerlist.size(); i++) {    // PeerList 中超過(可信任)標準的留下
        if (vec_Peerlist[i].rank > HSTDRank) {
            vec_Peerlist[i].rank = HSTDRank;
            temp.push_back(vec_Peerlist[i]);
        }
    }

    if (temp.size() > MAXPLNum) {
        for (int i = 0; i < temp.size() - MAXPLNum; i++) {  // 超過多少就刪幾個
            temp.pop_back();        // 刪除尾端元素
        }
    }
    vec_Peerlist = temp;
    vec_Trustlist.clear();  // Trustlist 清空
    if (debug_mode)
        printf("[INFO] ReBirth Finish\n");
}

void check() {
    int distrust_num = -1;
    char str[25] = {};

    WaitForSingleObject(untrustlist_lock, INFINITE);    // Lock
    WaitForSingleObject(peerlist_lock, INFINITE);

    distrust_num = find_min_rank(vec_UnTrustlist);
    if (distrust_num < 0) {   // UnTrustlist Size = 0
        distrust_num = find_unTrust(vec_Peerlist);
        if (distrust_num < 0) {   // UnTrustlist & Peerlist 都找不到低聲譽值
            ReleaseMutex(untrustlist_lock);     // 先Release才能退出
            ReleaseMutex(peerlist_lock);
            return;
        } else {
            strcpy(checkTarget.IP, vec_Peerlist[distrust_num].IP);
            strcpy(checkTarget.Port, vec_Peerlist[distrust_num].Port);
            checkTarget.rank = vec_Peerlist[distrust_num].rank;
            vec_Peerlist.erase(vec_Peerlist.begin() + distrust_num);
        }
    } else {
        strcpy(checkTarget.IP, vec_UnTrustlist[distrust_num].IP);
        strcpy(checkTarget.Port, vec_UnTrustlist[distrust_num].Port);
        checkTarget.rank = vec_UnTrustlist[distrust_num].rank;
        vec_UnTrustlist.erase(vec_UnTrustlist.begin() + distrust_num);
    }
    ReleaseMutex(untrustlist_lock); // Release
    ReleaseMutex(peerlist_lock);

    if (checkTarget.rank < DistrustRank) {

        check_now = true;
        mapCheck.clear();   // map 清空

        strcat(str, checkTarget.IP);
        strcat(str, checkTarget.Port);
        mapCheck[str] = checkTarget.rank;

        char send_data[buffer_size] = {};
        int time_t[6] = {};
        char time_expire[15] = {};

        time_to_int(time_stamp, time_t); //   [0]年 [1]月 [2]日 [3]時 [4]分 [5]秒
        add_time(2, 3, time_t);   // 訊息三天過期
        sprintf(time_expire, "%04d%02d%02d%02d%02d%02d", time_t[0], time_t[1], time_t[2], time_t[3], time_t[4],
                time_t[5]);
        add_time(2, 1, time_t);   // 過期後一天回收訊息
        sprintf(checkTimeExpire, "%04d%02d%02d%02d%02d%02d", time_t[0], time_t[1], time_t[2], time_t[3], time_t[4],
                time_t[5]);

        check_num = rand(); // 編號
        //  格式[ Check:編號:到期時間:檢查目標IP:檢查目標Port:發送端IP:發送端Port ]
        sprintf(send_data, "%s:%d:%s:%s:%s:%s:%s", "Check", check_num, time_expire, checkTarget.IP,
                checkTarget.Port, my_ip, my_port);
        send_check_mes(send_data, true);
        if (debug_mode)
            printf("------------------[Start Check No.%d] %s:%s = %d-------------------\n", check_num, checkTarget.IP,
                   checkTarget.Port, checkTarget.rank);
    }
}

void check_bot_report() {

    bool expire_flag = false;
    int now_int[6], expire_int[6];
    int rankSum = 0;
    time_to_int(checkTimeExpire, expire_int);
    time_to_int(time_stamp, now_int);

    if (!compare_with_time(now_int, expire_int))
        expire_flag = true;
    // 時間已過期
    if (expire_flag) {
        map<string, int>::iterator iter;
        for (iter = mapCheck.begin(); iter != mapCheck.end(); iter++) {
            rankSum += iter->second;
        }
        rankSum = rankSum / mapCheck.size();

        printf("[Check] %s:%s  Rank.Avg = %d   Map_Size = %d\n", checkTarget.IP, checkTarget.Port, rankSum,
               mapCheck.size());

        if (rankSum <= DistrustRank) {
            char send_data[30] = {};
            _socket c(Concole_IP, Concole_Port, buffer_size); // socket Console
            if (c.get_status()) {
                sprintf(send_data, "Ban:%s:%s", checkTarget.IP, checkTarget.Port);
                while (c.send_((char *) send_data) < 0) {
                    Sleep(2000);    // 傳失敗就一直傳
                    if (debug_mode)
                        printf("[Report] Send Console Failed\n");
                }
                if (debug_mode)
                    printf("[Report]--->Console :[%s]\n", send_data);
                //c.shutdown_(_socket::BOTH);
                c.close_();
                mapCheck.clear();
            } else {
                if (debug_mode)
                    printf("[INFO] Connect Console failed\n");
                c.close_();
            }
        }
        check_now = false;
        mapCheck.clear();
    }
}

void handle_check_mes(char data[]) {
    const char *p = ":";
    char *buf = {}, time_expire[15] = {};
    char temp[buffer_size] = {};
    char mes[buffer_size] = {};
    bool expire_flag = false, check_flag = false;
    int count = 0;
    PeerList tar, source;

    strcpy(temp, data);
    strcpy(mes, data);
    buf = strtok(temp, p);
    // 分析 Check 的訊息
    while (buf) {
        switch (count) {
            case 0:
                // Check
                break;
            case 1:
                // 編號
                if (check_bot_status) {
                    int temp = atoi(buf);
                    if (check_num == temp)
                        check_flag = true;
                }
                break;
            case 2:
                strcpy(time_expire, buf);
                int now_int[6], expire_int[6];
                time_to_int(time_expire, expire_int);
                time_to_int(time_stamp, now_int);
                if (!compare_with_time(now_int, expire_int))
                    expire_flag = true;
                // 時間已過期
                break;
            case 3:
                memset(tar.IP, '\0', 20);
                strcpy(tar.IP, buf);
                break;
            case 4:
                memset(tar.Port, '\0', 6);
                strcpy(tar.Port, buf);
                break;
            case 5:
                memset(source.IP, '\0', 20);
                strcpy(source.IP, buf);
                break;
            case 6:
                memset(source.Port, '\0', 6);
                strcpy(source.Port, buf);
                break;
        }
        buf = strtok(NULL, p);
        count++;
        if (count > 6)
            break;
    }

    char str[25] = {};
    int rank = 0;

    count = 0;
    if (check_bot_status && check_flag) {   // 是CheckBot 且訊息編號與正在詢問的相同
        // CheckBot 統計
        while (buf) {
            if (count % 3 == 0)
                strcat(str, buf);
            else if (count % 3 == 1)
                strcat(str, buf);
            else {
                rank = atoi(buf);
                mapCheck[str] = rank;
                memset(str, '\0', sizeof(str));
            }
            buf = strtok(NULL, p);
            count++;
        }
    } else {
        // Servent 傳遞
        while (buf) {
            buf = strtok(NULL, p);
            count++;
        }
        count = count / 3;
        if (expire_flag || count >= 6) {        // 過期 or 滿六筆資料 直接回傳給 Check Bot
            _socket c((char *) source.IP, (char *) source.Port, buffer_size);
            if (c.get_status()) {
                c.send_(data);
                //c.shutdown_(_socket::BOTH);
                c.close_();
            } else {
                c.close_();
            }
            return;
        } else {
            // 尋找目標
            int tar_num = find_repeat_list(vec_UnTrustlist, tar);  // 找 UnTrustlist
            if (tar_num >= 0)
                rank = vec_UnTrustlist[tar_num].rank;
            else {
                tar_num = find_repeat_list(vec_Peerlist, tar);          // 找 Peerlist
                if (tar_num >= 0)
                    rank = vec_Peerlist[tar_num].rank;
                else {
                    tar_num = find_repeat_list(vec_Trustlist, tar);         // 找 Trustlist
                    if (tar_num >= 0)
                        rank = vec_Trustlist[tar_num].rank;
                }
            }
            if (tar_num >= 0) {   // 找到目標
                char add_mes[20] = {};
                sprintf(add_mes, ":%s:%s:%d", my_ip, my_port, rank);
                strcat(data, add_mes); // 加上自己的訊息
                // 再問別人
                send_check_mes(data, true);
            } else
                send_check_mes(data, true);    // 問兩人
        }
    }
}

int find_repeat_list(vector<PeerList> vec, PeerList p) {
    for (int i = 0; i < vec.size(); i++) { // 檢查 對方傳的IP,Port有無重複
        if (strcmp(vec[i].IP, p.IP) == 0)
            if (strcmp(vec[i].Port, p.Port) == 0) {
                return i;    // 重複的
            }
    }
    return -1;   //  無重複
}

void send_check_mes(char send_data[], bool write_flag) {
    int num1 = -1, num2 = -1;
    if (vec_Trustlist.size() >= 2) {
        // 兩個都從 TrustList 問
        WaitForSingleObject(trustlist_lock, INFINITE);      // Lock

        num1 = rand() % vec_Trustlist.size();
        do {
            num2 = rand() % vec_Trustlist.size();
        } while (num1 == num2);

        _socket c1((char *) vec_Trustlist[num1].IP, (char *) vec_Trustlist[num1].Port, buffer_size);
        if (c1.get_status()) {
            c1.send_(send_data);
            if (debug_mode)
                printf("[Send] --> %s:%s [%s]\n", vec_Trustlist[num1].IP, vec_Trustlist[num1].Port, send_data);
            //c1.shutdown_(_socket::BOTH);
            c1.close_();
        } else {
            c1.close_();
        }
        // 自己有寫入訊息 才傳給2個人 否則只傳一個
        if (write_flag) {
            _socket c2((char *) vec_Trustlist[num2].IP, (char *) vec_Trustlist[num2].Port, buffer_size);
            if (c2.get_status()) {
                c2.send_(send_data);
                if (debug_mode)
                    printf("[Send] --> %s:%s [%s]\n", vec_Trustlist[num2].IP, vec_Trustlist[num2].Port, send_data);
                // c2.shutdown_(_socket::BOTH);
                c2.close_();
            } else {
                c2.close_();
            }
        }

        ReleaseMutex(trustlist_lock);       // Release

    } else if (vec_Trustlist.size() == 1) {
        // TrustList , PeerList 各傳一個
        WaitForSingleObject(peerlist_lock, INFINITE);       // Lock
        WaitForSingleObject(trustlist_lock, INFINITE);

        num1 = 0;
        num2 = find_max_rank(vec_Peerlist);

        _socket c1((char *) vec_Trustlist[num1].IP, (char *) vec_Trustlist[num1].Port, buffer_size);
        if (c1.get_status()) {
            c1.send_(send_data);
            if (debug_mode)
                printf("[Send] --> %s:%s [%s]\n", vec_Trustlist[num1].IP, vec_Trustlist[num1].Port, send_data);
            //c1.shutdown_(_socket::BOTH);
            c1.close_();
        } else {
            c1.close_();
        }
        // 自己有寫入訊息 才傳給2個人 否則只傳一個
        if (write_flag) {
            _socket c2((char *) vec_Peerlist[num2].IP, (char *) vec_Peerlist[num2].Port, buffer_size);
            if (c2.get_status()) {
                c2.send_(send_data);
                if (debug_mode)
                    printf("[Send] --> %s:%s [%s]\n", vec_Peerlist[num2].IP, vec_Peerlist[num2].Port, send_data);
                //c2.shutdown_(_socket::BOTH);
                c2.close_();
            } else {
                c2.close_();
            }
        }

        ReleaseMutex(peerlist_lock);
        ReleaseMutex(trustlist_lock);       // Release

    } else {
        // 至少有一個要是 Trust List
    }
}

void dice_hibernate_time() {
    if (rand_num < 10)
        hibernate_time = 22;    // 10點休眠
    else if (rand_num < 20)
        hibernate_time = 23;    // 11點休眠
    else if (rand_num < 30)
        hibernate_time = 0;     // 12點休眠
    else if (rand_num < 50)
        hibernate_time = 1;     // 1點休眠
    else if (rand_num < 60)
        hibernate_time = 2;     // 2點休眠
    else if (rand_num < 70)
        hibernate_time = 3;     // 3點休眠
    else
        hibernate_time = -1;
}

void inspect_hibernate() {
    int time_t[6] = {};
    if (hibernate_time < 0) {
        return;
    }
    time_to_int(time_stamp, time_t);    //   [0]年 [1]月 [2]日 [3]時 [4]分 [5]秒

    if (hibernate_time == 22 || hibernate_time == 23) {
        if ((time_t[3] > 21) || (time_t[3] < ((hibernate_time + Offline_hr) % 24))) {
            if (!hibernate_mode) {
                hibernate_mode = true;
                if (debug_mode)
                    printf("\n[Hibernate]\n");
            }
        } else if (hibernate_mode) {
            hibernate_mode = false;
            if (debug_mode)
                printf("\n[Wake Up]\n");
        }
    } else {
        if ((time_t[3] > hibernate_time) && (time_t[3] < (hibernate_time + Offline_hr))) {
            if (!hibernate_mode) {
                hibernate_mode = true;
                if (debug_mode)
                    printf("\n[Hibernate]\n");
            }
        } else if (hibernate_mode) {
            hibernate_mode = false;
            if (debug_mode)
                printf("\n[Wake Up]\n");
        }
    }
}

bool loss_rate_choose() {
    int percent = 0, temp = 0;
    if (loss_rate == 0)
        percent = 0;
    else if (loss_rate == 1)
        percent = 10;
    else if (loss_rate == 2)
        percent = 20;
    else if (loss_rate == 3)
        percent = 30;

    temp = rand() % 100;

    if (temp < percent)
        return true;
    else
        return false;

}