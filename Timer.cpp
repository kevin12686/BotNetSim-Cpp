#include <iostream>
#include <windows.h>
#include "Timer.h"

using namespace std;

Timer::Timer(float rate) {
    this->setRate(rate);
    this->Run = false;
}

Timer::Timer(short *datetime, float rate) {
    this->setDateTime(datetime);
    this->setRate(rate);
    this->Run = false;
}

Timer::Timer(short year, short month, short day, short hour, short minute, short second, float rate) {
    short datetime[6] = {year, month, day, hour, minute, second};
    this->setDateTime(datetime);
    this->setRate(rate);
    this->Run = false;
}

short *Timer::getDateTime() {
    return this->DateTime;
}


short Timer::getYear() {
    return (this->DateTime)[0];
}

short Timer::getMonth() {
    return (this->DateTime)[1];
}

short Timer::getDay() {
    return (this->DateTime)[2];
}

short Timer::getHour() {
    return (this->DateTime)[3];
}

short Timer::getMinute() {
    return (this->DateTime)[4];
}

short Timer::getSecond() {
    return (this->DateTime)[5];
}

void Timer::setDateTime(short *datetime) {
    this->DateTime = datetime;
}

void Timer::setYear(short year) {
    (this->DateTime[0]) = year;
}

void Timer::setMonth(short month) {
    (this->DateTime[1]) = month;
}

void Timer::setDay(short day) {
    (this->DateTime[2]) = day;
}

void Timer::setHour(short hour) {
    (this->DateTime[3]) = hour;
}

void Timer::setMinute(short minute) {
    (this->DateTime[4]) = minute;
}

void Timer::setSecond(short second) {
    (this->DateTime[5]) = second;
}

float Timer::getRate() {
    return this->Rate;
}

void Timer::setRate(float rate) {
    this->Rate = rate;
}

void Timer::timeGoOn() {
    bool flag = false;
    short passTime = (short) (((this->UpdateRate) / 1000) * this->Rate);
    this->DateTime[5] += passTime;
    // Second
    if (this->DateTime[5] > 59) {
        this->DateTime[5] = 0;
        this->DateTime[4]++;
    } else {
        // Minute
        if (this->DateTime[4] > 59) {
            this->DateTime[4] = 0;
            this->DateTime[3]++;
        }
        // Hour
        if ((this->DateTime[3]) > 23) {
            this->DateTime[3] = 0;
            this->DateTime[2]++;
        }
        // Day
        if (this->DateTime[2] > this->dayInMonth()) {
            this->DateTime[2] = 1;
            this->DateTime[1]++;
        }
        // Month
        if (this->DateTime[1] > 12) {
            this->DateTime[1] = 1;
            this->DateTime[0]++;
        }
    }
}

void Timer::run() {
    if (this->Debug) {
        cout << "[Debug INFO] Timer start." << endl;
    }
    this->Run = true;
    while (this->Run) {
        Sleep(this->UpdateRate);
        this->timeGoOn();
    }
    if (this->Debug) {
        cout << "[Debug INFO] Timer stop." << endl;
    }
}

void Timer::stop() {
    this->Run = false;
}

bool Timer::getStatus() {
    return this->Run;
}

short Timer::getUpdateRate() {
    return this->UpdateRate;
}

void Timer::setUpdateRate(short update_rate) {
    this->UpdateRate = update_rate;
}

short Timer::dayInMonth() {
    bool flag = false;
    short day31[7] = {1, 3, 5, 7, 8, 10, 12};
    short year = this->DateTime[0], month = this->DateTime[1], result = 0;

    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0 && year % 3200 != 0)) {
            result = 29;
        } else {
            result = 28;
        }
    } else {
        for (int i; i < 7; i++) {
            if (month == day31[i]) {
                flag = true;
                break;
            }
        }
        if (flag) {
            result = 31;
        } else {
            result = 30;
        }
    }
    return result;
}