#include <iostream>
#include <windows.h>
#include "_thread.h"

_thread::_thread(int (*target)()) {
    this->Target = target;
}

_thread::~_thread() {
    this->Clean();
}

void _thread::start() {
    this->Thread_H = CreateThread(NULL, 0, this->Process, (LPVOID) this, 0, &(this->Thread_id));
    if (this->Thread_H == NULL) {
        std::cout << "[Error] Thread Created Error. (ErrorCode: \"" << GetLastError() << "\")" << std::endl;
    } else if (Debug) {
        std::cout << "[Debug INFO] Thread Created." << std::endl;
    }
}

void _thread::terminate() {
    if (TerminateThread(this->Thread_H, 0) && this->Debug) {
        std::cout << "[Debug INFO] Thread Terminated." << std::endl;
    } else {
        std::cout << "[Error] Thread Terminated Error. (ErrorCode: \"" << GetLastError() << "\")" << std::endl;
    }
    this->Clean();
}

void _thread::join() {
    WaitForSingleObject(this->Thread_H, INFINITE);
}

bool _thread::status() {
    return this->Status;
}

DWORD _thread::getThreadID() {
    return this->Thread_id;
}

void _thread::Clean() {
    CloseHandle(this->Thread_H);
    if (this->Debug) {
        std::cout << "[Debug INFO] Handle Cleaned" << std::endl;
    }
}