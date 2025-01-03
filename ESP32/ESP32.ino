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
    xTaskCreate(LED_Task, "LED_Task", 2048, NULL, 0, NULL);
    xTaskCreate(serial_Task, "SerialTask", 8192, NULL, 3, NULL);
    deviceInit();
    httpOTA_Init();
    MQTT_Init();
    MyWiFi_Init();
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
    int lastStatus = WiFi.status();
    while (1) {
        if (WiFi.status() == WL_CONNECTED) {    // 3
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(500));
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(300));
        } else if (WiFi.status() == WL_IDLE_STATUS) {
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(300));
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(700));
        } else {
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(100));
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        if (WiFi.status() != lastStatus) {
            Serial.printf("当前网络状态：%d\n", WiFi.status());
            lastStatus = WiFi.status();
        }
        
        Stack_Debug();
    }
}

