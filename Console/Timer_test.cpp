#include <iostream>
#include <windows.h>
#include "Timer.h"

using namespace std;

DWORD WINAPI timer(LPVOID);

int main() {
    HANDLE acc_thread;
    DWORD thread_id;

    float rate = 1;

    cout << "Rate: ";
    cin >> rate;

    Timer *time = new Timer(rate);
    time->Debug = true;
    acc_thread = CreateThread(NULL, 0, timer, (LPVOID) time, 0, &thread_id);

    if (acc_thread == NULL) {
        cout << "Thread Created Error." << endl;
        return 1;
    }

    for (short i = 0; i < 10; i++) {
        Sleep(1000);
        cout << time->toString() << endl;
    }
    time->stop();

    WaitForSingleObject(acc_thread, INFINITE);
    CloseHandle(acc_thread);

    delete (time);

    return 0;
}

DWORD WINAPI timer(LPVOID obj) {
    Timer *t = (Timer *) obj;
    t->run();
    return 1;
}