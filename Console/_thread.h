#ifndef BOTNETSIM_CPP_THREAD_H
#define BOTNETSIM_CPP_THREAD_H

#include <iostream>
#include <windows.h>

class _thread {
public:
    _thread(int (*)());

    ~_thread();

    void start();

    void terminate();

    void join();

    bool status();

    DWORD getThreadID();

private:
    HANDLE Thread_H = NULL;
    DWORD Thread_id = -1;
    bool Debug = false, Status = false;

    int (*Target)() = NULL;

    void Clean();

    DWORD static WINAPI Process(LPVOID target) {
        if (target == NULL) {
            std::cout << "Thread Error. (Target can not be NULL)" << std::endl;
            return 0;
        }
        _thread *t = (_thread *) (target);
        int (*ptr)() = t->Target;
        t->Status = true;
        ptr();
        t->Status = false;
        return 0;
    };
};

#endif //BOTNETSIM_CPP_THREAD_H
