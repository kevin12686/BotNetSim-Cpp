#include <iostream>
#include <queue>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <random>
#include <chrono>
#include <tchar.h>
#include "_socket.h"
#include "_socketserver.h"

#define LOCAL_PORT 1999
#define REPORT_INTERVALS 5

using namespace std;

typedef struct MYTHREAD {
    HANDLE handle;
    DWORD id;
} MyThread;


void socketAccept();

void messageHandle(LPVOID);

void report();

char *getLocalIP();

ostringstream calculate_time(char *);


vector<char *> suspect;
vector<char *> host;
_socketserver *listenSocket;

bool server_status = false;

char *CONSOLE_IP_ADDRESS = new char[16]{0};
char *LOCAL_IP_ADDRESS;
int LOCAL_TIMEZONE;

string TIMESTAMP;

bool CONTROL_DEBUG = false;

int main(int argc, char *argv[]) {

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    default_random_engine generator(seed);
    uniform_int_distribution<int> dis(-9, 9);
    LOCAL_TIMEZONE = dis(generator);

    MyThread accept_t;
    MyThread report_t;

    /* Get Console IP Address From File */

    fstream fin("Console.txt");
    if (!fin) {
        printf("[Error] Console.txt Not Found!\n");
    } else {
        fin.getline(CONSOLE_IP_ADDRESS, 16, '\n');
    }
    if (!CONSOLE_IP_ADDRESS) {
        printf("[Error] Cannot Get Console IP Address!\nPlease Input Console IP Address:\n");
        cin >> CONSOLE_IP_ADDRESS;
    }

    /* Get Local IP Address From Function */

    size_t local_ip_length = strlen(getLocalIP()) + 1;
    LOCAL_IP_ADDRESS = new char[local_ip_length]{0};
    strcpy_s(LOCAL_IP_ADDRESS, local_ip_length, getLocalIP());

    if (!strcmp(LOCAL_IP_ADDRESS, "Failed")) {
        printf("[Error] Cannot Get Local IP Address!\nPlease Input Local IP Address:\n");
        cin >> LOCAL_IP_ADDRESS;
    }

    printf("[Application] Console IP Address: %s\n", CONSOLE_IP_ADDRESS);
    printf("[Application] Local IP Address: %s\n", LOCAL_IP_ADDRESS);
    printf("[Application] TIMEZONE: %d\n", LOCAL_TIMEZONE);


    if (!OpenClipboard(nullptr)) {
        printf("OpenClipboard Failed!\n");
        exit((int) GetLastError());
    }

    EmptyClipboard();


    WSADATA wsadata;
    _socket::wsastartup_(&wsadata);

    server_status = true;

    _socket socket2Console(CONSOLE_IP_ADDRESS, (char *) "6666", 1024);
    char msg[30] = "CTRL";
    strcat(msg, (const char *) LOCAL_IP_ADDRESS);
    strcat(msg, (const char *) ":");
    strcat(msg, std::to_string(LOCAL_PORT).c_str());
    if (socket2Console.send_(msg) == -1) {
        cout << "[Socket] Send Message To Console Failed" << endl;
        exit(-1);
    }
    socket2Console.close_();


    listenSocket = new _socketserver((char *) "1999", 1024);


    accept_t.handle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE) socketAccept, nullptr, 0, &(accept_t.id));

    report_t.handle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE) report, nullptr, 0, &(report_t.id));

    Sleep(1000);

    cout << "[Application] Controller Start!" << endl;
    cout << "[INSTRUCTION] Input \'L\' To Show All." << endl;
    cout << "[INSTRUCTION] Input \'H\' To Show Hosts." << endl;
    cout << "[INSTRUCTION] Input \'S\' To Show Suspects." << endl;
    cout << "[INSTRUCTION] Input \'T\' To Show Timestamp." << endl;
    cout << "[INSTRUCTION] Input \'E\' To Exit." << endl;

    char operation;
    bool FLAG = true;
    vector<char *>::iterator each;
    do {
        cin >> operation;
        switch (operation) {
            case 'L':
            case 'l':
                cout << "Host Number: " << host.size() << endl;
                cout << "Suspect Number: " << suspect.size() << endl;
                break;
            case 'H':
            case 'h':
                for (each = host.begin(); each != host.end(); each++) {
                    cout << "HOST => " << LOCAL_IP_ADDRESS << ":" << *each << endl;
                }
                break;
            case 'S':
            case 's':
                for (each = suspect.begin(); each != suspect.end(); each++) {
                    cout << "SUSPECT > " << *each << endl;
                }
                break;
            case 'T':
            case 't':
                cout << "[DateTime] " << TIMESTAMP << endl;
                break;
            case 'E':
            case 'e':
                cout << "[Application] Controller Shutdown!" << endl;
                server_status = false;
                FLAG = false;
                break;
            default:
                cout << "[Application] Invalid Instruction!" << endl;
        }

    } while (FLAG);

    WaitForSingleObject(accept_t.handle, INFINITE);
    CloseHandle(accept_t.handle);

    WaitForSingleObject(report_t.handle, INFINITE);
    CloseHandle(report_t.handle);

    delete listenSocket;

    return 0;
}


