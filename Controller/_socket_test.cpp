#include "_socket.h"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <iostream>
#include <sstream>

HANDLE hSlot;
LPTSTR SlotName = TEXT((char*)"\\\\.\\mailslot\\7777");
char* boardcast;
char buffer[256];
DWORD NumberOfBytesRead;

BOOL WINAPI MakeSlot(LPTSTR lpszSlotName)
{
    hSlot = CreateMailslot(lpszSlotName,
                           0,                             // no maximum message size
                           MAILSLOT_WAIT_FOREVER,         // no time-out for operations
                           (LPSECURITY_ATTRIBUTES) NULL); // default security

    if (hSlot == INVALID_HANDLE_VALUE)
    {
        printf("CreateMailslot failed with %d\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

int main()
{
    WSADATA wsadata;
    _socket::Debug = false;

    _socket::wsastartup_(&wsadata);

    _socket f((char *) "127.0.0.1", (char *) "1999", 1024);
    f.send_((char*) "SS7777");
    f.close_();

    MakeSlot(SlotName);
    bool FLAG = TRUE;
    while(FLAG)
    {
        if(ReadFile(hSlot, buffer, 256, &NumberOfBytesRead, nullptr)) {
            printf("%.*s\n", NumberOfBytesRead, buffer);
        }

    }

    return 0;
}