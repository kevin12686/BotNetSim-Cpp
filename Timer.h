class Timer {
public:
    bool Debug = false;

    Timer(float);

    Timer(short *, float);

    Timer(short, short, short, short, short, short, float);

    short *getDateTime();

    short getYear();

    short getMonth();

    short getDay();

    short getHour();

    short getMinute();

    short getSecond();

    void setDateTime(short *);

    void setYear(short);

    void setMonth(short);

    void setDay(short);

    void setHour(short);

    void setMinute(short);

    void setSecond(short);

    float getRate();

    void setRate(float);

    void timeGoOn();

    void run();

    void stop();

    bool getStatus();

    short getUpdateRate();

    void setUpdateRate(short);

    short dayInMonth();

private:
    // DateTime {year, month, day, hour, minute, second}
    // UpdateRate (mini second)
    short DateTime[6], UpdateRate;
    float Rate;
    bool Run;
};