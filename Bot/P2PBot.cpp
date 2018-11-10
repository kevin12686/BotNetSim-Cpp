#include <iostream>
#include <ctime>
#include <queue>
#include "_socketserver.h"
#include "_thread.h"
#include <mutex>
#include <tchar.h>

using namespace std;
typedef struct peerlist {
    char IP[16];
    char Port[5];
    char ddos_time[15];
} PeerList;
vector<PeerList> vec_Peerlist;      // peerlist清單
vector<PeerList> vec_Ddoslist;
vector<PeerList> vec_Sensorlist;

bool register_console();

bool register_controller();

void accept_thread();

void client_thread();

void bot();

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

int find_repeat_list(vector<PeerList>, PeerList);

void dice_hibernate_time();

void inspect_hibernate();

bool loss_rate_choose();

HANDLE QMutex;                      // Lock
HANDLE peerlist_lock;

mutex change_mtx;
queue<_socket *> ClientQ;            // Collect parameters for client_thread
vector<_thread *> ClientT;          // Collect _thread

_socketserver *s;
#define IDLE 4000
#define change_time 3600000 // 兩小時交換一次
#define Host_flag  -1
#define Bot_flag 1
#define Crawler_flag 2
#define Sensor_flag  3
#define Offline_hr  4

bool server_status = false;
bool debug_mode = false;
bool hibernate_mode = false;
char my_ip[15] = "127.0.0.1", my_port[15] = "8000";
char c_ip[15] = "127.0.0.1", c_port[15] = "6666";
char *Concole_IP = c_ip, *Concole_Port = c_port;

char time_stamp[15] = "20180823115900";
const int MAXPLNum = 15;
int register_flag = Host_flag; // Bot = 1, Crawler = 2, Sensor = 3
int delay = 0;
int time_rate = 2000;
int hibernate_time = -1;
int rand_num, loss_rate;

