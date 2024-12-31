#include "mqtt.h"
#include <ArduinoJson.h>
#include <AsyncMqtt_Generic.h>
#include "WiFi.h"
#include <Preferences.h>
#include "deviceData.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <HTTPClient.h>
#include <time.h>

static int mqttConnectionTimes = 0; /* MQTT连接次数，超过5次自动重启 */

void onMqttMessage(char* topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                   const size_t& len, const size_t& index, const size_t& total);
JsonObject mqttSaveHostConfig(JsonObject params);
JsonObject mqttSaveUserConfig(JsonObject params);

AsyncMqttClient mqttClient;                     /*MQTT客户端实例*/
TimerHandle_t mqttReconnectTimer;               /*MQTT重新连接的定时器*/
TimerHandle_t wifiReconnectTimer;               /*Wi-Fi重新连接的定时器*/

char receivedMessages[MAX_MESSAGE_LENGTH];      /* 存储接收到的消息的数组 */

Preferences mqtt_preferences;                   /* flash存储对象 */
IPAddress mqttHost = IPAddress(139, 9, 223, 99);/* 服务器IP对象 */
int mqttPort = 1883;                            /* 服务器端口号 */
String mqttUsername = "ESP32_device" + String(deviceID);                  /* MQTT登录账号 */
String mqttPassword = "123456";                 /* MQTT登录密码 */

TaskHandle_t mqttTaskHandle = NULL;             /* 任务句柄 */


void clear_mqtt_Preferences() {
    mqtt_preferences.clear(); // 清除所有Preferences数据
}

void reconnectToWiFi(void)
{
    // Serial.println("重新连接WiFi");
    // /* 尝试连接存储的WiFi */ 
    // for (int i = 0; i < maxWiFiConfigs; i++) {
    //     connectToWiFi(i);
    //     if (WiFi.status() == WL_CONNECTED) {
    //         break; // 如果已连接，跳出循环
    //     }
    // }
}

/*连接MQTT*/
void connectToMqtt()
{
    // Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

/*处理Wi-Fi连接状态的变化并根据状态决定是否连接到MQTT代理*/
void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
        /*区分不同esp32的内核版本*/
    #if USING_CORE_ESP32_CORE_V200_PLUS
        /*Wi-Fi准备就绪*/
        case ARDUINO_EVENT_WIFI_READY:
            // Serial.println("WiFi ready");
        break;
        /*ESP32的Wi-Fi站点模式（STA）启动*/
        case ARDUINO_EVENT_WIFI_STA_START:
            // Serial.println("WiFi STA starting");
        break;
        /*ESP32成功连接到Wi-Fi网络*/
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            // Serial.println("WiFi STA connected");
        break;
        /*ESP32成功获取到IPv6或IPv4地址*/
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            // Serial.println("WiFi connected");
            // Serial.print("IP address: ");
            // Serial.println(WiFi.localIP());
            connectToMqtt();
        break;
        /*ESP32失去IP地址*/
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Serial.println("WiFi lost IP");
        break;
        /*ESP32与Wi-Fi网络断开连接*/
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi lost connection");
            xTimerStop(mqttReconnectTimer, 0);  /*停止mqttReconnectTimer以确保在Wi-Fi重新连接之前不会重新连接MQTT代理。然后启动wifiReconnectTimer，尝试重新连接Wi-Fi*/
            xTimerStart(wifiReconnectTimer, 0);
        break;
    #else
        case SYSTEM_EVENT_STA_GOT_IP:
            // Serial.println("WiFi connected");
            // Serial.println("IP address: ");
            // Serial.println(WiFi.localIP());
            connectToMqtt();
        break;
    
        case SYSTEM_EVENT_STA_DISCONNECTED:
            // Serial.println("WiFi lost connection");
            xTimerStop(mqttReconnectTimer, 0); /*停止mqttReconnectTimer以确保在Wi-Fi重新连接之前不会重新连接MQTT代理。然后启动wifiReconnectTimer，尝试重新连接Wi-Fi*/
            xTimerStart(wifiReconnectTimer, 0);
        break;
    #endif
        default:
        break;
    }
}

uint16_t MQTT_Subscribe_Topics(const char *topic, uint8_t qos)
{
    return mqttClient.subscribe(topic, qos);
}

