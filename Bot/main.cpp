#include <iostream>
#include <ctime>
#include <queue>
#include "_socketserver.h"
#include "_thread.h"
#include <mutex>
using namespace std;

void register_ip();
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
bool server_status = false;
bool debug_mode = false;
char my_ip[15] = "127.0.0.1", my_port[15] = "8000";
char c_ip[15] = "127.0.0.1", c_port[15] = "6666";
char* Concole_IP =  c_ip, * Concole_Port = c_port;
char time_stamp[15] = "20180823115900";
const int MAXPLNum = 15 ;
int register_flag = -1; // Bot = 1, Crawler = 2, Sensor = 3

int main(int argc, char* argv[]) {
    change_mtx.lock();

    strcpy(my_ip,argv[0]);
    strcpy(my_port,argv[1]);
    strcpy(c_ip,argv[2]);
    strcpy(c_port,argv[3]);
    if(strcmp(argv[4],"Y") == 0){
        printf("[OPEN Debug Mode]\n");
        debug_mode = true;
    }
    else{
        printf("[Colse Debug Mode]\n");
        debug_mode = false;
    }

    cout << "Host :" << my_ip << ":" << my_port << endl;
    cout << "Console :" << c_ip << ":" << c_port << endl << endl;
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
    /*
    // Test data
    PeerList p1 = {"127.0.0.1","6666"};
    PeerList p2 = {"127.0.0.1","8888"};
    PeerList p3 = {"192.1.1.168","3500"};
    PeerList p4 = {"198.1.1.100","2020"};
    vec_peerlist.push_back(p1);
    vec_peerlist.push_back(p2);
    vec_peerlist.push_back(p3);
    vec_peerlist.push_back(p4);
    // Test data --
    */


    // 建立Server
    server_status = true;
    s = new _socketserver((char *) my_port, 1024);
    if(!s->get_status()) // 檢查port是否可用
        return -1;

    _thread accept_t((int (*)()) accept_thread);
    accept_t.start();

    register_ip();   // 與Console註冊

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

    int inst = 0;
    int num = 0;
    printf("\n----------------------------------\n");
    printf("-----Input '1'  :Peerlist Size----\n");
    printf("-----Input '3'  :Peerlist IP------\n");
    printf("-----Input '-1' :Exit-------------\n");
    printf("----------------------------------\n");
    while(1){

        scanf("%d",&inst);
        if(inst == 1)
            cout << my_ip <<":"<< my_port << "-->" <<"Peerlise_Size :" << vec_peerlist.size() << endl;
        else if(inst == 2){
            scanf("%d",&num);
            cout << vec_peerlist[num].IP << ":" << vec_peerlist[num].Port << endl << endl;
        }
        else if(inst == 3){
            int len = vec_peerlist.size();
            for(int i=0;i < len;i++)
                cout << vec_peerlist[i].IP << ":" << vec_peerlist[i].Port <<endl;
        }
        else if(inst == -1)
            break;
    }

    getchar();
    server_status = false;
    accept_t.join();
    // 清理垃圾
    delete (s);
    _socket::wsacleanup_();
    CloseHandle(QMutex);
    system("pause");
    return 0;
}
void register_ip(){

    char *send_data = new char[1024];
    char *rec_data = new char[1024];
    const char *p = ":";
    char *buf={};

    // Console
    memset(rec_data,'\0',1024);
    sprintf(send_data,"HOST%s:%s",my_ip,my_port);

    _socket c( Concole_IP,  Concole_Port, 1024); // socket Console
    if( c.send_(send_data) > 0)
        if(debug_mode)
            cout <<"Send to Console :"<< send_data << endl;

    rec_data = c.recv_();   // 等待Console回覆
    if(rec_data){
        if(debug_mode)
            cout <<"[Receive] Console :"<<  rec_data << endl << endl << endl;
        buf = strtok(rec_data,p);
        if(strcmp(buf,"Change") == 0 ){
            buf=strtok(NULL,p);
            if(strcmp(buf,"Bot") == 0 )
                register_flag = 1;
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
    c.shutdown_(_socket::BOTH);
    c.close_();

    // Controller
    sprintf(send_data,"S%s",my_port);
    _socket ctrl( my_ip, (char *)"1999", 1024); // socket Controller
    if( ctrl.send_(send_data) > 0)
        if(debug_mode)
            cout <<"Send to Controller :"<< send_data << endl;
    rec_data = ctrl.recv_();
    if(rec_data){
        if(strcmp(rec_data,"OK") == 0 )
            if(debug_mode)
                printf("[Receive] Controller :OK\n");
    }
    else{
        if(debug_mode)
            printf("[INFO] Register Controller failed\n\n");
    }
    ctrl.shutdown_(_socket::BOTH);
    ctrl.close_();

    register_flag = 1; // Bot
    delete[] send_data;
    delete[] rec_data;
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
            t = new _thread((int (*)()) client_thread);
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
        //cout << "From " << client->getIPAddr() << " : " << rec_data << endl;
        rec_flag = handle_recv_mes(rec_data);
        if( (rec_flag ==  'E') && (vec_peerlist.size() > 1) ){
            if(register_flag == 3){ // Sensor 回報
                send_mes('R');
            }
            else{
                handle_send_mes(rec_flag,send_data);
                client->send_(send_data);
            }
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
                        handle_recv_mes(rec_data);
                        client->send_((char *)"EXIT" );
                        register_flag = 2;
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

    //client->shutdown_(_socket::BOTH);
    client->close_();
    delete client;
    delete[] rec_data;
}

void bot(){
    srand( (unsigned)time(NULL) );
    printf("-------Bot--------\n");
    while(server_status){
        if(vec_peerlist.size() > 1){
            send_mes('E');
            //send_mes('D');
        }
        if(vec_ddoslist.size() > 0)
            if( strcmp(time_stamp,vec_ddoslist[0].ddos_time) <= 0){
                cout << "DDOS:" << vec_ddoslist[0].IP << ":" << vec_ddoslist[0].Port << ":" << time_stamp << endl;
                vec_ddoslist.erase(vec_ddoslist.begin());
        }

        Sleep(4000);
    }

};
void crawler(){
    srand( (unsigned)time(NULL) );
    printf("-------Crawler--------\n");
    while(server_status){
        if( (vec_peerlist.size() > 1) && (vec_sensorlist.size() > 1) ){
            send_mes('E');
        }
        Sleep(4000);
    }
}
void sensor(){
    printf("-------Sensor--------\n");


}
void send_mes(char flag){
    int send_num = -1;
    char *rec_data = new char[1024];
    char *send_data = new char[1024];

    send_num = handle_send_mes(flag,send_data );
    if(flag == 'E'){        // 交換 Peerlist
        _socket c( vec_peerlist[send_num].IP,  vec_peerlist[send_num].Port, 1024);
        c.send_(send_data );
        if(debug_mode)
            cout << "send "<<vec_peerlist[send_num].IP << ":" << vec_peerlist[send_num].Port <<":[" <<send_data << "]"<<endl;
        rec_data = c.recv_();
        if(rec_data){
            handle_recv_mes(rec_data);
        }
        //c.shutdown_(_socket::BOTH);
        c.close_();
    }
    else if(flag =='D'){    // DDOS
        for(int i=0 ;i < vec_peerlist.size(); i++){
            _socket c( vec_peerlist[i].IP,  vec_peerlist[i].Port, 1024); // socket
            c.send_(send_data );

            //c.shutdown_(_socket::BOTH);
            c.close_();
            Sleep(500);
        }
    }
    else if(flag =='R'){    // 回報抓到的 Bot
        _socket c( my_ip, (char *) "1999", 1024);
        c.send_(send_data );

        //c.shutdown_(_socket::BOTH);
        c.close_();
    }
    delete[] rec_data;
    delete[] send_data;
}
int handle_send_mes(char flag, char *send_buf){
    //  格式[ 發送時間:到期時間:指令:目標IP:目標Port:發送端IP:發送端Port ]

    memset(send_buf,'\0',1024);
    int send_num = -1, time_t[6]={};
    char time_expire[15]={}, instr[20];

    time_to_int(time_stamp,time_t); //   [0]年 [1]月 [2]日 [3]時 [4]分 [5]秒
    time_t[2] = time_t[2] + 3; // 訊息三天過期
    sprintf(time_expire,"%04d%02d%02d%02d%02d%02d",time_t[0],time_t[1],time_t[2],time_t[3],time_t[4],time_t[5]);
    if(register_flag == 1)// 選socket的對象
        send_num = rand()%vec_peerlist.size();
    if(register_flag == 2)
        send_num = rand()%vec_sensorlist.size();

    int tar_num = -1;
    if (flag == 'E'){
        strcpy(instr,"Exchange_peerlist");
        do{
            tar_num = rand()%vec_peerlist.size();
        }while((send_num == tar_num) || tar_num < 0);
        if(register_flag == 1) // Bot
            sprintf(send_buf,"%s:%s:%s:%s:%s:%s:%s",time_stamp ,time_expire ,instr ,vec_peerlist[tar_num].IP ,vec_peerlist[tar_num].Port, my_ip, my_port);
        if(register_flag == 2) // Crawler
            sprintf(send_buf,"%s:%s:%s:%s:%s:%s:%s",time_stamp ,time_expire ,instr ,vec_sensorlist[tar_num].IP ,vec_sensorlist[tar_num].Port, my_ip, my_port);
    }
    else if(flag == 'D'){
        strcpy(instr,"DDOS");
        tar_num = vec_ddoslist.size()-1;
        sprintf(send_buf,"%s:%s:%s:%s:%s:%s:%s",time_stamp ,vec_ddoslist[tar_num].ddos_time ,instr ,vec_ddoslist[tar_num].IP ,vec_ddoslist[tar_num].Port, my_ip, my_port);
    }
    else if(flag == 'R'){
        sprintf(send_buf,"S%s:%s",vec_peerlist[0].IP, vec_peerlist[0].Port);
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
    strcpy(output,data);
    buf = strtok(data,p);
    while(buf){
        switch(count){
            case 0 :
                if(strcmp(buf,"T") == 0 )
                    update_time_flag = true;
                else if(strcmp(buf,"Change") == 0){
                    buf=strtok(NULL,p);
                    if(strcmp(buf,"Bot") == 0 )
                        rtn_flag = 'B';
                    else if(strcmp(buf,"Crawler") == 0 )
                        rtn_flag = 'C';
                    else if(strcmp(buf,"Sensor") == 0 )
                        rtn_flag = 'S';
                }
                else if(strcmp(buf,"Peerlist") == 0 ){
                    buf=strtok(NULL,p);
                    count = 0;
                    PeerList p1;
                    while(buf){
                        count = count%2;
                        if(count == 0)
                            strcpy(p1.IP,buf);
                        else{
                            strcpy(p1.Port,buf);
                            vec_peerlist.push_back(p1);
                        }
                        count++;
                        buf=strtok(NULL,p);
                    }
                }
                else if((strcmp(buf,"Sensorlist") == 0) && (register_flag == 2) ){
                    buf=strtok(NULL,p);
                    count = 0;
                    PeerList p1;
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
                    if(debug_mode)
                        printf("[Update Time] :%s\n",time_stamp);
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
                                //cout << "repeat_PL:" << tar_ip << ":" << tar_port << endl;
                                rtn_flag = 'E';
                                break;
                            }
                    }
                    if(rtn_flag != 'E'){
                        PeerList p1;
                        strcpy(p1.IP,tar_ip);
                        strcpy(p1.Port,tar_port);
                        vec_peerlist.push_back(p1);
                        //cout << "add_PL:" << p1.IP << ":" << p1.Port << endl;
                        if ( (vec_peerlist.size() > MAXPLNum)  ){    // Bot PeerList太多要刪掉
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
                    if(debug_mode)
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
                break;
        }
        buf=strtok(NULL,p);
        count++;
    }
    if(debug_mode)
        printf("[Receive] %s:%s--->[%s]\n",client_ip,client_port,output);
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