int main(int argc, char *argv[]) {
    change_mtx.lock();

    strcpy(my_ip, argv[0]);
    strcpy(my_port, argv[1]);
    strcpy(c_ip, argv[2]);
    strcpy(c_port, argv[3]);
    rand_num = atoi(argv[5]);
    loss_rate = atoi(argv[6]);

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
    s = new _socketserver((char *) my_port, 1024);
    if (!s->get_status()) // 檢查port是否可用
        return -1;

    _thread accept_t((int (*)()) accept_thread);    // accept
    accept_t.start();
    _thread exit_t((int (*)()) check_exit);       // check_exit
    exit_t.start();
    _thread time_t((int (*)()) check_time);       // check_exit
    time_t.start();

    // 與Console註冊
    bool console_flag = false;
    while ((!console_flag) && server_status) {
        console_flag = register_console();
        Sleep(5000);
    }
    change_mtx.lock(); // 鎖住，等待 Change 指令

    if (register_flag == Bot_flag) {
        _thread bot_t((int (*)()) bot);
        bot_t.start();
    } else if (register_flag == Crawler_flag) {
        _thread crawler_t((int (*)()) crawler);
        crawler_t.start();
    } else if (register_flag == Sensor_flag) {
        _thread sensor_t((int (*)()) sensor);
        sensor_t.start();
    }

    // Controller註冊
    bool controller_flag = false;
    int fail_reg_count = 0;
    while ((!controller_flag) && server_status) {
        controller_flag = register_controller();
        if (!controller_flag) {
            fail_reg_count++;
        }
        Sleep(1000);
    }

    int inst = 0;
    int num = 0;
    if (debug_mode) {
        while (server_status) {
            printf("\n----------------------------------\n");
            printf("-----Input '0'  :Bot Status ------\n");
            printf("-----Input '1'  :Peer List -------\n");
            printf("-----Input '2'  :DDOS List--------\n");
            printf("-----Input '3'  :Time ------------\n");
            printf("-----Input '4'  :Sleep Status-----\n");
            printf("-----Input '5'  :Loss Rate--------\n");
            printf("-----Input '6'  :Clear Screen-----\n");
            printf("-----Input '-1' :Exit-------------\n");
            printf("----------------------------------\n");
            scanf("%d", &inst);
            if (inst < -1 || inst > 10) {
                printf("[Input Error]\n");
                continue;
            }
            if (inst == 0) {
                if (register_flag == Bot_flag)
                    printf("-------Bot--------\n");
                else if (register_flag == Crawler_flag)
                    printf("-------Crawler--------\n");
                else if (register_flag == Sensor_flag)
                    printf("-------Sensor--------\n");
                printf("[My IP]: %s\n", my_ip);
                printf("[My Port]: %s\n", my_port);
            } else if (inst == 1) {
                printf("\n-----------PeerList Size = %d -----------\n", vec_Peerlist.size());
                for (int i = 0; i < vec_Peerlist.size(); i++)
                    printf("[No.%d %s:%s]\n", i, vec_Peerlist[i].IP, vec_Peerlist[i].Port);
            } else if (inst == 2) {
                int len = vec_Ddoslist.size();
                for (int i = 0; i < len; i++)
                    printf("[DDOS Queue No.%d] %s:%s  [Time] %s\n", i + 1, vec_Ddoslist[i].IP, vec_Ddoslist[i].Port,
                           vec_Ddoslist[i].ddos_time);
            } else if (inst == 3){
                printf("[Time] :%s:%d\n", time_stamp, time_rate);
            } else if (inst == 4) {
                printf("[Goot Night at %d]\n", hibernate_time);
                if (hibernate_mode)
                    printf("[Sleep Now]\n");
                else
                    printf("[Activive]\n");
            }else if (inst == 5) {
                printf("[Loss Rate] = %d %% \n", loss_rate * 10);
            } else if (inst == 6) {
                system("CLS");
            }
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

    char *send_data = new char[1024];
    char *rec_data = new char[1024];
    const char *p = ":";
    char *buf = {};
    bool flag = false;
    // Console
    _socket c(Concole_IP, Concole_Port, 1024); // socket Console
    if (c.get_status()) {
        memset(rec_data, '\0', 1024);
        sprintf(send_data, "HOST%s:%s", my_ip, my_port);
        if (c.send_(send_data) > 0) {
            if (debug_mode)
                cout << "Send to Console :" << send_data << endl;
            rec_data = c.recv_();   // 等待Console回覆
            if (rec_data) {
                if (debug_mode)
                    cout << "[Receive] Console :" << rec_data << endl << endl;
                buf = strtok(rec_data, p);
                if (strcmp(buf, "OK") == 0) {
                    register_flag = Host_flag;
                    flag = true;
                }
            } else {
                if (debug_mode)
                    printf("[INFO] Register Console failed\n");
                register_flag = Host_flag;
            }
        }
        c.shutdown_(_socket::BOTH);
        c.close_();
    } else {
        c.close_();
        if (debug_mode)
            printf("[INFO] Register Console failed\n");
        register_flag = Host_flag;
    }


    delete[] send_data;
    delete[] rec_data;
    return flag;
}

bool register_controller() {
    char *send_data = new char[1024];
    char *rec_data = new char[1024];
    bool flag = false;
    // Controller
    _socket ctrl((char *) "127.0.0.1", (char *) "1999", 1024); // socket Controller
    if (ctrl.get_status()) {
        sprintf(send_data, "S%s", my_port);
        if (ctrl.send_(send_data) > 0) {
            if (debug_mode)
                cout << "Send to Controller :" << send_data << endl;
            if (ctrl.check_recv_(IDLE)) {   // Check recv
                rec_data = ctrl.recv_();
                if (rec_data) {
                    if (strcmp(rec_data, "OK") == 0)
                        flag = true;
                    if (debug_mode)
                        printf("[Receive] Controller :OK\n");
                } else {
                    if (debug_mode)
                        printf("[INFO] Register Controller failed\n");
                }
            } else {
                if (debug_mode)
                    printf("[INFO] Register Controller failed\n");
            }
        }
        ctrl.shutdown_(_socket::BOTH);
        ctrl.close_();
    } else {
        ctrl.close_();
        if (debug_mode)
            printf("[INFO] Register Controller failed\n");
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
            //t = new _thread((int (*)()) client_thread);
            //t->start();
            //ClientT.push_back(t);
        }
    }/*
    vector<_thread *>::iterator it_i;
    for (it_i = ClientT.begin(); it_i != ClientT.end(); it_i++) {
        (*it_i)->join();
        delete (*it_i);
    }*/
    if (debug_mode)
        cout << "Stop accepting..." << endl;
}

void client_thread() {
    srand((unsigned) time(NULL));
    WaitForSingleObject(QMutex, INFINITE);
    _socket *client = ClientQ.front();
    ClientQ.pop();
    ReleaseMutex(QMutex);
    char *rec_data ;
    char rec_flag;
    char send_data[1024] = {};
    memset(send_data, '\0', 1024);
    rec_data = client->recv_();
    if (!hibernate_mode){   // 休眠不接收訊息
        if (rec_data) {
            WaitForSingleObject(peerlist_lock, INFINITE);       // Lock
            rec_flag = handle_recv_mes(rec_data);
            ReleaseMutex(peerlist_lock);        // Release

            /*if (register_flag == Bot_flag || register_flag == Crawler_flag) {
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
                WaitForSingleObject(peerlist_lock, INFINITE);       // Lock
                handle_send_mes(rec_flag, send_data);
                ReleaseMutex(peerlist_lock);        // Release
                client->send_(send_data);
            } else if (rec_flag == 'D') {
                send_mes('D');
            } else if ((rec_flag == 'B') || (rec_flag == 'C')) {    // 註冊 要求 List
                client->send_((char *) "Request:Peerlist");
                memset(rec_data, '\0', 1024);
                rec_data = client->recv_();
                if (rec_data) {
                    handle_recv_mes(rec_data);
                    if (rec_flag == 'B') {
                        client->send_((char *) "EXIT");
                        register_flag = Bot_flag;
                        change_mtx.unlock();
                    } else if (rec_flag == 'C') {
                        client->send_((char *) "Request:Sensorlist");
                        memset(rec_data, '\0', 1024);
                        rec_data = client->recv_();
                        if (rec_data) {
                            register_flag = Crawler_flag;
                            handle_recv_mes(rec_data);
                            client->send_((char *) "EXIT");
                            change_mtx.unlock();
                        }
                    }
                } else {
                    if (debug_mode)
                        printf("[INFO] Request Peerlist failed\n");
                    register_flag = Host_flag;
                }
            } else if (rec_flag == 'S') {
                register_flag = Sensor_flag;
                change_mtx.unlock();
            }
        }
    }

    client->close_();
    delete client;
    delete[] rec_data;
}

void bot() {
    srand((unsigned) time(NULL));
    if (debug_mode)
        printf("-------Bot--------\n");
    Sleep(3000);
    int count_time = 0;
    while (server_status) {    // Server 有無啟動
        if (count_time >= change_time) {
            count_time = 0;
            if (vec_Peerlist.size() > 1){
                WaitForSingleObject(peerlist_lock, INFINITE);       // Lock
                send_mes('E');        // Exchange_PeerList
                ReleaseMutex(peerlist_lock);        // Release
            }
            if (vec_Ddoslist.size() > 0) {
                if (strcmp(time_stamp, vec_Ddoslist[0].ddos_time) >= 0)
                    send_mes('A');    // Attack
            }
        }
        Sleep(500);
        count_time += 500 * time_rate;
    }
};

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
    char *rec_data = new char[1024];
    char *send_data = new char[1024];

    send_num = handle_send_mes(flag, send_data);
    if (flag == 'E') {        // 交換 Peerlist
        _socket c((char *) vec_Peerlist[send_num].IP, (char *) vec_Peerlist[send_num].Port, 1024);
        if (c.get_status()) {
            c.send_(send_data);
            if (debug_mode)
                cout << "[Send]--->" << vec_Peerlist[send_num].IP << ":" << vec_Peerlist[send_num].Port << ":["
                     << send_data << "]" << endl;
            if (c.check_recv_(IDLE)) {   // Check recv
                rec_data = c.recv_();
                if (rec_data)
                    handle_recv_mes(rec_data);
            } else {
                if (debug_mode)
                    printf("Recv Time out\n");
            }
            c.shutdown_(_socket::BOTH);
            c.close_();
        }
        c.close_();
    } else if (flag == 'D') {    // DDOS
        for (int i = 0; i < vec_Peerlist.size(); i++) {
            _socket c(vec_Peerlist[i].IP, vec_Peerlist[i].Port, 1024); // socket
            if (c.get_status()) {
                c.send_(send_data);
                c.shutdown_(_socket::BOTH);
                c.close_();
            } else {
                c.close_();
            }

            Sleep(500);
        }
    } else if (flag == 'R') {    // 回報抓到的 Bot
        _socket c((char *) "127.0.0.1", (char *) "1999", 1024);
        if (c.get_status()) {
            c.send_(send_data);
            vec_Peerlist.erase(vec_Peerlist.begin());
            if (debug_mode)
                cout << "[Report] " << "Controller " << "--->:[" << send_data << "]" << endl;
            c.shutdown_(_socket::BOTH);
            c.close_();
        } else {
            c.close_();
        }

    } else if (flag == 'A') {    // Attack DDOS IP
        _socket c(vec_Ddoslist[0].IP, vec_Ddoslist[0].Port, 1024); // socket
        if (c.get_status()) {
            c.send_((char *) "DDOS");
            if (debug_mode) {
                printf("[DDOS]--->%s:%s  [Time] %s\n", vec_Ddoslist[0].IP, vec_Ddoslist[0].Port,
                       vec_Ddoslist[0].ddos_time);
            }
            vec_Ddoslist.erase(vec_Ddoslist.begin());
            c.shutdown_(_socket::BOTH);
            c.close_();
        } else {
            if (debug_mode)
                printf("[DDOS] Can't Connect\n");
            c.close_();
        }

    }
    delete[] rec_data;
    delete[] send_data;
}

int handle_send_mes(char flag, char *send_buf) {
    //  格式[ 發送時間:到期時間:指令:目標IP:目標Port:發送端IP:發送端Port ]
    memset(send_buf, '\0', 1024);
    int send_num = -1, time_t[6] = {};
    char time_expire[15] = {}, instr[20];

    time_to_int(time_stamp, time_t); //   [0]年 [1]月 [2]日 [3]時 [4]分 [5]秒
    add_time(2, 3, time_t); // 訊息三天過期
    sprintf(time_expire, "%04d%02d%02d%02d%02d%02d", time_t[0], time_t[1], time_t[2], time_t[3], time_t[4], time_t[5]);

    send_num = rand() % vec_Peerlist.size();

    int tar_num = -1;
    if (flag == 'E') {
        strcpy(instr, "Exchange_peerlist");
        do {
            tar_num = rand() % vec_Peerlist.size();
        } while ((send_num == tar_num) || tar_num < 0);
        if (register_flag == Bot_flag) // Bot
            sprintf(send_buf, "%s:%s:%s:%s:%s:%s:%s", time_stamp, time_expire, instr, vec_Peerlist[tar_num].IP,
                    vec_Peerlist[tar_num].Port, my_ip, my_port);
        if (register_flag == Crawler_flag) {// Crawler
            int sensor_num = rand() % vec_Sensorlist.size();
            sprintf(send_buf, "%s:%s:%s:%s:%s:%s:%s", time_stamp, time_expire, instr, vec_Sensorlist[sensor_num].IP,
                    vec_Sensorlist[sensor_num].Port, my_ip, my_port);
        }

    } else if (flag == 'D') {
        strcpy(instr, "DDOS");
        tar_num = vec_Ddoslist.size() - 1;
        sprintf(send_buf, "%s:%s:%s:%s:%s:%s:%s", time_stamp, vec_Ddoslist[tar_num].ddos_time, instr,
                vec_Ddoslist[tar_num].IP, vec_Ddoslist[tar_num].Port, my_ip, my_port);
    } else if (flag == 'R') { // Report
        sprintf(send_buf, "R%s:%s", vec_Peerlist[0].IP, vec_Peerlist[0].Port);
    } else
        return -1;
    return send_num;
}

char handle_recv_mes(char data[]) {
    bool ExPL_flag = false, DDOS_flag = false, update_time_flag = false;
    int count = 0;
    char *buf = {}, tar_ip[16] = {}, tar_port[5] = {}, time_expire[15] = {}, client_ip[16] = {}, client_port[5] = {};
    char rtn_flag = 'N';
    char output[1024];
    const char *p = ":";
    PeerList p1;
    strcpy(output, data);
    buf = strtok(data, p);
    memset(tar_ip, '\0', 16);
    memset(tar_port, '\0', 5);
    while (buf) {
        switch (count) {
            case 0 :
                if (strcmp(buf, "Kill") == 0) {       // 關閉程式
                    server_status = false;
                    change_mtx.unlock();
                    return 'N';
                } else if (strcmp(buf, "C") == 0) {     // 更改時間頻率
                    buf = strtok(NULL, p);
                    time_rate = atoi(buf);
                    if (debug_mode)
                        printf("[Update Time Rate] :%d\n", time_rate);
                    return 'N';
                } else if (strcmp(buf, "T") == 0)
                    update_time_flag = true;
                else if (strcmp(buf, "Change") == 0) {
                    buf = strtok(NULL, p);
                    if (strcmp(buf, "Bot") == 0) {
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
                            register_flag = Bot_flag;
                            change_mtx.unlock();
                            if (debug_mode)
                                cout << "[Receive] Console :" << output << endl << endl;
                            return 'N';
                        }
                    } else if (strcmp(buf, "Crawler") == 0)
                        rtn_flag = 'C';
                    else if (strcmp(buf, "Sensor") == 0)
                        rtn_flag = 'S';
                } else if (strcmp(buf, "Peerlist") == 0) {
                    buf = strtok(NULL, p);
                    count = 0;
                    while (buf) {
                        count = count % 2;
                        if (count == 0) {
                            memset(p1.IP, '\0', 16);
                            strcpy(p1.IP, buf);
                        } else {
                            memset(p1.Port, '\0', 5);
                            strcpy(p1.Port, buf);
                            bool flag = true;
                            for (int i = 0; i < vec_Peerlist.size(); i++) { // 檢查 對方傳的IP,Port有無重複
                                if (strcmp(vec_Peerlist[i].IP, p1.IP) == 0)
                                    if (strcmp(vec_Peerlist[i].Port, p1.Port) == 0) {
                                        flag = false;
                                        break;
                                    }
                            }
                            if (flag)
                                vec_Peerlist.push_back(p1);
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
                    if (register_flag == Host_flag)
                        return 'N';
                    ExPL_flag = true;
                } else if (strcmp(buf, "DDOS") == 0) {
                    if (register_flag == Host_flag)
                        return 'N';
                    DDOS_flag = true;
                }
                break;
            case 3 :
                strcpy(p1.IP, buf);
                break;
            case 4 :
                strcpy(p1.Port, buf);
                if (ExPL_flag) {
                    if (find_repeat_list(vec_Peerlist, p1) >= 0) {   // 檢查PeerList 對方傳的IP,Port有無重複
                        rtn_flag = 'E'; // 重複
                        if (debug_mode)
                            printf("[Peerlist Repeat] \n");
                    }
                    if (rtn_flag != 'E') {
                        vec_Peerlist.push_back(p1);
                        if ((vec_Peerlist.size() > MAXPLNum)) {    // Bot PeerList太多要刪掉
                            int rand_num = rand() % vec_Peerlist.size();
                            vec_Peerlist.erase(vec_Peerlist.begin() + rand_num);
                        }
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
                    rtn_flag = 'D';
                }
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

bool compare_with_time(int a[], int b[]) {
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
                if (register_flag == Bot_flag)
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

int find_repeat_list(vector<PeerList> vec, PeerList p) {
    for (int i = 0; i < vec.size(); i++) { // 檢查 對方傳的IP,Port有無重複
        if (strcmp(vec[i].IP, p.IP) == 0)
            if (strcmp(vec[i].Port, p.Port) == 0) {
                return i;    // 重複的
            }
    }
    return -1;   //  無重複
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
        if ((time_t[3] >= hibernate_time) && (time_t[3] < (hibernate_time + Offline_hr))) {
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