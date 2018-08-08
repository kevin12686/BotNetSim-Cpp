#include <iostream>
#include "_thread.h"

using namespace std;

void jobA(void);
void jobB(void);

int main() {
    _thread t((int (*)()) jobA), f((int (*)()) jobB);
    t.start();
    f.start();
    t.join();
    f.join();
    return 0;
}

void jobA() {
    cout << "I am doing a jobA." << endl;
}

void jobB() {
    cout << "I am doing a jobB." << endl;
}