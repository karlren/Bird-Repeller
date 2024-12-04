#ifndef __MYSTEPPER_H__
#define __MYSTEPPER_H__
#include "AccelStepper.h"

class MyStepper : public AccelStepper {
public:
    MyStepper(int stepPin, int dirPin, int resetPin, int shellDetPin, uint32_t CIRCLE_Puls);
    bool Init(void);
    bool load(void);
private:
    int StepPin, dirPin, resetPin, shellDetPin;
    uint32_t CIRCLE_Puls;
    void RunSpeedTo(uint16_t dis, uint16_t speed);
};

#endif
