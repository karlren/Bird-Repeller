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
int deviceID = 2;
/* LED IO */
const int LED_PIN = 2;

void LED_Task(void *parameter);

JsonObject rpcTest(JsonObject params)
{
    if (params.containsKey("message")) {
        Serial.printf("Test message = %s\n", params["message"].as<String>().c_str());
    }
    Serial.println("rpc test running\n");

    String message = "Thank you for your use";

    /* 需要是静态的，这样返回的时候才会不被销毁 */ 
    static DynamicJsonDocument doc(128); 
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}

void setup() {
    Serial.begin(115200);
    deviceInit();
    httpOTA_Init();
    MQTT_Init();
    MyWiFi_Init();
    if (!rpc.registerProcedure(rpcTest, "rpcTest"))
        Serial.println("rpcTask注册失败");
    xTaskCreate(LED_Task, "LED_Task", 2048, NULL, 0, NULL);
    xTaskCreate(serial_Task, "SerialTask", 2048, NULL, 3, NULL);
}

void loop() {
    static int OTA_Initflag = 0;
    vTaskDelay(200 / portTICK_PERIOD_MS); // 防止任务占用过多 CPU
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
        vTaskDelay(200 / portTICK_PERIOD_MS); // 防止任务占用过多 CPU
        // Serial.printf("connecting\n");
    }
    while (1) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Stack_Debug();
    }
}

