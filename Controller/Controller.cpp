#include <iostream>
#include <queue>
#include <string>
#include "_thread.h"
#include <sstream>
#include <iomanip>
#include <assert.h>
#include <thread>
#include <map>
#include <algorithm>
#include "_socket.h"
#include "_socketserver.h"

#define LOCAL_PORT 1999
#define REPORT_INTERVALS 5
#define REVIVE_INTERVALS 5

using namespace std;

void socketAccept();

void messageHandle();

void report();

void revive();

std::ostringstream calculate_time(char *);

// Lock
HANDLE QMutex;

// Collect parameters for client_thread
queue<_socket *> clientSocket;

// Collect _thread
vector<_thread *> ClientT;

vector<char *> suspect;
vector<char *> host;
map<char *, short> disconnect;

_socketserver *listenSocket;

bool server_status = false;

char *CONSOLE_IP_ADDRESS;
char *LOCAL_IP_ADDRESS;
int LOCAL_TIMEZONE;

bool controller_debug = false;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "Please input three parameters \'console ip\', \'local ip\', \'timezone\'." << endl;
        return 0;
    }

    size_t console_ip_length = strlen(argv[1]) + 1;
    CONSOLE_IP_ADDRESS = new char[console_ip_length]{0};
    strcpy_s(CONSOLE_IP_ADDRESS, console_ip_length, argv[1]);

    size_t local_ip_length = strlen(argv[2]) + 1;
    LOCAL_IP_ADDRESS = new char[local_ip_length]{0};
    strcpy_s(LOCAL_IP_ADDRESS, local_ip_length, argv[2]);

    LOCAL_TIMEZONE = std::atoi(argv[3]);

    QMutex = CreateMutex(nullptr, false, nullptr);
    if (QMutex == nullptr) {
        cout << "CreateMutex Error. ErrorCode=" << GetLastError() << endl;
        return 1;
    }


    // 創建監聽socket

    WSADATA wsadata;
    _socket::wsastartup_(&wsadata);

    server_status = true;

    _socket socket2Console(CONSOLE_IP_ADDRESS, (char *) "6666", 1024);
    char msg[30] = "CTRL";
    strcat(msg, (const char *) LOCAL_IP_ADDRESS);
    strcat(msg, (const char *) ":");
    strcat(msg, std::to_string(LOCAL_PORT).c_str());
    if (socket2Console.send_(msg) == -1) {
        cout << "Send Message To Console Failed" << endl;
    }
    socket2Console.shutdown_(_socket::BOTH);
    socket2Console.close_();


    listenSocket = new _socketserver((char *) "1999", 1024);

    // 收訊息Thread
    _thread accept_t((int (*)()) socketAccept);
    accept_t.start();

    // 回報Thread
    _thread report_t((int (*)()) report);
    report_t.start();

    _thread revive_t((int (*)()) revive);
    revive_t.start();

    Sleep(1000);

    cout << "[INFO] Controller Start!" << endl;
    cout << "[INSTRUCTION] Input \'L\' To ALL." << endl;
    cout << "[INSTRUCTION] Input \'H\' To Show Hosts." << endl;
    cout << "[INSTRUCTION] Input \'S\' To Show Suspects." << endl;
    cout << "[INSTRUCTION] Input \'E\' To Exit." << endl;

    char operation;
    bool FLAG = true;
    vector<char *>::iterator each;
    do {
        cout << ">";
        cin >> operation;
        switch (operation) {
            case 'L':
                cout << "Host Number: " << host.size() << endl;
                cout << "Suspect Number: " << suspect.size() << endl;
                break;
            case 'H':
                for (each = host.begin(); each != host.end(); each++) {
                    cout << "HOST => " << LOCAL_IP_ADDRESS << ":" << *each << " [DISCONNECT:" << disconnect[*each]
                         << "]" << endl;
                }
                break;
            case 'S':
                for (each = suspect.begin(); each != suspect.end(); each++) {
                    cout << "SUSPECT > " << *each << endl;
                }
                break;
            case 'E':
                cout << "Controller Shutdown!" << endl;
                server_status = false;
                accept_t.join();
                report_t.join();
                FLAG = false;
                break;
            default:
                cout << "Invalid Intruction!" << endl;
        }
    } while (FLAG);

    // 清理垃圾
    delete listenSocket;

    CloseHandle(QMutex);
    return 0;
}