bool MQTT_Publish_Message(const char *topic, uint8_t qos, uint8_t retain, const char* message)
{
    return mqttClient.publish(topic, qos, retain, message);
}

/* 连接到MQTT的回调函数，打印连接信息 */
void onMqttConnect(bool sessionPresent)
{
#if _MY_MQTT_LOGLEVEL_ >= 3
    Serial.print("Connected to MQTT broker: ");
    Serial.print(mqttHost);
    Serial.print(", port: ");
    Serial.print(mqttPort);
    Serial.print(", Session present: ");
    Serial.println(sessionPresent);
#endif
    mqttSendDeviceData();
    mqttConnectionTimes = 0;
    MQTT_Subscribe_Topics("esp32/control", 0);
}

/* MQTT断开时的回调函数，reason是一个枚举类型，表示MQTT断开的原因 */
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    mqttConnectionTimes++;
    if (mqttConnectionTimes >= 10) {
        ESP.restart();
    }
#if _MY_MQTT_LOGLEVEL_ >= 1
    Serial.println("Disconnected from MQTT.");
#endif
    if (WiFi.isConnected()) {
        xTimerStart(mqttReconnectTimer, 0);
    } else {
        xTimerStart(wifiReconnectTimer, 0);
    }
}

/* MQTT订阅成功的回调函数，packetId是MQTT订阅的标识符，qos是信息传递的可靠级别 */
void onMqttSubscribe(const uint16_t& packetId, const uint8_t& qos)
{
#if _MY_MQTT_LOGLEVEL_ >= 4
    Serial.println("Subscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    Serial.print("  qos: ");
    Serial.println(qos);
#endif
}

/* MQTT取消订阅的回调函数，packetId是MQTT订阅的标识符 */
void onMqttUnsubscribe(const uint16_t& packetId)
{
#if _MY_MQTT_LOGLEVEL_ >= 4
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
#endif
}

/* MQTT消息发布后的回调函数 */
void onMqttPublish(const uint16_t& packetId)
{
#if _MY_MQTT_LOGLEVEL_ >= 4
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
#endif
}
JsonObject location(JsonObject params)
{
    static DynamicJsonDocument errorDoc(128);
    DynamicJsonDocument sendDoc(2048);
    DynamicJsonDocument rep(4096);
    HTTPClient http;
    String sendMessage;
    String recvPayload;
    int n = WiFi.scanNetworks();
    if (n < 0) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "周围无WiFi连接";
        // error["deviceID"] = deviceID;
        return error;
    }
    sendDoc["timestamp"] = time(NULL);
    sendDoc["id"] = mqttUsername;
    sendDoc["asset"]["id"] = mqttUsername;
    for(int i = 0; i < n; i++)
    {
        sendDoc["location"]["wifis"][i]["macAddress"] = WiFi.BSSIDstr(i);
        sendDoc["location"]["wifis"][i]["signalStrength"] = WiFi.RSSI(i);
    }
    serializeJson(sendDoc, sendMessage);
    static DynamicJsonDocument dOc(128); 
    JsonObject resu1t = dOc.to<JsonObject>();
    http.begin("https://api.newayz.com/location/hub/v1/track_points?access_key=lpWs3Pk2gaNcvUUYcZBWqin1l7fi6BTo");      
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Host", "api.newayz.com");
    http.addHeader("Connection", "keep-alive");
    int httpCode = http.POST(sendMessage);
    String payload = http.getString();
    DeserializationError error = deserializeJson(rep, payload);
serializeJson(rep, recvPayload);  // 将 JSON 文档序列化为字符串
Serial.println(recvPayload);      // 打印字符串
    if (error) 
    {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "服务器返回JSON数据解析错误";
        error["deviceID"] = deviceID;
        return error;
    }
    String address = rep["location"]["address"]["name"].as<String>();
    String place = rep["location"]["place"]["name"].as<String>();
    double longitude = rep["location"]["position"]["point"]["longitude"];
    double latitude = rep["location"]["position"]["point"]["latitude"];
    static DynamicJsonDocument doc(128); 
    JsonObject result = doc.to<JsonObject>();
    resu1t["location"]["address"] = address;
    resu1t["location"]["place"] = place;
    resu1t["position"]["longitude"] = longitude;
    resu1t["position"]["latitude"] = latitude;
    resu1t["deviceID"] = deviceID;
    result["deviceID"] = deviceID;
    String output;
    serializeJson(dOc, output);
    MQTT_Publish_Message("deviceData/address", 2, 1, output.c_str());

    Preferences locationPreferences;
    locationPreferences.begin("location", false);
    latitude = locationPreferences.putDouble("latitude", latitude);
    longitude = locationPreferences.putDouble("longitude", longitude);
    locationPreferences.end();
    return result;
}


