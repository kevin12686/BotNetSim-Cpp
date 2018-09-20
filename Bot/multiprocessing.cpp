#include <iostream>
#include <vector>

#include "_socket.h"

using namespace std;


int main() {

    STARTUPINFO info = {sizeof(info)};
    PROCESS_INFORMATION information;
    char c_ip[20] = {}, c_port[10] = {};
    char ip[20] = {}, port[10] = {};
    char ip_port[40] = {}, p[10] = {};
    char debug[3] = {};
    int temp = 0, process_num = 0, t = -1;
    bool console_flag = false;

    printf("Input '1' : Create New Console\nInput '0' : No Operation\n");
    scanf("%d",&t);
    if(t == 1)
        console_flag = true;
    else
        console_flag = false;
    printf("Input 'Y' : OPEN Debug mode \nInput 'N' : No Operation\n");
    scanf("%s",debug);

    printf("Input  Process Number\n");
    scanf("%d",&process_num);

    printf("Input Console --- IP Port\n");
    scanf("%s",c_ip);
    scanf("%s",c_port);

    printf("Input Host --- IP Port\n");
    scanf("%s",ip);
    scanf("%s",port);

    temp = atoi(port);
    for (int i = 0; i < process_num; i++) {
        sprintf(p, "%d", temp);
        sprintf(ip_port, "%s %s %s %s %s", ip, p, c_ip, c_port, debug);

        char dir[] = "Main.exe";
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
        Sleep(10);
    }
    
    char instr[10]={};
    WSADATA wsadata;
    _socket::wsastartup_(&wsadata);
    while(1){
        scanf("%s",instr);
        if(strcmp(instr,"EXIT") == 0){
            char send_port[10] = {};
            temp = atoi(port);
            for (int i = 0; i < process_num; i++){
                sprintf(send_port, "%d", temp);
                _socket s((char *) "127.0.0.1", send_port, 1024);
                s.send_((char *) "Kill");
                temp++;
            }
            system("pause");
            return 0;
        }

    }

    // WaitForSingleObject(information.hProcess, INFINITE);
    //CloseHandle(information.hProcess);
    cout << "-----------closing" << endl;
    system("pause");
    return 0;
}
