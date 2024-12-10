#ifndef AIRPUMP_H
#define AIRPUMP_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class airpump
{
public:
    airpump();
    void Airpump_Init();
    void Airpump_inflat(const String& level);
    void Airpump_stop_inflat();
    void Airpump_deflate(const String& level);
private:
    const int stop_inflatebtn = 34;//停止充气
    const int inflatebtn = 14;//充气
    const int deflatebtn = 15;//放气
};

extern airpump airpumps;

#endif