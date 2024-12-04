#ifndef __MYTASK_H__
#define __MYTASK_H__
#include "MyWiFi.h"
#include <ArduinoJson.h>
#include "mqtt.h"
#define DEBUG 1

void Stack_Debug(void);

/* 在setup中创建此任务 */
/* xTaskCreate(serialTask, "SerialTask", 2048, NULL, 1, NULL); */
void serial_Task(void *parameter);

#endif // !__MYTASK_H__