void socketAccept() {

    printf("[Application] Accepting...\n");

    vector<MyThread *> clientT;
    while (server_status) {
        if (listenSocket->check_connect_(500)) {
            if (CONTROL_DEBUG) {
                cout << "[Socket] Somebody connected." << endl;
            }

            _socket *client = listenSocket->accept_();

            MyThread *temp = new MyThread;
            temp->handle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE) messageHandle, (LPVOID) client, 0,
                                        &(temp->id));
            if (temp->handle) {
                if (CONTROL_DEBUG) {
                    printf("[Thread] Client Accepted.\n");
                }
                clientT.push_back(temp);
            }
        }
    }
    vector<MyThread *>::iterator it_i;
    for (it_i = clientT.begin(); it_i != clientT.end(); it_i++) {
        WaitForSingleObject((*it_i)->handle, INFINITE);
        CloseHandle((*it_i)->handle);
        delete (*it_i);
    }
    printf("[Application] Stop accepting...\n");
}


void messageHandle(LPVOID s) {
    _socket *clientS = (_socket *) s;

    char *message = clientS->recv_();

    if (CONTROL_DEBUG)
        cout << "[Message] From " << clientS->getIPAddr() << " : " << message << endl;

    char msg_type = message[0];

    // T for time, R for Report, S for Sign up
    if (msg_type == 'T') {

        // get virtual time from console
        char *datetime = new char[17]{0};
        strncpy(datetime, &message[1], 14);
        strncpy(datetime, (char *) calculate_time(datetime).str().c_str(), 16);

        const size_t len = strlen(datetime) + 1;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        memcpy(GlobalLock(hMem), datetime, len);
        GlobalUnlock(hMem);
        SetClipboardData(CF_TEXT, hMem);
        TIMESTAMP = datetime;

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
        clientS->send_((char *) "OK");

    }

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
            s.close_();

        }
        Sleep(REPORT_INTERVALS * 1000);
    }
}


ostringstream calculate_time(char *arg_time) {
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


char *getLocalIP() {

    WSADATA wsaData;
    ::WSAStartup(
            MAKEWORD(2, 2),
            &wsaData);


    char szHost[256];
    ::gethostname(szHost, 256);

    hostent *pHost = ::gethostbyname(szHost);
    in_addr addr;

    for (short i = 0;; i++) {

        char *p = pHost->h_addr_list[i];
        if (p == nullptr) {
            break;
        }
        memcpy(&addr.S_un.S_addr, p, pHost->h_length);

        char *strIp = ::inet_ntoa(addr);
        char *temp = new char[20]{0};
        strcpy(temp, strIp);
        if ((temp[0] == '1') & (temp[1] == '9') & (temp[2] == '2')) {

        } else {
            ::WSACleanup();
            return strIp;
        }
    }

    ::WSACleanup();
    return (char *) "Failed";
}