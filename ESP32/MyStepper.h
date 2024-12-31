#ifndef __MYSTEPPER_H__
#define __MYSTEPPER_H__
#include "AccelStepper.h"

#define CIRCLE_PULS          6400         //电机旋转一圈的脉冲数

class MyStepper : public AccelStepper {
public:
    MyStepper(int stepPin, int dirPin, int resetPin, int shellDetPin, uint32_t CIRCLE_Puls = CIRCLE_PULS);
    bool Init(void);
    bool load(void);
private:
    SemaphoreHandle_t mutex;  // 创建互斥锁句柄
    int StepPin, dirPin, resetPin, shellDetPin;
    uint32_t CIRCLE_Puls;
    uint32_t lastTime;
    uint32_t maxRunTime = 10000;
    void RunSpeedTo(uint16_t dis, uint16_t speed);
};

#endif
