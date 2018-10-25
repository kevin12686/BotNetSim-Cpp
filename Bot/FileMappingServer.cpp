#include <iostream>
#include <windows.h>
#include <tchar.h>

using namespace std;

int main() {

    HANDLE hFileMap;
    BOOL bResult;
    PCHAR lpBuffer = nullptr;
    char Buffer[50];

    hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 256, _T("TIME"));
    if (!hFileMap) {
        printf("CreateFileMapping Failed Error: %d.\n", GetLastError());
        exit(-1);
    } else {
        printf("CreateFileMapping Success!\n");
    }
    lpBuffer = (PCHAR) MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 256);
    if (lpBuffer == nullptr) {
        printf("MapViewOfFile Failed Error: %d.\n", GetLastError());
        exit(-1);
    } else {
        printf("MapViewOfFile Success!\n");
    }

    for(short i = 0; i < 40; i++) {
        strcpy(Buffer, "T:20180930225900");
        CopyMemory(lpBuffer, Buffer, sizeof(Buffer));
        printf("End Writing!\n");

        Sleep(10000);
    }

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