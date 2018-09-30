#include <iostream>
#include <ctime>
#include <queue>
#include "_socketserver.h"
#include "_thread.h"
#include <mutex>
#include <tchar.h>
using namespace std;

void register_console();
bool register_controller();
void accept_thread();
void client_thread();
void bot();
void crawler();
void sensor();
void send_mes(char );
int handle_send_mes(char ,char *);
char handle_recv_mes(char []);
void time_to_int(char [], int []);
void update_time(int []);
bool compare_with_time(int [], int []);
int check_exit();
int check_time();
typedef struct peerlist {
    char IP[16];
    char Port[5];
    char ddos_time[15];
}PeerList;
vector<PeerList> vec_peerlist;      // peerlist清單
vector<PeerList> vec_ddoslist;
vector<PeerList> vec_sensorlist;

HANDLE QMutex;                      // Lock
mutex change_mtx;
queue<_socket *> ClientQ;            // Collect parameters for client_thread
vector<_thread *> ClientT;          // Collect _thread

_socketserver *s;
#define IDLE 5000
bool server_status = false;
bool debug_mode = false;
char my_ip[15] = "127.0.0.1", my_port[15] = "8000";
char c_ip[15] = "127.0.0.1", c_port[15] = "6666";
char* Concole_IP =  c_ip, * Concole_Port = c_port;
char* MY_IP = my_ip, *MY_PORT = my_port;
char time_stamp[15] = "20180823115900";
const int MAXPLNum = 15 ;
int register_flag = -1; // Bot = 1, Crawler = 2, Sensor = 3
int delay = 0;
int time_rate = 2000;
int main(int argc, char* argv[]) {
    change_mtx.lock();

    strcpy(my_ip,argv[0]);
    strcpy(my_port,argv[1]);
    strcpy(c_ip,argv[2]);
    strcpy(c_port,argv[3]);
    if(strcmp(argv[4],"Y") == 0){
        if(debug_mode)
            printf("[OPEN Debug Mode]\n");
        debug_mode = true;
        _socket::Debug = true;
    }
    else
        debug_mode = false;


    if(debug_mode){
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
    if(!s->get_status()) // 檢查port是否可用
        return -1;

    _thread accept_t((int (*)()) accept_thread);    // accept
    accept_t.start();
    _thread exit_t((int (*)()) check_exit);       // check_exit
    exit_t.start();
    _thread time_t((int (*)()) check_time);       // check_exit
    time_t.start();

    register_console();         // 與Console註冊
    change_mtx.lock(); // 鎖住，等待 Change 指令

    if(register_flag == 1){
        _thread bot_t((int (*)()) bot);
        bot_t.start();
    }
    else if(register_flag == 2){
        _thread  crawler_t((int (*)())  crawler);
        crawler_t.start();
    }
    else if(register_flag == 3){
        _thread sensor_t((int (*)()) sensor);
        sensor_t.start();
    }

    // Controller註冊
    bool controller_flag = false;
    int fail_reg_count = 0;
    while( (!controller_flag) && server_status){
        controller_flag = register_controller();
        if( !controller_flag){
            fail_reg_count++;
        }
        Sleep(1000);
    }

    int inst = 0;
    int num = 0;
    if(debug_mode){
        printf("\n----------------------------------\n");
        printf("-----Input '1'  :Peerlist Size----\n");
        printf("-----Input '3'  :Peerlist IP------\n");
        printf("-----Input '4'  :Show Time------\n");
        printf("-----Input '5'  :Show DDOS------\n");
        printf("-----Input '-1' :Exit-------------\n");
        printf("----------------------------------\n");
        
        while(server_status){
            scanf("%d",&inst);
            if(inst == 1)
                cout << my_ip <<":"<< my_port << "-->" <<"Peerlise_Size :" << vec_peerlist.size() << endl;
            else if(inst == 2){
                scanf("%d",&num);
                printf("[%s:%s]\n",vec_peerlist[num].IP, vec_peerlist[num].Port);
            }
            else if(inst == 3){
                int len = vec_peerlist.size();
                for(int i=0;i < len;i++)
                    printf("[%s:%s]\n",vec_peerlist[i].IP, vec_peerlist[i].Port);
            }
            else if(inst == 4)
                printf("[Time] :%s\n",time_stamp);
            else if(inst == 5){
                int len = vec_ddoslist.size();
                for(int i=0;i < len;i++)
                    printf("[DDOS Queue No.%d] %s:%s  [Time] %s\n",i+1,vec_ddoslist[i].IP,vec_ddoslist[i].Port,vec_ddoslist[i].ddos_time );
            }

            else if(inst == -1)
                break;
        }
    }
    while(server_status){
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
void register_console(){

    char *send_data = new char[1024];
    char *rec_data = new char[1024];
    const char *p = ":";
    char *buf={};

    // Console
    _socket c( Concole_IP,  Concole_Port, 1024); // socket Console
    if(c.get_status()){
        memset(rec_data,'\0',1024);
        sprintf(send_data,"HOST%s:%s",my_ip,my_port);
        if( c.send_(send_data) > 0){
            if(debug_mode)
                cout <<"Send to Console :"<< send_data << endl;
            rec_data = c.recv_();   // 等待Console回覆
            if(rec_data){
                if(debug_mode)
                    cout <<"[Receive] Console :"<<  rec_data << endl << endl;
                buf = strtok(rec_data,p);
                if(strcmp(buf,"Change") == 0 ){
                    buf=strtok(NULL,p);
                    if(strcmp(buf,"Bot") == 0 ){
                        register_flag = 1;
                        buf=strtok(NULL,p);
                        if(strcmp(buf,"1") == 0 )
                            delay = 1;
                    }
                    else if(strcmp(buf,"Crawler") == 0 )
                        register_flag = 2;
                    else if(strcmp(buf,"Sensor") == 0 )
                        register_flag = 3;

                    if( (register_flag == 1)|| (register_flag == 2) ){  // Bot & Crawler 要求 Peerlist
                        c.send_((char *) "Request:Peerlist");
                        memset(rec_data,'\0',1024);
                        rec_data = c.recv_();
                        if(rec_data){
                            if(register_flag == 1){            // Bot
                                c.send_((char *)"EXIT" );
                                handle_recv_mes(rec_data);
                                change_mtx.unlock();
                            }
                            else if(register_flag == 2){        //  Crawler 要求 Sensorlist
                                c.send_((char *)"Request:Sensorlist" );
                                memset(rec_data,'\0',1024);
                                rec_data = c.recv_();
                                if(rec_data){
                                    c.send_((char *)"EXIT" );
                                    handle_recv_mes(rec_data);
                                    change_mtx.unlock();
                                }
                                else{
                                    if(debug_mode)
                                        printf("[INFO] Request Sensorlist failed\n");
                                }
                            }
                        }
                        else{
                            if(debug_mode)
                                printf("[INFO] Request Peerlist failed\n");
                            register_flag = -1;
                        }
                    }
                    else if(register_flag == 3){            // Sensor
                        change_mtx.unlock();
                    }
                }
                else if(strcmp(buf,"OK") == 0 ){
                    register_flag = -1;
                }
            }
            else{
                if(debug_mode)
                    printf("[INFO] Register Console failed\n");
                register_flag = -1;
            }
        }
    }
    else{
        if(debug_mode)
            printf("[INFO] Register Console failed\n");
        register_flag = -1;
    }
    c.close_();


    delete[] send_data;
    delete[] rec_data;
}
bool register_controller(){
    char *send_data = new char[1024];
    char *rec_data = new char[1024];
    bool flag = false;
    // Controller
    _socket ctrl((char *) "127.0.0.1", (char *)"1999", 1024); // socket Controller
    if(ctrl.get_status()){
        sprintf(send_data,"S%s",my_port);
        if( ctrl.send_(send_data) > 0){
            if(debug_mode)
                cout <<"Send to Controller :"<< send_data << endl;
            if(ctrl.check_recv_(IDLE)) {   // Check recv
                rec_data = ctrl.recv_();
                if(rec_data){
                    if(strcmp(rec_data,"OK") == 0 )
                        flag = true;
                    if(debug_mode)
                        printf("[Receive] Controller :OK\n");
                }
                else{
                    if(debug_mode)
                        printf("[INFO] Register Controller failed\n");
                }
            }
            else{
                if(debug_mode)
                    printf("[INFO] Register Controller failed\n");
            }
        }
    }
    else{
        if(debug_mode)
            printf("[INFO] Register Controller failed\n");
    }
    ctrl.close_();
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
    if(debug_mode)
        cout << "Stop accepting..." << endl;
}
void client_thread() {
    srand( (unsigned)time(NULL) );
    WaitForSingleObject(QMutex, INFINITE);
    _socket *client = ClientQ.front();
    ClientQ.pop();
    ReleaseMutex(QMutex);
    char *rec_data = new char[1024];
    char rec_flag;
    char send_data[1024]={};
    memset(send_data,'\0',1024);
    rec_data = client->recv_();
    if(rec_data){
        rec_flag = handle_recv_mes(rec_data);
        if( (rec_flag != 'T') && (delay == 1)){
                int temp = rand()%5000;
                Sleep(temp);
        }
        if( (rec_flag ==  'E') && (register_flag!=3)){
            handle_send_mes(rec_flag,send_data);
            client->send_(send_data);
        }
        else if(rec_flag == 'D'){
            send_mes('D');
        }
        else if( (rec_flag == 'B')||(rec_flag == 'C') ){
            client->send_((char *)"Request:Peerlist" );
            memset(rec_data,'\0',1024);
            rec_data = client->recv_();
            if(rec_data){
                handle_recv_mes(rec_data);
                if(rec_flag == 'B'){
                    client->send_((char *)"EXIT" );
                    register_flag = 1;
                    change_mtx.unlock();
                }
                else if (rec_flag == 'C'){
                    client->send_((char *)"Request:Sensorlist" );
                    memset(rec_data,'\0',1024);
                    rec_data = client->recv_();
                    if(rec_data){
                        register_flag = 2;
                        handle_recv_mes(rec_data);
                        client->send_((char *)"EXIT" );
                        change_mtx.unlock();
                    }
                }
            }
            else{
                if(debug_mode)
                    printf("[INFO] Request Peerlist failed\n");
                register_flag = -1;
            }
        }
        else if(rec_flag == 'S'){
            register_flag = 3;
            change_mtx.unlock();
        }
    }

    client->shutdown_(_socket::BOTH);
    client->close_();
    delete client;
    delete[] rec_data;
}

void bot(){
    srand( (unsigned)time(NULL) );
    if(debug_mode)
        printf("-------Bot--------\n");
    Sleep(3000);
    while(server_status){
        if(vec_peerlist.size() > 1){
            send_mes('E');
            //send_mes('D');
        }
        if(vec_ddoslist.size() > 0)
            if( strcmp(time_stamp,vec_ddoslist[0].ddos_time) >= 0){
                if(debug_mode)
                    printf("[DDOS]--->%s:%s  [Time] %s\n",vec_ddoslist[0].IP,vec_ddoslist[0].Port,vec_ddoslist[0].ddos_time );
                vec_ddoslist.erase(vec_ddoslist.begin());
        }
        Sleep(4000);
    }

};
void crawler(){
    srand( (unsigned)time(NULL) );
    if(debug_mode)
        printf("-------Crawler--------\n");
    Sleep(3000);
    while(server_status){
        if( (vec_peerlist.size() > 0) && (vec_sensorlist.size() > 0) ){
            send_mes('E');
        }
        Sleep(4000);
    }
}
void sensor(){
    if(debug_mode)
        printf("-------Sensor--------\n");
    while(server_status){
        if(vec_peerlist.size() > 0){
            send_mes('R');
        }
        Sleep(1000);
    }
}
void send_mes(char flag){
    int send_num = -1;
    char *rec_data = new char[1024];
    char *send_data = new char[1024];

    send_num = handle_send_mes(flag,send_data );
    if(flag == 'E'){        // 交換 Peerlist
        _socket c( (char*)vec_peerlist[send_num].IP,  (char*)vec_peerlist[send_num].Port, 1024);
        if(c.get_status()){
            c.send_(send_data );
            if(debug_mode)
                cout << "[Send]--->"<<vec_peerlist[send_num].IP << ":" << vec_peerlist[send_num].Port <<":[" <<send_data << "]"<<endl;
            if(c.check_recv_(IDLE)) {   // Check recv
                rec_data = c.recv_();
                if(rec_data)
                    handle_recv_mes(rec_data);
            }
            else{
                if(debug_mode)
                    printf("Recv Time out\n");
            }
        }
        c.close_();
    }
    else if(flag =='D'){    // DDOS
        for(int i=0 ;i < vec_peerlist.size(); i++){
            _socket c( vec_peerlist[i].IP,  vec_peerlist[i].Port, 1024); // socket
            if(c.get_status() )
                c.send_(send_data );
            c.close_();
            Sleep(500);
        }
    }
    else if(flag =='R'){    // 回報抓到的 Bot
        _socket c( (char *)"127.0.0.1", (char *) "1999", 1024);
        if(c.get_status()){
            c.send_(send_data );
            vec_peerlist.erase(vec_peerlist.begin() );
            if(debug_mode)
                cout << "[Report] "<< "Controller " <<"--->:[" << send_data << "]"<<endl;
        }
        c.close_();
    }
    delete[] rec_data;
    delete[] send_data;
}
int handle_send_mes(char flag, char *send_buf){
    //  格式[ 發送時間:到期時間:指令:目標IP:目標Port:發送端IP:發送端Port ]
    memset(send_buf,'\0',1024);
    int send_num = -1,time_t[6]={};
    char time_expire[15]={}, instr[20];

    time_to_int(time_stamp,time_t); //   [0]年 [1]月 [2]日 [3]時 [4]分 [5]秒
    time_t[2] = time_t[2] + 3; // 訊息三天過期
    sprintf(time_expire,"%04d%02d%02d%02d%02d%02d",time_t[0],time_t[1],time_t[2],time_t[3],time_t[4],time_t[5]);

    send_num = rand()%vec_peerlist.size();

    int tar_num = -1;
    if (flag == 'E'){
        strcpy(instr,"Exchange_peerlist");
        do{
            tar_num = rand()%vec_peerlist.size();
        }while((send_num == tar_num) || tar_num < 0);
        if(register_flag == 1) // Bot
            sprintf(send_buf,"%s:%s:%s:%s:%s:%s:%s",time_stamp ,time_expire ,instr ,vec_peerlist[tar_num].IP ,vec_peerlist[tar_num].Port, my_ip, my_port);
        if(register_flag == 2){// Crawler
            int  sensor_num =  rand()%vec_sensorlist.size();
            sprintf(send_buf,"%s:%s:%s:%s:%s:%s:%s",time_stamp ,time_expire ,instr ,vec_sensorlist[sensor_num].IP ,vec_sensorlist[sensor_num].Port, my_ip, my_port);
        }

    }
    else if(flag == 'D'){
        strcpy(instr,"DDOS");
        tar_num = vec_ddoslist.size()-1;
        sprintf(send_buf,"%s:%s:%s:%s:%s:%s:%s",time_stamp ,vec_ddoslist[tar_num].ddos_time ,instr ,vec_ddoslist[tar_num].IP ,vec_ddoslist[tar_num].Port, my_ip, my_port);
    }
    else if(flag == 'R'){ // Report
        sprintf(send_buf,"R%s:%s",vec_peerlist[0].IP, vec_peerlist[0].Port);
    }
    else
        return -1;
    return send_num;
}
char handle_recv_mes(char data[]) {
    bool ExPL_flag = false, DDOS_flag = false, update_time_flag = false;
    int count = 0;
    char *buf={}, tar_ip[16]={}, tar_port[5]={}, time_expire[15]={}, client_ip[16]={},client_port[5]={};
    char rtn_flag = 'N';
    char output[1024];
    const char *p = ":";
    PeerList p1;
    strcpy(output,data);
    buf = strtok(data,p);
    memset(tar_ip,'\0',16);
    memset(tar_port,'\0',5);
    while(buf){
        switch(count){
            case 0 :
                if(strcmp(buf,"Kill") == 0 ){       // 關閉程式
                    server_status = false;
                    change_mtx.unlock();
                    return 'N';
                }
                else if(strcmp(buf,"C") == 0 ){     // 更改時間頻率
                    buf=strtok(NULL,p);
                    time_rate = atoi(buf);
                    if(debug_mode)
                        printf("[Update Time Rate] :%d\n",time_rate);
                    return 'N';
                }
                else if(strcmp(buf,"T") == 0 )
                    update_time_flag = true;
                else if(strcmp(buf,"Change") == 0){
                    buf=strtok(NULL,p);
                    if(strcmp(buf,"Bot") == 0 ){
                        register_flag = 1;
                        rtn_flag = 'B';
                        buf=strtok(NULL,p);
                        if(strcmp(buf,"1") == 0 ){
                            if(debug_mode)
                                printf("[Delay]\n");
                            delay = 1;
                        }
                    }
                    else if(strcmp(buf,"Crawler") == 0 )
                        rtn_flag = 'C';
                    else if(strcmp(buf,"Sensor") == 0 )
                        rtn_flag = 'S';
                }
                else if(strcmp(buf,"Peerlist") == 0 ){
                    buf=strtok(NULL,p);
                    count = 0;
                    while(buf){
                        count = count%2;
                        if(count == 0){
                            memset(p1.IP,'\0',16);
                            strcpy(p1.IP,buf);
                        }
                        else{
                            memset(p1.Port,'\0',5);
                            strcpy(p1.Port,buf);
                            bool flag = true;
                            for(int i=0 ;i < vec_peerlist.size(); i++){ // 檢查 對方傳的IP,Port有無重複
                                if(strcmp(vec_peerlist[i].IP,p1.IP) == 0)
                                    if(strcmp(vec_peerlist[i].Port,p1.Port) == 0){
                                        flag = false;
                                        break;
                                    }
                            }
                            if(flag)
                                vec_peerlist.push_back(p1);
                        }
                        count++;
                        buf=strtok(NULL,p);
                    }
                }
                else if((strcmp(buf,"Sensorlist") == 0) ){
                    buf=strtok(NULL,p);
                    count = 0;;
                    while(buf){
                        count = count%2;
                        if(count == 0)
                            strcpy(p1.IP,buf);
                        else{
                            strcpy(p1.Port,buf);
                            vec_sensorlist.push_back(p1);
                        }
                        count++;
                        buf=strtok(NULL,p);
                    }
                }
                break;
            case 1 :
                if(update_time_flag){
                    strcpy(time_stamp,buf); // 更新時間
                  // if(debug_mode)
                       // printf("[Update Time] :%s\n",time_stamp);
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
                if(strcmp(buf,"Exchange_peerlist") == 0 ){
                    if(register_flag == -1)
                        return 'N';
                    ExPL_flag = true;
                }
                else if(strcmp(buf,"DDOS") == 0){
                    if(register_flag == -1)
                        return 'N';
                    DDOS_flag = true;
                }
                break;
            case 3 :
                strcpy(p1.IP,buf);
                break;
            case 4 :
                strcpy(p1.Port,buf);
                if(ExPL_flag){
                    for(int i=0 ;i < vec_peerlist.size(); i++){ // 檢查 對方傳的IP,Port有無重複
                        if(strcmp(vec_peerlist[i].IP,p1.IP) == 0)
                            if(strcmp(vec_peerlist[i].Port,p1.Port) == 0){
                                rtn_flag = 'E';
                                break;
                            }
                    }
                    if(rtn_flag != 'E'){
                        vec_peerlist.push_back(p1);
                        if ( (vec_peerlist.size() > MAXPLNum)  ){    // Bot PeerList太多要刪掉
                            int rand_num = rand()%vec_peerlist.size();
                            vec_peerlist.erase(vec_peerlist.begin() + rand_num);
                        }
                        rtn_flag = 'E';
                    }
                }
                else if (DDOS_flag){
                    for(int i=0 ;i < vec_ddoslist.size(); i++){ // 檢查 對方傳的IP,Port有無重複
                        if(strcmp(vec_ddoslist[i].IP,p1.IP) == 0)
                            if(strcmp(vec_ddoslist[i].Port,p1.Port) == 0)
                                return 'N'; // IP重複
                    }
                    strcpy(p1.ddos_time,time_expire);
                    // 還要處理時間進位
                    vec_ddoslist.push_back(p1);
                    if(debug_mode)
                        cout << "[Add_DDOS]--->" << p1.IP << ":" << p1.Port << ":" << time_expire << endl;
                    rtn_flag = 'D';
                }
                break;
            case 5 :
                strcpy(client_ip,buf);
                break;
            case 6 :
                strcpy(client_port,buf);
                break;
        }
        buf=strtok(NULL,p);
        count++;
    }
    if(debug_mode){
        printf("[Receive] %s:%s--->[%s]\n",client_ip,client_port,output);
    }

    return rtn_flag;
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
int check_exit(){

    HANDLE hFileMap;
    BOOL bResult;
    PCHAR lpBuffer = nullptr;

    hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _T("Bot_Kill"));

    if (!hFileMap) {
        if(debug_mode)
            printf("[Error] OpenFileMapping Failed: %d.\n", GetLastError());
        exit(-1);
    } else {
        if(debug_mode)
            printf("[Check Exit] Success!\n");
    }
    while(true){
        lpBuffer = (PCHAR) MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 256);

        if (lpBuffer == nullptr) {
            if(debug_mode)
                printf("[Error] MapViewOfFile Failed: %d.\n", GetLastError());
            exit(-1);
        }
        if(strcmp(lpBuffer,"Kill")==0){
            if(debug_mode)
                printf("\n[ EXIT ]\n");

            bResult = UnmapViewOfFile(lpBuffer);
            if (!bResult) {
                if(debug_mode)
                    printf("UnmapViewOfFile Failed Error: %d.\n", GetLastError());
                exit(-1);
            }
            else {
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
int check_time(){

    HANDLE hFileMap;
    BOOL bResult;
    PCHAR lpBuffer = nullptr;
    const char *p = ":";
    char *buf={};
    char data[20] = {};
    hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _T("BotNetShareDateTime"));

    if (!hFileMap) {
        if(debug_mode)
            printf("[Error] OpenFileMapping Failed: %d.\n", GetLastError());
        //exit(-1);
        return -1;
    } else {
        if(debug_mode)
            printf("[Check Time] Success!\n");
    }
    while(true){
        lpBuffer = (PCHAR) MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 256);

        if (lpBuffer == nullptr) {
            if(debug_mode)
                printf("[Error] MapViewOfFile Failed: %d.\n", GetLastError());
            exit(-1);
        }
        //if(debug_mode)
            //printf("%s\n",lpBuffer);
        strcpy(data,lpBuffer);
        buf = strtok(data,p);
        if(strcmp(buf,"T")==0){
            buf = strtok(NULL,p);
            strcpy(time_stamp,buf);
        }
        Sleep(500);
    }
}