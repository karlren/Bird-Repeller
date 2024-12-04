#include <string>
#include "MyWiFi.h"
#include <DNSServer.h>


/* 清除数据引脚，开机时接低电平就会清空flash中存储的wifi数据 */
static const int Clear_Button_Pin = 25;

WebServer server(80);
DNSServer dns; // 创建 DNS 服务器实例
Preferences wifi_preferences;

const char* apSSID = "ESP32-Access-Point"; // AP模式下的SSID
const char* apPassword = "123456789"; // AP模式下的密码
const int maxWiFiConfigs = 5; // 最大WiFi配置数量

// 用于存储WiFi SSID和密码的键
String wifiKeyPrefix = "wifi_"; // 存储WiFi数据的前缀
String idKey = "wifi_count"; // 存储WiFi配置数量的键

JsonObject Save_WiFi(JsonObject params);

void clear_wifi_Preferences() {
    wifi_preferences.clear(); // 清除所有Preferences数据
}

void connectToWiFi(int index) 
{
    String ssid = wifi_preferences.getString((wifiKeyPrefix + String(index) + "_ssid").c_str(), "");
    String password = wifi_preferences.getString((wifiKeyPrefix + String(index) + "_password").c_str(), "");

    if (ssid.length() > 0 && password.length() > 0) {
        Serial.println("Connecting to stored WiFi[" + String(index) + "]: " + ssid);
        WiFi.begin(ssid.c_str(), password.c_str());

        // 等待连接
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 10) { // 最多尝试10次
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to WiFi: " + ssid);
        } else {
            Serial.println("\nFailed to connect to: " + ssid);
        }
    }
}

void handleRoot()
{
    String html = "<html><head>"
                  "<meta charset='utf-8'>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                  "<title>WiFi 配置</title>"
                  "<style>"
                  "input[type='text'] { width: 100%; margin-bottom: 10px; }" // 输入框下方间隔
                  "input[type='submit'] { width: 100%; margin-top: 10px; }" // 按钮上方间隔
                  "</style>"
                  "</head><body>"
                  "<h2>WiFi 配置</h2>"
                  "<form action='/connect' method='POST'>"
                  "WiFi账号: <input type='text' name='ssid'><br>"
                  "WiFi密码: <input type='text' name=e'password'><br>"
                  "<input type='submit' value='提交WiFi信息'>"
                  "</form>"
                  "<form action='/restart' method='POST'>"
                  "<input type='submit' value='重启 ESP32' style='margin-top: 10px;'>"
                  "</form>"
                  "</body></html>";
    server.send(200, "text/html", html);
}



void handleRestart() {
    server.send(200, "text/html", "ESP32 is restarting...");
    delay(1000); // 给用户一点时间看到消息
    ESP.restart(); // 执行重启
}

void handleConnect() {
    if (server.arg("ssid") != "") {
        wifi_preferences.putString((wifiKeyPrefix + String(wifi_preferences.getInt(idKey.c_str(), 0)) + "_ssid").c_str(), server.arg("ssid"));
    }
    if (server.arg("password") != "") {
        wifi_preferences.putString((wifiKeyPrefix + String(wifi_preferences.getInt(idKey.c_str(), 0)) + "_password").c_str(), server.arg("password"));
    }
    Serial.printf("WiFi %s configuration is successful.\n", server.arg("ssid"));
    int wifiCount = wifi_preferences.getInt(idKey.c_str(), 0);
    wifi_preferences.putInt(idKey.c_str(), wifiCount + 1); // 更新WiFi配置数量
    // 返回包含按钮的HTML
    String html = "<html><head>"
                  "<meta charset='utf-8'>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                  "<title>配置成功</title>"
                  "<style>"
                  "h2 { font-size: 24px; }" // 放大字体
                  "form { text-align: center; margin-top: 20px; }" // 居中对齐
                  "input[type='submit'] { width: 50%; margin: 10px 0; }" // 按钮宽度和间隔
                  "</style>"
                  "</head><body>"
                  "<h2>WiFi 配置已保存。</h2>"
                  "<form action='/' method='GET'>"
                  "<input type='submit' value='继续添加 WiFi'>"
                  "</form>"
                  "<form action='/restart' method='POST'>"
                  "<input type='submit' value='重启 ESP32'>"
                  "</form>"
                  "</body></html>";

    server.send(200, "text/html", html);
}

void handleNotFound() 
{
    String url = server.uri();
    Serial.println("Requested URL: " + url);
    
    // 重定向到 ESP32 的 IP 地址
    server.sendHeader("Location", "http://192.168.4.1", true);
    server.send(302, "text/plain", "Redirecting...");
}

void MyWiFi_Init(void)
{
    int i;
    wifi_preferences.begin("wifi_config", false); // 初始化Preferences

    // 检查Clear_Button_Pin引脚状态
    pinMode(Clear_Button_Pin, INPUT_PULLDOWN);
    if (digitalRead(Clear_Button_Pin) == HIGH) {
        clear_wifi_Preferences(); // 清除Flash中的内容
        Serial.println("WiFi Preferences cleared.\n");
    }
    rpc.registerProcedure(Save_WiFi, "SaveWiFi");

    // 尝试连接存储的WiFi
    for (i = 0; i < maxWiFiConfigs; i++) {
        connectToWiFi(i);
        if (WiFi.status() == WL_CONNECTED) {
            return; // 如果已连接，返回
        }
    }

    // 如果没有连接，启动AP模式
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.softAP(apSSID, apPassword);
        Serial.println("Access Point started. Connect to WiFi: " + String(apSSID));
        Serial.println("Set the ip address of WiFi to 192.168.4.1");
        // 启动 DNS 服务器
        dns.start(53, "*", WiFi.softAPIP());

        server.onNotFound(handleNotFound);
        server.on("/", handleRoot);
        server.on("/connect", HTTP_POST, handleConnect);
        server.on("/restart", HTTP_POST, handleRestart);
        server.begin();
    }
}

void MyWiFi_loop() 
{
    dns.processNextRequest(); // 处理 DNS 请求
    server.handleClient(); // 处理 HTTP 请求
}

JsonObject Save_WiFi(JsonObject params) 
{
    static DynamicJsonDocument errorDoc(128);
    /* 检查键是否存在 */ 
    if (!params.containsKey("id")) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "Missing id";
        error["deviceID"] = deviceID;
        return error;
    } 
    if (!params.containsKey("SSID")) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "Missing SSID";
        error["deviceID"] = deviceID;
        return error;
    }
    if (!params.containsKey("password")) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "Missing password";
        error["deviceID"] = deviceID;
        return error;
    }

    int id = params["id"];
    String ssid = params["SSID"].as<String>(); /* 使用 as<String>() 确保类型正确 */ 
    String password = params["password"].as<String>();

    /* 保存 WiFi 信息 */ 
    wifi_preferences.putString((wifiKeyPrefix + String(id) + "_ssid").c_str(), ssid);
    wifi_preferences.putString((wifiKeyPrefix + String(id) + "_password").c_str(), password);

    String message = "Received: SSID: " + ssid + ", Password: " + password + ", id: " + String(id);

    /* 创建一个动态JSON文档 */ 
    static DynamicJsonDocument doc(128); 
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}