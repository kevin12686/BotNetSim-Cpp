#include <iostream>
#include <windows.h>
#include <tchar.h>

using namespace std;

int main() {
    //  调用WSAStarup初始化WINsock库
    WSADATA wsaData;
    ::WSAStartup(MAKEWORD(2,2),&wsaData);
    //  存放主机名的缓冲区
    char szHost[256];

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
        //  输出
        printf("IP[%d]:%s\n",i+1,strIp);
    }
    //  终止对Winsock库的使用
    ::WSACleanup();
    return 0;
}