void MQTT_Init(void)
{
    /* 初始化 Preferences */
    mqtt_preferences.begin("mqtt", false);

    /* 检查是否已经保存过服务器配置 */
    if (mqtt_preferences.isKey("host")) {
        mqttHost.fromString(mqtt_preferences.getString("host", ""));
        mqttPort = mqtt_preferences.getInt("port", 1883);
    } else {
        /* 如果没有存储的值，将默认值保存到闪存 */
        mqtt_preferences.putString("host", mqttHost.toString());
        mqtt_preferences.putInt("port", mqttPort);
    }
    /* 检查是否已经保存过MQTT用户配置 */
    if (mqtt_preferences.isKey("username")) {
        mqttUsername = mqtt_preferences.getString("username", "");
        mqttPassword = mqtt_preferences.getString("password", "");
    } else {
        mqtt_preferences.putString("username", mqttUsername);
        mqtt_preferences.putString("password", mqttPassword);
    }
#if _MY_MQTT_LOGLEVEL_ >= 3
    Serial.printf("HostIP : %s\n", mqttHost.toString().c_str());
    Serial.printf("Port : %d\n", mqttPort);
    Serial.printf("UserName : %s\n", mqttUsername.c_str());
    Serial.printf("Password : %s\n", mqttPassword.c_str());
#endif

#if _MY_MQTT_LOGLEVEL_ >= 4
    /* 打印MQTT版本信息 */
    Serial.println(ASYNC_MQTT_GENERIC_VERSION);
#endif
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
                                      reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
                                      reinterpret_cast<TimerCallbackFunction_t>(reconnectToWiFi));
    
    /*注册Wi-Fi事件处理函数 WiFiEvent*/
    WiFi.onEvent(WiFiEvent);
    /*注册MQTT连接成功后的回调函数*/
    mqttClient.onConnect(onMqttConnect);
    /*注册MQTT断开连接时的回调函数*/
    mqttClient.onDisconnect(onMqttDisconnect);
    /*注册MQTT订阅成功后的回调函数*/
    mqttClient.onSubscribe(onMqttSubscribe);
    /*注册取消订阅时的回调函数*/
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    /*注册接收MQTT消息后的回调函数*/
    mqttClient.onMessage(onMqttMessage);
    /*注册发布消息成功后的回调函数*/
    mqttClient.onPublish(onMqttPublish);
    
    /*设置MQTT服务器的IP地址和端口*/
    mqttClient.setServer(mqttHost, mqttPort);
    /*设置MQTT的用户名和密码，用于认证*/
    mqttClient.setCredentials(mqttUsername.c_str(), mqttPassword.c_str());
    /*设置MQTT客户端ID，一般用用户名作为客户端ID*/
    mqttClient.setClientId(mqttUsername.c_str()); 

    if (!rpc.registerProcedure(mqttSaveHostConfig, "saveMQTTHost")) {
        Serial.println("注册mqttSaveHostConfig函数失败");
    }
    if (!rpc.registerProcedure(mqttSaveUserConfig, "saveMQTTUser")) {
        Serial.println("注册mqttSaveUserConfig函数失败");
    }
    if (!rpc.registerProcedure(location, "location")) {
        Serial.println("注册location函数失败");
    }
}

void mqttSendDeviceData(void)
{
    vTaskDelay(pdMS_TO_TICKS(deviceID));
    // 创建 JSON 文档
    StaticJsonDocument<200> doc;
    doc["deviceID"] = deviceID;
    doc["IP"] = WiFi.localIP().toString();
    doc["edition"] = String(currentmajor) + "." + String(currentminor) + "." + String(currentpatch);
    // 序列化为字符串
    String output;
    serializeJson(doc, output);
    MQTT_Publish_Message("esp32/deviceList", 2, 0, output.c_str());
}

