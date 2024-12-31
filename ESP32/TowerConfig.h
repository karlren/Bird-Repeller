#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

class TowerConfig {
public:
    // 构造函数
    TowerConfig(const char* fileName) : configFileName(fileName) {}

    // 初始化 SPIFFS 文件系统
    bool begin(void);

    // 保存炮塔配置到文件
    JsonObject saveFiringConfig(JsonObject params);

    // 读取文件并解析 JSON 数据
    JsonObject readFiringConfig(JsonObject params);

private:
    const char* configFileName; // 配置文件名
};

