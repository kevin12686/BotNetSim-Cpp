#ifndef BOTNETSIM_CPP_THREADING_H
#define BOTNETSIM_CPP_THREADING_H

#include <iostream>
#include <winnt.h>

class Timer {
public:
    bool Debug = false;

    Timer(int);

    Timer(int *, int);

    Timer(int, int, int, int, int, int, int);

    int *getDateTime();

    int getYear();

    int getMonth();

    int getDay();

    int getHour();

    int getMinute();

    int getSecond();

    void lock();

    void release();

    void setDateTime(int *);

    void setYear(int);

    void setMonth(int);

    void setDay(int);

    void setHour(int);

    void setMinute(int);

    void setSecond(int);

    int getRate();

    void setRate(int);

    void timeGoOn();

    void run();

    void stop();

    bool getStatus();

    short getUpdateRate();

    void setUpdateRate(short);

    short dayInMonth();

    std::string toString();

    std::string timestamp();

private:
    // DateTime arr[6]{year, month, day, hour, minute, second}
    int Default_Datetime[6] = {2018, 1, 1, 0, 0, 0};
    int *DateTime;
    int TimePass = 0;
    // UpdateRate (mini second)
    short UpdateRate = 100;
    int Rate;
    bool Run;
    HANDLE time_mutex;
};

#endif //BOTNETSIM_CPP_THREADING_H