#include "Task.h"
#include "deviceData.h"
#include "jsonrpc.h"

static DynamicJsonDocument g_doc(4096);

void Stack_Debug(void)
{
    UBaseType_t remainingStack = uxTaskGetStackHighWaterMark(NULL); // NULL表示获取当前任务的栈大小
    #if DEBUG == 1
        if (remainingStack * sizeof(StackType_t) < 200) {
            Serial.printf("LED Task Remaining stack size: %d\n", remainingStack * sizeof(StackType_t));// 转换为字节
        }
    #elif DEBUG == 2
        Serial.printf("LED Task Remaining stack size: %d\n", remainingStack * sizeof(StackType_t));// 转换为字节
    #endif
}


void serial_Task(void *parameter) 
{
    while (true) {
        if (Serial.available()) {
            String json = Serial.readStringUntil('\n');
            DeserializationError error = deserializeJson(g_doc, json);

            if (!error) {
                rpc.jsonParse(g_doc, "serial");
            } else {
                // Serial.println("Failed to parse JSON");
                rpc.sendError("Failed to parse JSON", "serial");
            }
        }
        Stack_Debug();
        vTaskDelay(pdMS_TO_TICKS(1000)); // 防止任务占用过多 CPU
        g_doc.clear();
    }
}