JsonObject mqttSaveHostConfig(JsonObject params)
{
    static DynamicJsonDocument errorDoc(128);
    IPAddress host;
    int port;
    String message;
    /* 检查键是否存在 */ 
    if (!params.containsKey("host")) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "Missing host";
        error["deviceID"] = deviceID;
        return error;
    }
    String hostip = params["host"].as<String>();
    host.fromString(hostip);
    mqtt_preferences.putString("host", host.toString());

    if (params.containsKey("port")) {
        port = params["port"].as<int>();
        mqtt_preferences.putInt("port", port);
        message = "Save MQTTHost Config : " + hostip + ", port : " + String(port);
    } else {
        message = "Save MQTTHost Config : " + hostip;
    }
    
    /* 创建一个动态JSON文档 */ 
    static DynamicJsonDocument doc(128); 
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}

JsonObject mqttSaveUserConfig(JsonObject params)
{
    static DynamicJsonDocument errorDoc(128);
    String message = "Save MQTTUser Config : ";
    String username, password;
    /* 检查键是否存在 */ 
    if (!params.containsKey("username") && !params.containsKey("password")) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "Missing username and password";
        error["deviceID"] = deviceID;
        return error;
    }
    if (params.containsKey("username")) {
        username = params["username"].as<String>();
        mqtt_preferences.putString("username", username);
        message = message + "username = " + username;
    }
    if (params.containsKey("password")) {
        password = params["password"].as<String>();
        mqtt_preferences.putString("password", password);
        message = message + "password = " + password;
    }
    
    /* 创建一个动态JSON文档 */ 
    static DynamicJsonDocument doc(128); 
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}

// 任务函数，用于处理接收到的MQTT消息
void mqttTask(void* pvParameters) 
{
    String payload = (char*)pvParameters;  // 从参数中获取消息内容
    DynamicJsonDocument mqttDoc(4096);
    DeserializationError error = deserializeJson(mqttDoc, payload);
    if (!error) {
        rpc.jsonParse(mqttDoc, "mqtt");
    } else {
        rpc.sendError("Failed to parse JSON", "mqtt");
    }

    // Serial.print("Handling MQTT message: ");
    // Serial.println(payload);
    
    UBaseType_t remainingStack = uxTaskGetStackHighWaterMark(NULL); // NULL表示获取当前任务的栈大小
    Serial.printf("MQTT任务剩余栈大小为: %d\n", remainingStack * sizeof(StackType_t));// 转换为字节
    // 任务结束时，删除任务
    vTaskDelete(NULL);
}

/****************************************订阅消息处理函数如下****************************************/
void onMqttMessage(char* topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                   const size_t& len, const size_t& index, const size_t& total)
{
    if (len == 0 || len == 1) {
        return;
    }
    payload[len] = '\0';
    // 将接收到的消息转为String
    String message = String((char*)payload);
    // 使用strdup复制消息内容到堆上
    char* messageCopy = strdup(message.c_str());

#if _MY_MQTT_LOGLEVEL_ >= 4
    Serial.println("Publish received.");
    Serial.print("  topic: ");
    Serial.println(topic);
    Serial.print("  qos: ");
    Serial.println(properties.qos);
    Serial.print("  dup: ");
    Serial.println(properties.dup);
    Serial.print("  retain: ");
    Serial.println(properties.retain);
    Serial.print("  len: ");
    Serial.println(len);
    Serial.print("  index: ");
    Serial.println(index);
    Serial.print("  total: ");
    Serial.println(total);
#endif
    if (!strcmp(topic, "esp32/control")) {
        xTaskCreate(
            mqttTask,              // 任务函数
            "MqttTask",            // 任务名称
            8192,                  // 任务栈大小
            (void*)messageCopy,     // 任务参数（MQTT消息）
            1,                     // 任务优先级
            &mqttTaskHandle        // 任务句柄
        );
    } else {
        Serial.printf("From [%s] : %s\n", topic, payload);
    }
}