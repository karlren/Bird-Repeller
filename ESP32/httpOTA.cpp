#include "httpOTA.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "deviceData.h"

static const char* versionUrl = "http://139.9.223.99/files/OTA/version.json";
static String currentVersion = "1.0.1"; // 当前版本

int currentmajor, currentminor, currentpatch;

Preferences httpOTA_preferences;

/* 保存最新的版本信息 */
static void httpOTA_SaveVersionInfo(int major, int minor, int patch)
{
    httpOTA_preferences.putInt("major", major);
    httpOTA_preferences.putInt("minor", minor);
    httpOTA_preferences.putInt("patch", patch);
}

/* 清除保存的版本信息 */
static void httpOTA_ClearVersionInfo(void)
{
    httpOTA_preferences.clear();
}

/* 读取代码中和flash中最新的版本号，可以放在初始化最前面 */
void httpOTA_Init(void)
{
    static int major = 0, minor = 0, patch = 0;
    httpOTA_preferences.begin("httpOTA", false); // 初始化Preferences

    int firstDot = currentVersion.indexOf('.');
    int secondDot = currentVersion.indexOf('.', firstDot + 1);

    currentmajor = currentVersion.substring(0, firstDot).toInt();
    currentminor = currentVersion.substring(firstDot + 1, secondDot).toInt();
    currentpatch = currentVersion.substring(secondDot + 1).toInt();

    /* 检查是否已经保存过版本信息 */
    if (httpOTA_preferences.isKey("major")) {
        major = httpOTA_preferences.getInt("major", 0);
    } else {
        httpOTA_preferences.putInt("major", currentmajor);
        Serial.printf("No major information\n");
        major = currentmajor;
    }
    if (httpOTA_preferences.isKey("minor")) {
        minor = httpOTA_preferences.getInt("minor", 0);
    } else {
        httpOTA_preferences.putInt("minor", currentminor);
        Serial.printf("No minor information\n");
        minor = currentminor;
    }
    if (httpOTA_preferences.isKey("patch")) {
        patch = httpOTA_preferences.getInt("patch", 0);
    } else {
        httpOTA_preferences.putInt("patch", currentpatch);
        Serial.printf("No patch information\n");
        patch = currentpatch;
    }
    // Serial.printf("major = %d, minor = %d, patch = %d\n", major, minor, patch);
    if (major > currentmajor || (major == currentmajor && minor > currentminor) 
        || (major == currentmajor && minor == currentminor && patch > currentpatch)) {
        /* 如果flash保存的版本号大于代码中的版本号，则以flash保存的版本为准 */ 
        currentmajor = major;
        currentminor = minor;
        currentpatch = patch;
        goto httpOTA_Init_end;
    } else if (major < currentmajor || (major == currentmajor && minor < currentminor) 
                || (major == currentmajor && minor == currentminor && patch < currentpatch)) {
        /* 如果flash保存的版本号小于代码中的版本号，则以代码中的版本为准，并将代码中的版本信息写入flash */
        httpOTA_SaveVersionInfo(currentmajor, currentminor, currentpatch);
        goto httpOTA_Init_end;
    }

httpOTA_Init_end:
    Serial.printf("The current version is \"%d.%d.%d\"\n", currentmajor, currentminor, currentpatch);
    return;
}
/* 根据服务器上的json文件判断是否有更新的版本，有就返回true */
static bool checkForUpdate(String &firmwareUrl) {
    HTTPClient http;
    http.begin(versionUrl);
    int httpCode = http.GET();
    int latestmajor, latestminor, latestpatch;
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        
        int latestDeviceID = doc["deviceID"];
        if (latestDeviceID != deviceID && latestDeviceID != 0) 
            return false;
        String latestVersion = doc["version"];
        String fileurl = doc["url"];
        firmwareUrl = fileurl;
        
        int firstDot = latestVersion.indexOf('.');
        int secondDot = latestVersion.indexOf('.', firstDot + 1);

        latestmajor = latestVersion.substring(0, firstDot).toInt();
        latestminor = latestVersion.substring(firstDot + 1, secondDot).toInt();
        latestpatch = latestVersion.substring(secondDot + 1).toInt();

        if (latestmajor > currentmajor || (latestmajor == currentmajor && latestminor > currentminor)
            || (latestmajor == currentmajor && latestminor == currentminor && latestpatch > currentpatch)) {
            httpOTA_SaveVersionInfo(latestmajor, latestminor, latestpatch);
            httpOTA_preferences.end();
            Serial.printf("latestVersion : \"%d.%d.%d\"\n", latestmajor, latestminor, latestpatch);
            Serial.printf("Updating firmware...\n");
            DynamicJsonDocument sendDoc(128);
            sendDoc["message"] = "Updating firmware...";
            sendDoc["latestVersion"] = String(latestmajor) + "." + String(latestminor) + "." + String(latestpatch);
            size_t size = measureJson(sendDoc);
            char output[size + 1];
            serializeJson(doc, output, size + 1);
            MQTT_Publish_Message("esp32/deviceList", 2, 0, output);
            vTaskDelay(pdMS_TO_TICKS(500));
            return true;
        } else {
            return false;
        }
    } else {
        Serial.printf("Failed to check for update: %s\n", http.errorToString(httpCode).c_str());
        return false;
    }
}
/* 下载url地址对应的bin文件并更新 */
static void downloadFirmware(const String &url) {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        WiFiClient *stream = http.getStreamPtr();
        size_t firmwareSize = http.getSize();

        if (Update.begin(firmwareSize)) {
            size_t written = Update.writeStream(*stream);

            if (written == firmwareSize) {
                Serial.println("Update complete. Rebooting...");
                Update.end(true);
                ESP.restart();
            } else {
                Serial.printf("Update failed, written bytes: %d\n", written);
                Update.end();
            }
        } else {
            Serial.println("Not enough space for update");
        }
    } else {
        Serial.printf("Firmware download failed: %s\n", http.errorToString(httpCode).c_str());
    }
}
/* 获取服务器上的json文件，判断如果版本更新就下载bin文件并更新 */
void httpOTA_Get(void)
{
    String firmwareUrl;
    if (checkForUpdate(firmwareUrl)) {
        Serial.printf("File URL : %s\n", firmwareUrl.c_str());
        downloadFirmware(firmwareUrl);
    } else {
        // Serial.println("No update available");
    }
}