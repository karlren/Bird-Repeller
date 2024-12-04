#ifndef __MQTT_H__
#define __MQTT_H__
#include "jsonrpc.h"
#include "deviceData.h"
#include "MyWiFi.h"

#define MQTT_HOST         IPAddress(139, 9, 223, 99)
#define MQTT_PORT         1883
#define MQTT_USERNAME     "ESP32"
#define MQTT_PASSWORD     "123456"

/* 0:无日志（禁用日志输出） */
/* 1:错误级别日志（仅输出错误信息） */
/* 2:警告级别日志（输出警告和错误信息） */
/* 3:信息级别日志（输出普通信息、警告和错误信息） */
/* 4:调试级别日志（输出详细调试信息） */
#define _ASYNC_MQTT_LOGLEVEL_   1   /* 设置异步 MQTT 库的日志级别 */
#define _MY_MQTT_LOGLEVEL_      3   /* 设置本库的日志级别 */ 
/* 最大消息长度 */
#define MAX_MESSAGE_LENGTH 512
/* 存储接收到的消息的数组 */
extern char receivedMessages[MAX_MESSAGE_LENGTH]; 


void MQTT_Init(void);
uint16_t MQTT_Subscribe_Topics(const char *topic, uint8_t qos);
bool MQTT_Publish_Message(const char *topic, uint8_t qos, uint8_t retain, const char* message);
void mqttSendDeviceData(void);
#endif // !__MQTT_H__