void socketAccept() {
    bool flag;
    cout << "Accepting..." << endl;
    _thread *t = NULL;
    while (server_status) {
        flag = listenSocket->check_connect_(1000);
        if (flag) {
            if (controller_debug) {
                cout << "Somebody connected." << endl;
            }

            WaitForSingleObject(QMutex, INFINITE);
            clientSocket.push(listenSocket->accept_());
            ReleaseMutex(QMutex);
            t = new _thread((int (*)()) messageHandle);
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


void messageHandle() {
    WaitForSingleObject(QMutex, INFINITE);
    _socket *clientS = clientSocket.front();
    clientSocket.pop();
    ReleaseMutex(QMutex);

    char *message = clientS->recv_();
    if (controller_debug) {
        cout << "From " << clientS->getIPAddr() << " : " << message << endl;
    }
    char msg_type = message[0];
    // 判斷收到訊息 s 類型
    // T for time, R for Report, S for Sign up
    if (msg_type == 'T') {

        // get virtual time from console
        char *datetime = new char[17]{0};
        strncpy(datetime, &message[1], 14);
        strncpy(datetime, (char *) calculate_time(datetime).str().c_str(), 16);
        for (short i = 0; i < host.size(); i++) {
            if (disconnect[host[i]] < 10) {
                _socket sConnect(LOCAL_IP_ADDRESS, host[i], 1024);
                if (sConnect.send_(datetime) == -1) {
                    disconnect[host[i]]++;
                }
                sConnect.shutdown_(_socket::BOTH);
                sConnect.close_();
            }
        }

        delete[] datetime;

    } else if (msg_type == 'R') {
        // get report from Crawler and Sensor
        char *suspect_ip = (char *) calloc(22, sizeof(char));
        memcpy(suspect_ip, &message[1], strlen(message) - 1);
        // append to suspect vector
        suspect.push_back(suspect_ip);

    } else if (msg_type == 'S') {
        // host Register
        char *host_port = (char *) calloc(6, sizeof(char));
        memcpy(host_port, &message[1], strlen(message) - 1);
        host.push_back(host_port);
        disconnect[host_port] = 0;
        clientS->send_((char *) "OK");
        /*
        if ( host.empty() || host.end() != find(host.begin(), host.end(), host_port)) {
            host.push_back(host_port);
            disconnect[host_port] = 0;
            clientS->send_((char *) "OK");
        }
        */

    } else {
        cout << "[Message] receive other type of message" << endl;
    }
    clientS->shutdown_(_socket::BOTH);
    clientS->close_();
    delete clientS;

}


void report() {
    while (server_status) {
        if (!suspect.empty()) {
            // report

            _socket s(CONSOLE_IP_ADDRESS, (char *) "6666", 1024);

            int send_q = 0;
            send_q = suspect.size() <= 40 ? suspect.size() : 40;
            string msg("R" + std::to_string(send_q));

            for (send_q; send_q > 0; send_q--) {
                msg += "#" + string(suspect.front());
                char *free_suspect = suspect.front();
                suspect.erase(suspect.begin());
                free(free_suspect);
            }

            char *cstr = new char[msg.length() + 1];
            strcpy(cstr, msg.c_str());
            s.send_(cstr);
            s.shutdown_(_socket::BOTH);
            s.close_();

        }
        Sleep(REPORT_INTERVALS * 1000);
    }
}

void revive() {
    while (server_status) {
        if (!host.empty()) {
            // report
            map<char *, short>::iterator it;
            for (it = disconnect.begin(); it != disconnect.end(); it++) {
                if (it->second >= 10) {
                    _socket test_socket(LOCAL_IP_ADDRESS, it->first, 1024);
                    if (test_socket.send_((char *) "TEST") != -1) {
                        it->second = 0;
                    }
                    test_socket.shutdown_(_socket::BOTH);
                    test_socket.close_();
                }
            }
        }
        Sleep(REVIVE_INTERVALS * 1000);
    }
}

std::ostringstream calculate_time(char *arg_time) {
    char year[5] = {0};
    char month[3] = {0};
    char day[3] = {0};
    char hour[3] = {0};
    char min[3] = {0};
    char sec[3] = {0};
    memcpy(year, &arg_time[0], 4);
    memcpy(month, &arg_time[4], 2);
    memcpy(day, &arg_time[6], 2);
    memcpy(hour, &arg_time[8], 2);
    memcpy(min, &arg_time[10], 2);
    memcpy(sec, &arg_time[12], 2);
    int datetime[6] = {std::atoi(year), std::atoi(month), std::atoi(day), std::atoi(hour), std::atoi(min),
                       std::atoi(sec)};

    bool flag = false;
    short day31[7] = {1, 3, 5, 7, 8, 10, 12};
    short result = 0;

    if (datetime[1] == 2) {
        if ((datetime[0] % 4 == 0 && datetime[0] % 100 != 0) || (datetime[0] % 400 == 0 && datetime[0] % 3200 != 0)) {
            result = 29;
        } else {
            result = 28;
        }
    } else {
        for (short i = 0; i < 7; i++) {
            if (datetime[1] == day31[i]) {
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

    datetime[3] += LOCAL_TIMEZONE;

    int temp;

    if (datetime[3] > 23) {
        temp = datetime[3];
        datetime[3] %= 24;
        datetime[2] += (temp - datetime[3]) / 24;
    }
    // Day
    if (datetime[2] > result) {
        temp = datetime[2];
        datetime[2] %= result;
        datetime[1] += (temp - datetime[2]) / result;
    }
    // Month (Careful: No Defensive)
    if (datetime[1] > 12) {
        datetime[1] -= 12;
        datetime[0]++;
    }
    sprintf(year, "%d", datetime[0]);
    sprintf(month, "%d", datetime[1]);
    sprintf(day, "%d", datetime[2]);
    sprintf(hour, "%d", datetime[3]);

    std::ostringstream timezone_datetime;

    timezone_datetime << "T:" << year << setfill('0') << setw(2) << month << setfill('0') << setw(2) << day
                      << setfill('0')
                      << setw(2) << hour << setfill('0') << setw(2) << min << setfill('0') << setw(2) << sec;

    return timezone_datetime;

}