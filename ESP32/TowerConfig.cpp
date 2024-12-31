#include "TowerConfig.h"


bool TowerConfig::begin(void) 
    {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS 初始化失败");
        return false;
    }
    return true;
}

JsonObject TowerConfig::saveFiringConfig(JsonObject params)
{
    static DynamicJsonDocument resultDoc(128);
    static DynamicJsonDocument errorDoc(128);
    static DynamicJsonDocument firingConfigDoc(1024);
    JsonObject errorObj = errorDoc.to<JsonObject>();
    JsonObject resultObj = resultDoc.to<JsonObject>();
    // 打开或创建文件
    File configFile = SPIFFS.open(configFileName, "w");
    if (!configFile) {
        errorObj["error"]["message"] = "无法打开文件进行写入";
        return errorObj;
    }
    firingConfigDoc["firingConfig"] = params["firingConfig"];
    // 将 JSON 对象写入文件
    serializeJson(firingConfigDoc, configFile);
    // 关闭文件
    configFile.close();
    resultObj["message"] = "配置已保存";
    return resultObj;
}

JsonObject TowerConfig::readFiringConfig(JsonObject params)
{
    static DynamicJsonDocument resultDoc(2048);
    static DynamicJsonDocument errorDoc(128);
    static DynamicJsonDocument ConfigDoc(2048);
    JsonObject errorObj = errorDoc.to<JsonObject>();
    File configFile = SPIFFS.open(configFileName, "r");
    if (!configFile) {
        errorObj["error"]["message"] = "无法打开文件";
        return errorObj;
    }
    // 读取文件内容并通过串口打印
    String fileContent = configFile.readString();
    DeserializationError deserializationError = deserializeJson(ConfigDoc, fileContent);
    if (deserializationError) {
        errorObj["error"]["message"] = "解析文件 JSON 失败:" + String(deserializationError.f_str());
        return errorObj;
    };
    resultDoc["firingConfig"] = ConfigDoc["firingConfig"];
    JsonObject resultObj = resultDoc.as<JsonObject>();
    resultObj["message"] = "读取配置成功";
    return resultObj;
}
