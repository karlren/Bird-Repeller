#ifndef __MYWIFI_H__
#define __MYWIFI_H__
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "jsonrpc.h"

extern WebServer server;
extern const int maxWiFiConfigs;   /* 最大WiFi配置数量 */
// extern Preferences wifi_preferences;

// 用于存储WiFi SSID和密码的键
extern String wifiKeyPrefix; // 存储WiFi数据的前缀
extern String idKey; // 存储WiFi配置数量的键

/* 在setup中调用 */
void MyWiFi_Init(void);
void connectToWiFi(int index);
void MyWiFi_loop();
#endif // !__MYWIFI_H__