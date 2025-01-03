#include "esp32-hal-gpio.h"
#include "MyStepper.h"

// 构造函数实现
MyStepper::MyStepper(int stepPin, int dirPin, int resetPin, int shellDetPin, uint32_t CIRCLE_Puls)
    : AccelStepper(AccelStepper::DRIVER, stepPin, dirPin),
      StepPin(stepPin), dirPin(dirPin), resetPin(resetPin), shellDetPin(shellDetPin), CIRCLE_Puls(CIRCLE_Puls) 
{
    setMaxSpeed(CIRCLE_Puls * 2);           /* 设置最大速度 */
    setSpeed(CIRCLE_Puls * 2);              /* 设置运行速度 */
    setAcceleration(CIRCLE_Puls * 2);       /* 设置加速度 */
    pinMode(resetPin, INPUT_PULLUP);        // 设置 resetPin 为输入
    pinMode(shellDetPin, INPUT_PULLUP);     // 设置 shellDetPin 为输入
    mutex = xSemaphoreCreateMutex();
}

bool MyStepper::Init(void) {
    /* 获取互斥锁 */ 
    if (xSemaphoreTake(mutex, 1000) != pdTRUE) {
        Serial.println("无法获取步进电机互斥锁");
        return false;  /* 锁获取失败时返回 */ 
    }
    lastTime = millis();
    /* 设置目标位置 */ 
    move(CIRCLE_Puls * 10);
    /* 设置电机速度 */ 
    setSpeed(CIRCLE_Puls * 2);
    while (!digitalRead(resetPin)) {
        runSpeed();
        if (millis() - lastTime > maxRunTime) {
            Serial.printf("云台复位1超时\n");
            goto initFail;
        } else if ((millis() - lastTime) % 1000 == 0) {
            vTaskDelay(5);  /* 每秒喂狗防止卡死 */
        }
    }
    while (digitalRead(resetPin)) {
        runSpeed();
        if (millis() - lastTime > maxRunTime) {
            Serial.printf("云台复位2超时\n");
            goto initFail;
        } else if ((millis() - lastTime) % 1000 == 0) {
            vTaskDelay(5);  /* 每秒喂狗防止卡死 */
        }
    }
    /* 当 resetPin 电平改变后，停止电机 */ 
    stop();
    if (distanceToGo() == 0) {
        Serial.printf("云台复位3超时\n");
        goto initFail;
    }
    /* 释放互斥锁 */
    xSemaphoreGive(mutex);
    return true;
initFail:
    /* 释放互斥锁 */
    xSemaphoreGive(mutex);
    return false;
}

bool MyStepper::load(void)
{
    // 获取互斥锁
    if (xSemaphoreTake(mutex, 1000) != pdTRUE) {
        Serial.println("无法获取步进电机互斥锁");
        return false;  // 锁获取失败时返回
    }
    int num = 0;
    while (1) {
        if (digitalRead(shellDetPin) == HIGH) {
            break;
        }
        RunSpeedTo(CIRCLE_Puls * 10 / 6, CIRCLE_Puls * 2);
        num++;
        if (num >= 5) 
            goto loadFail;
    }
    RunSpeedTo(CIRCLE_Puls * 10 / 6, CIRCLE_Puls * 2);
    // 释放互斥锁
    xSemaphoreGive(mutex);
    return true;
loadFail:
    // 释放互斥锁
    xSemaphoreGive(mutex);
    return false;
}

void MyStepper::RunSpeedTo(uint16_t dis, uint16_t speed)
{
    // 设置目标位置
    move(dis);
    // 设置电机速度
    setSpeed(speed);
    // 持续运行，直到电机到达目标位置
    while (distanceToGo() != 0) {
        runSpeed();
    }
}
