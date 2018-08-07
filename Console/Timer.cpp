#include <iostream>
#include <sstream>
#include <windows.h>
#include "Timer.h"

Timer::Timer(float rate) {
    this->setDateTime(this->Default_Datetime);
    this->setRate(rate);
    this->Run = false;
}

Timer::Timer(int *datetime, float rate) {
    this->setDateTime(datetime);
    this->setRate(rate);
    this->Run = false;
}

Timer::Timer(int year, int month, int day, int hour, int minute, int second, float rate) {
    int datetime[6] = {year, month, day, hour, minute, second};
    this->setDateTime(datetime);
    this->setRate(rate);
    this->Run = false;
}

int *Timer::getDateTime() {
    return this->DateTime;
}


int Timer::getYear() {
    return this->DateTime[0];
}

int Timer::getMonth() {
    return this->DateTime[1];
}

int Timer::getDay() {
    return this->DateTime[2];
}

int Timer::getHour() {
    return this->DateTime[3];
}

int Timer::getMinute() {
    return this->DateTime[4];
}

int Timer::getSecond() {
    return this->DateTime[5];
}

void Timer::setDateTime(int *datetime) {
    this->DateTime = datetime;
}

void Timer::setYear(int year) {
    (this->DateTime[0]) = year;
}

void Timer::setMonth(int month) {
    (this->DateTime[1]) = month;
}

void Timer::setDay(int day) {
    (this->DateTime[2]) = day;
}

void Timer::setHour(int hour) {
    (this->DateTime[3]) = hour;
}

void Timer::setMinute(int minute) {
    (this->DateTime[4]) = minute;
}

void Timer::setSecond(int second) {
    (this->DateTime[5]) = second;
}

float Timer::getRate() {
    return this->Rate;
}

void Timer::setRate(float rate) {
    this->Rate = rate;
}

void Timer::timeGoOn() {
    if (this->TimePass >= 1000) {
        short days = this->dayInMonth();
        int temp = this->TimePass;
        this->TimePass %= 1000;
        int passTime = (int) ((temp - this->TimePass) / 1000 * this->Rate);
        this->DateTime[5] += passTime;
        // Second
        if (this->DateTime[5] > 59) {
            temp = this->DateTime[5];
            this->DateTime[5] %= 60;
            this->DateTime[4] += (temp - this->DateTime[5]) / 60;
            // Minute
            if (this->DateTime[4] > 59) {
                temp = this->DateTime[4];
                this->DateTime[4] %= 60;
                this->DateTime[3] += (temp - this->DateTime[4]) / 60;
            }
            // Hour
            if ((this->DateTime[3]) > 23) {
                temp = this->DateTime[3];
                this->DateTime[3] %= 24;
                this->DateTime[2] += (temp - this->DateTime[3]) / 24;
            }
            // Day
            if (this->DateTime[2] > days) {
                temp = this->DateTime[2];
                this->DateTime[2] %= days;
                this->DateTime[1] += (temp - this->DateTime[2]) / days;
            }
            // Month (Careful: No Defensive)
            if (this->DateTime[1] > 12) {
                this->DateTime[1] -= 12;
                this->DateTime[0]++;
            }
        }
    }
}

void Timer::run() {
    if (this->Debug) {
        std::cout << "[Debug INFO] Timer start." << std::endl;
    }
    this->Run = true;
    while (this->Run) {
        Sleep(this->UpdateRate);
        this->TimePass += this->UpdateRate;
        this->timeGoOn();
    }
    if (this->Debug) {
        std::cout << "[Debug INFO] Timer stop." << std::endl;
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
        for (short i = 0; i < 7; i++) {
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

std::string Timer::toString() {
    std::ostringstream ostream;
    ostream << this->getYear() << "/" << this->getMonth() << "/" << this->getDay();
    ostream << " " << this->getHour() << ":" << this->getMinute() << ":" << this->getSecond();
    return ostream.str();
}