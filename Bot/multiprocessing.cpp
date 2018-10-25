#include <iostream>
#include <vector>
#include<fstream>
#include "_socket.h"
#include <windows.h>
#include <tchar.h>
#include <winsock2.h>
#include <ctime>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

char* get_IP();

int main() {
    srand((unsigned) time(NULL));
    STARTUPINFO info = {sizeof(info)};
    PROCESS_INFORMATION information;
    char console_ip[20] = {}, console_port[10] = "6666";
    char host_ip[20] = {}, host_port[10] = {};
    char ip_port[40] = {}, p[10] = {};
    char debug[3] = {};
    int temp = 0, setting_temp, process_num = 0, debug_int = 0, t = -1, count = 1;
    bool console_flag = false, host_ip_flag = false, setting_flag = false;
    char line[20];

    // Shared Memory
    HANDLE hFileMap;
    BOOL bResult;
    PCHAR lpBuffer = nullptr;
    char Buffer[50];

    hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 256, _T("Bot_Kill"));
    if (!hFileMap) {
        printf("CreateFileMapping Failed Error: %d.\n", GetLastError());
        exit(-1);
    } else {
        //printf("[CreateFileMapping Success!]\n");
    }

    lpBuffer = (PCHAR) MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 256);
    if (lpBuffer == nullptr) {
        printf("MapViewOfFile Failed Error: %d.\n", GetLastError());
        exit(-1);
    } else {
        //printf("[MapViewOfFile Success!]\n");
    }

    // OPEN Console.txt
    fstream fin;
    fin.open("Console.txt",ios::in);
    if(!fin){
        printf("Open Console Failed\n");
        return -1;
    }
    else{
        while(fin.getline(console_ip,sizeof(console_ip),'\n')){
            break;
        }
    }
    fin.close();
    printf("[Console IP: %s]\n",console_ip);

    // Get Host IP
    strcpy(host_ip,get_IP());
    if(strcmp(host_ip,"Failed")!=0){
        host_ip_flag = true;
        printf("[Host IP:%s]\n\n",host_ip);
    }
    else{
        printf("[Error] Get Host IP Failed\n");
    }

    // OPEN Setting.txt
    fstream setting;
    setting.open("Setting.txt",ios::in);
    if(!setting){
        printf("[INFO] Setting Failed\n");
    }
    else{
        printf("[INFO] Setting Successful\n");
        setting_flag = true;
        while(setting>>setting_temp){
            if(count == 1)
                t = setting_temp;
            else if(count == 2)
                debug_int = setting_temp;
            else if(count == 3)
                process_num = setting_temp;
            else if(count == 4){
                sprintf(host_port,"%d",setting_temp);
            }
            count++;
        }
    }
    setting.close();


    // Input Setting
    printf("Input '1' : Create New Console\nInput '0' : No Operation\n");
    if(setting_flag)
        printf("%d\n",t);
    else
        scanf("%d",&t);
    if(t == 1)
        console_flag = true;
    else
        console_flag = false;
    printf("Input '1' : OPEN Debug mode \nInput '0' : No Operation\n");
    if(setting_flag)
        printf("%d\n",debug_int);
    else
        scanf("%d",&debug_int);
    if(debug_int == 1)
        strcpy(debug,"Y");
    else
        strcpy(debug,"N");

    printf("Input  Process Number\n");
    if(setting_flag)
        printf("%d\n",process_num);
    else
        scanf("%d",&process_num);
    if(host_ip_flag){
        printf("Input Host --- \"Port\"\n");
        if(setting_flag)
            printf("%s\n",host_port);
        else
            scanf("%s",host_port);
    }
    else{
        printf("Input Host --- IP Port\n");
        scanf("%s",host_ip);
        scanf("%s",host_port);
    }

    // 開啟 Process
    temp = atoi(host_port);
    for (int i = 0; i < process_num; i++) {
        int sleep_num = rand()%100;
        int packet_loss_num = rand()%4;
        sprintf(p, "%d", temp);
        sprintf(ip_port, "%s %s %s %s %s %d %d", host_ip, p, console_ip, console_port, debug, sleep_num, packet_loss_num);

        char dir[] = "SocialNetwork_Bot.exe";
        // 跳新視窗 CREATE_NEW_CONSOLE
        // 一般設 0
        BOOL child;
        if(console_flag == true)
            child = CreateProcess(dir, ip_port, NULL, NULL, false, CREATE_NEW_CONSOLE, NULL, NULL,&info, &information);
        else if(console_flag == false)
            child = CreateProcess(dir, ip_port, NULL, NULL, false, 0, NULL, NULL,&info, &information);

        if (child) {
            //cout << "CreateProcess Successful" << endl;
        } else {
            cout << "CreateProcess Failure" << endl;
        }
        temp++;
        Sleep(5);
    }

    // Exit
    char instr[10]={};
    WSADATA wsadata;
    _socket::wsastartup_(&wsadata);
    while(strcmp(instr,"E")!=0 ){
        scanf("%s",instr);
        if(strcmp(instr,"E") == 0){
            strcpy(Buffer, "Kill");
            CopyMemory(lpBuffer, Buffer, sizeof(Buffer));
            printf("Kill Bot\n");
            bResult = UnmapViewOfFile(lpBuffer);
            if (!bResult) {
                printf("UnmapViewOfFile Failed Error: %d.\n", GetLastError());
                exit(-1);
            }
            else {
                printf("UnmapViewOfFile Success!\n");
            }
            return 0;
        }
    }

    system("pause");
    return 0;
}
char* get_IP(){
    //  调用WSAStarup初始化WINsock库
    WSADATA wsaData;
    ::WSAStartup(MAKEWORD(2,2),&wsaData);
    //  存放主机名的缓冲区
    char szHost[256];
    char *strIp = nullptr;
    char temp[20];
    //  取得本地主机名称
    ::gethostname(szHost, 256);
    //  通过主机名得到地址信息，一个主机可能有多个网卡，多个IP地址
    hostent *pHost = ::gethostbyname(szHost);
    in_addr addr;
    int i;
    for (i = 0;; i++){
        //获得地址（网络字节序）
        char *p = pHost->h_addr_list[i];
        if (p == NULL){
            break;
        }
        //  将地址拷贝到in_addr结构体中
        memcpy(&addr.S_un.S_addr, p, pHost->h_length);
        //  将in_addr转换为主机字节序
        char *strIp = ::inet_ntoa(addr);
        strcpy(temp,strIp);
        if( (temp[0]=='1') & (temp[1]=='9') & (temp[2]=='2') ){

        }
        else{
            ::WSACleanup();
            return strIp;
        }
    }
    //  终止对Winsock库的使用

    ::WSACleanup();
    return (char*)"Failed";
}