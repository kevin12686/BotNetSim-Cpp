#include <iostream>
#include <vector>
#include <windows.h>

using namespace std;


int main() {

    STARTUPINFO info = {sizeof(info)};
    PROCESS_INFORMATION information;

    char ip[20] = {}, port[10] = {};
    char p[10] = {};
    char ip_port[40] = {};
    int temp = 0, process_num = 0, t = -1;
    bool console_flag = false;

    printf("Create New Console = 1\nNo Operation = 0\nInput:");
    scanf("%d",&t);
    if(t == 1)
        console_flag = true;
    else
        console_flag = false;
    printf("Input  Process_num\n");
    scanf("%d",&process_num);

    printf("Input  IP Port\n");
    scanf("%s",ip);
    scanf("%s",port);

    temp = atoi(port);

    for (int i = 0; i < process_num; i++) {
        sprintf(p, "%d", temp);
        sprintf(ip_port, "%s %s", ip,p);

        char dir[] = "Main.exe";
        // 跳新視窗 CREATE_NEW_CONSOLE
        // 一般設 0
        BOOL child;
        if(console_flag == true)
            child = CreateProcess(dir, ip_port, 0, 0, 0, CREATE_NEW_CONSOLE, 0, 0, &info, &information);
        else if(console_flag == false)
            child = CreateProcess(dir, ip_port, 0, 0, 0, CREATE_NO_WINDOW, 0, 0, &info, &information);


        if (child) {
            //cout << "CreateProcess Successful" << endl;
        } else {
            cout << "CreateProcess Failure" << endl;
        }
        temp++;
    }

    //cout << "-----------waitting" << endl;
    WaitForSingleObject(information.hProcess, INFINITE);
    CloseHandle(information.hProcess);
    cout << "-----------closing" << endl;
    system("pause");
    return 0;
}
