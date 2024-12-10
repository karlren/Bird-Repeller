#include "jsonrpc.h"
#include "mqtt.h"
#include "MyWiFi.h"
#include "deviceData.h"
#include "Task.h"
#include "httpOTA.h"
#include "MyOTA.h"
#include "PanTilt.h"
#include "deviceControl.h"

/* 设备号 */
int deviceID = 4;
/* LED IO */
const int LED_PIN = 2;

void LED_Task(void *parameter);

void setup() {
    Serial.begin(115200);
    deviceInit();
    httpOTA_Init();
    MQTT_Init();
    MyWiFi_Init();
    xTaskCreate(LED_Task, "LED_Task", 2048, NULL, 0, NULL);
    xTaskCreate(serial_Task, "SerialTask", 2048, NULL, 3, NULL);
}

void loop() {
    static int OTA_Initflag = 0;
    vTaskDelay(pdMS_TO_TICKS(200)); // 防止任务占用过多 CPU
    if (WiFi.status() == WL_CONNECTED) {
        if (OTA_Initflag == 0) {
            OTA_Init();
            Serial.printf("OTA Init Success\n");
            OTA_Initflag = 1;
        } else {
            OTA_Get();  /* OTA_Get不能放到任务函数里面，要不然bin文件会让任务栈空间溢出 */
        }
        httpOTA_Get();
    } else {
        MyWiFi_loop();
    }
}


void LED_Task(void *parameter) 
{
    pinMode(LED_PIN, OUTPUT);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(200)); // 防止任务占用过多 CPU
    }
    while (1) {
        if (WiFi.status() == WL_CONNECTED) {
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(500));
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(300));
        } else {
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(100));
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        Stack_Debug();
    }
}

