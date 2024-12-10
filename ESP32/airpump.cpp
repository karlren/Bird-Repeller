#include "airpump.h"

// 全局对象的定义
airpump airpumps;

// 构造函数
airpump::airpump() 
{
    Airpump_Init();
}

void airpump::Airpump_Init()
{
    pinMode(inflatebtn,OUTPUT);//设置为输出模式
    pinMode(deflatebtn,OUTPUT);

    pinMode(stop_inflatebtn, INPUT);//设置为上拉输入模式

    digitalWrite(inflatebtn, LOW);//初始化为低电平
    digitalWrite(deflatebtn, LOW);
}

void airpump::Airpump_inflat(const String& level)
{
    if(level.equals("HIGH")){
        digitalWrite(inflatebtn, HIGH);//充气
    }else if (level.equals("LOW"))
    {
        digitalWrite(inflatebtn, LOW);//停止充气
    } 
}

void airpump::Airpump_stop_inflat()
{
    digitalWrite(inflatebtn, LOW);//停止充气
}

void airpump::Airpump_deflate(const String& level)
{
    if(level.equals("HIGH")){
        digitalWrite(deflatebtn, HIGH);//放气
    }else if (level.equals("LOW"))
    {
        digitalWrite(deflatebtn, LOW);//停止放气
    }
}