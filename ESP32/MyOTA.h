#ifndef __MyOTA_H__
#define __MyOTA_H__
#include "MyWiFi.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
void OTA_Init(void);
void OTA_Get(void);
// void OTA_Task(void *parameter);
#endif
