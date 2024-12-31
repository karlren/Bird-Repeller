#include "deviceControl.h"
#include "MyStepper.h"
#include "jsonrpc.h"
#include "mqtt.h"
#include "Task.h"
#include "TowerConfig.h"

/* 角度误差允许范围 */
float angleAllowsRangeErrors = 3.0f;

/* 装弹步进电机相关IO口 */
const int stepPin = 4;      /* 装弹步进电机驱动IO口 */
const int dirPin = 5;       /* 装弹步进电机方向IO口 */
const int resetPin = 23;    /* 装弹步进电机复位IO口 */
const int shellDetPin = 13; /* 装弹空弹检测IO口 */

/* 点火 */
const int ignitionPin = 27;         /* 点火击发IO口 */
SemaphoreHandle_t ignitionMutex;    /* 点火击发互斥锁句柄  */ 
int ignitionPinValidBit = HIGH;      /* 点火开关IO引脚有效位 */

/* 清理炮管气泵 */
const int cleanBarrelPin = 12;      /* 清理炮管气泵IO口 */
SemaphoreHandle_t cleanBarrelMutex; /* 清理炮管气泵互斥锁句柄 */ 
int cleanBarrelPinValidBit = LOW;  /* 气泵IO引脚有效位 */

Preferences devicePreferences;

PanTilt PT(19, 18);
MyStepper stepper(stepPin, dirPin, resetPin, shellDetPin);
TowerConfig towerConfig("/tower_config.json");

// TimerHandle_t angleReturnTimer;               /*角度回传的定时器*/

void GetDeviceData(void);
JsonObject reset(JsonObject params);
JsonObject PanTiltControl(JsonObject params);
JsonObject getPTAngle(JsonObject params);
JsonObject Stepperload(JsonObject params);
JsonObject StepperHoming(JsonObject params);
JsonObject getDeviceData(JsonObject params);
JsonObject loading(JsonObject params);
JsonObject changeDeviceID(JsonObject params);
JsonObject ignition(JsonObject params);
JsonObject cleanBarrel(JsonObject params);
JsonObject configDeviceInformation(JsonObject params);
JsonObject configFiringInfomation(JsonObject params);
JsonObject shoot(JsonObject params);

bool deviceInit(void)
{
    PT.begin(9600);
    towerConfig.begin();
    GetDeviceData();
    ignitionMutex = xSemaphoreCreateMutex();
    cleanBarrelMutex = xSemaphoreCreateMutex();

    pinMode(ignitionPin, OUTPUT);
    digitalWrite(ignitionPin, !ignitionPinValidBit);
    pinMode(cleanBarrelPin, OUTPUT);
    digitalWrite(cleanBarrelPin, !cleanBarrelPinValidBit);

    if (!rpc.registerProcedure(reset, "reset")) {
        Serial.println("函数reset注册失败");
    }
    if (!rpc.registerProcedure(PanTiltControl, "PanTiltControl")) {
        Serial.println("函数PanTiltControl注册失败");
    }
    if (!rpc.registerProcedure(getPTAngle, "getPTAngle")) {
        Serial.println("函数getPTAngle注册失败");
    }
    if (!rpc.registerProcedure(Stepperload, "Stepperload")) {
        Serial.println("函数Stepperload注册失败");
    }
    if (!rpc.registerProcedure(StepperHoming, "StepperHoming")) {
        Serial.println("函数StepperHoming注册失败");
    }
    if (!rpc.registerProcedure(getDeviceData, "getDeviceData")) {
        Serial.println("函数getDeviceData注册失败");
    }
    if (!rpc.registerProcedure(loading, "loading")) {
        Serial.println("函数loading注册失败");
    }
    if (!rpc.registerProcedure(changeDeviceID, "changeDeviceID")) {
        Serial.println("函数changeDeviceID注册失败");
    }
    if (!rpc.registerProcedure(ignition, "ignition")) {
        Serial.println("函数ignition注册失败");
    }
    if (!rpc.registerProcedure(cleanBarrel, "cleanBarrel")) {
        Serial.println("函数cleanBarrel注册失败");
    }
    if (!rpc.registerProcedure(configDeviceInformation, "configDeviceInformation")) {
        Serial.println("函数configDeviceInformation注册失败");
    }
    if (!rpc.registerProcedure(configFiringInfomation, "configFiringInfomation")) {
        Serial.println("函数configFiringInfomation注册失败");
    }
    if (!rpc.registerProcedure(shoot, "shoot")) {
        Serial.println("函数shoot注册失败");
    }
    
    // stepper.Init();
    return true;
}

void GetDeviceData(void)
{
    devicePreferences.begin("deviceData", false);
    if (devicePreferences.isKey("deviceID")) {
        deviceID = devicePreferences.getInt("deviceID", 0);
    } else {
        devicePreferences.putInt("deviceID", deviceID);
    }
}

JsonObject reset(JsonObject params)
{
    static DynamicJsonDocument resultDoc(128); 
    ESP.restart(); // 执行重启
    JsonObject resultObj = resultDoc.to<JsonObject>();
    return resultObj;
}

JsonObject PanTiltControl(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    String message = "";
    int command, param1 = 0, param2 = 0;

    /* 检查键是否存在 */ 
    if (!params.containsKey("command")) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "Missing command";
        return error;
    }
    command = params["command"].as<int>();
    if (params.containsKey("param1")) {
        param1 = params["param1"].as<int>();
        message = message + "param1 = " + String(param1);
    }
    if (params.containsKey("param2")) {
        param2 = params["param2"].as<int>();
        message = message + "param2 = " + String(param2);
    }
    if (PT.control(command, param1, param2)) {
        JsonObject result = doc.to<JsonObject>();
        result["message"] = message;
        result["deviceID"] = deviceID;
        return result;
    } else {
        JsonObject error = errorDoc.to<JsonObject>();
        // error["error"]["message"] = "Failed to control the PT";
        error["error"]["message"] = "与云台通讯异常";
        return error;
    }
}

JsonObject getPTAngle(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    // String message = "Obtaining the PT Angle succeeded";
    String message = "获取云台角度成功";
    float panAngle;         /* 水平角度 */
    float tiltAngle;        /* 垂直角度 */

    panAngle = PT.getPanAngle();

    if (panAngle != panAngle) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "与云台通讯异常";
        return error;
    }
    tiltAngle = PT.getTiltAngle();

    JsonObject result = doc.to<JsonObject>();

    result["message"] = message;
    // result["method"] = "returnPTAngle";
    result["deviceID"] = deviceID;
    result["panAngle"] = panAngle;
    result["tiltAngle"] = tiltAngle;

    return result;
}

JsonObject Stepperload(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    if (!stepper.load()) {
        JsonObject error = errorDoc.to<JsonObject>();
        // error["error"]["message"] = "Stepper motor loading failure";
        error["error"]["message"] = "无法控制步进电机装弹";
        return error;
    }
    // String message = "Stepper motor loading Successful";
    String message = "步进电机装弹完毕";
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}

JsonObject StepperHoming(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    if (!stepper.Init()) {
        JsonObject error = errorDoc.to<JsonObject>();
        // error["error"]["message"] = "Stepper motor Homing failure";
        error["error"]["message"] = "无法控制步进电机复位";
        return error;
    }
    // String message = "Stepper motor homing Successful";
    String message = "步进电机复位完毕";
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}

JsonObject getDeviceData(JsonObject params)
{
    double latitude, longitude, altitude;
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    JsonObject result = doc.to<JsonObject>();
    Preferences locationPreferences;
    locationPreferences.begin("location", false);
    latitude = locationPreferences.getDouble("latitude", 0);
    longitude = locationPreferences.getDouble("longitude", 0);
    altitude = locationPreferences.getDouble("altitude", 0);
    locationPreferences.end();
    
    result["IP"] = WiFi.localIP().toString();
    result["edition"] = String(currentmajor) + "." + String(currentminor) + "." + String(currentpatch);
    result["deviceID"] = deviceID;
    result["location"]["latitude"] = latitude;
    result["location"]["longitude"] = longitude;
    result["location"]["altitude"] = altitude;

    JsonObject firingConfig = towerConfig.readFiringConfig(result);
    result["firingConfig"] = firingConfig["firingConfig"];
    return result;
}

JsonObject loading(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    uint32_t lastTime = millis();
    /* 调用0号预设点 */
    if (!PT.control(12, 0, 0)) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "与云台通讯异常";
        return error;
    }
           
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        float panAngle;      /* 水平角度 */
        float tiltAngle;        /* 垂直角度 */
        panAngle = PT.getPanAngle();
        tiltAngle = PT.getTiltAngle();
        if (((panAngle > 58.0f && panAngle < 62.0f) || (panAngle < 2.0f || panAngle > 358.0f))
            && (tiltAngle > 358.0f || tiltAngle < 2.0f)) {
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        }
        if (millis() - lastTime > 5000) {
            // JsonObject error = errorDoc.to<JsonObject>();
            // error["error"]["message"] = "云台移动超时";
            // return error;
            break;
        }
    }
    if (!stepper.load()) {
        JsonObject error = errorDoc.to<JsonObject>();
        // error["error"]["message"] = "Stepper motor loading failure";
        error["error"]["message"] = "无法控制步进电机装弹";
        return error;
    }
    JsonObject result = doc.to<JsonObject>();
    // result["message"] = "loaded";
    result["message"] = "装弹成功";
    return result;
}

JsonObject changeDeviceID(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    int newID = -1;
    if (params.containsKey("newID")) {
        newID = params["newID"].as<int>();
    } else {
        JsonObject error = errorDoc.to<JsonObject>();
        // error["error"]["message"] = "Incorrect parameter. Failed to modify the device ID";
        error["error"]["message"] = "不正确的参数。修改设备ID失败";
        return error;
    }
    devicePreferences.putInt("deviceID", newID);
    // String message = "The device ID is changed successfully. The new ID is:" + String(newID);
    String message = "设备ID修改成功。新的ID为:" + String(newID);
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}

/* 点火击发函数，未测试 */
JsonObject ignition(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    
    // 获取互斥锁
    if (xSemaphoreTake(ignitionMutex, 0) != pdTRUE) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "点火中。。";
        return error; 
    }
    // String message = "The ignition was successful";
    String message = "设备" + String(deviceID) +"点火成功";
    digitalWrite(ignitionPin, ignitionPinValidBit);
    vTaskDelay(pdMS_TO_TICKS(1000));
    digitalWrite(ignitionPin, !ignitionPinValidBit);
    xSemaphoreGive(ignitionMutex);
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}

/* 清理炮管函数，未测试 */
JsonObject cleanBarrel(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    // 获取互斥锁
    if (xSemaphoreTake(cleanBarrelMutex, 0) != pdTRUE) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "当前正在清理炮管";
        return error; 
    }
    // String message = "Clean barrel successful";
    String message = "设备" + String(deviceID) +"清理炮管成功";
    digitalWrite(cleanBarrelPin, cleanBarrelPinValidBit);
    vTaskDelay(pdMS_TO_TICKS(1000));
    digitalWrite(cleanBarrelPin, !cleanBarrelPinValidBit);
    xSemaphoreGive(cleanBarrelMutex);
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}

JsonObject configDeviceInformation(JsonObject params)
{
    static DynamicJsonDocument doc(1024); 
    static DynamicJsonDocument errorDoc(128);
    int i;
    String mqttHost, mqttPort, mqttUsername, mqttPassword;
    String wifiSSID[maxWiFiConfigs], wifiPassword[maxWiFiConfigs];
    JsonObject error = errorDoc.to<JsonObject>();
    if (!params.containsKey("way")) {
        error["error"]["message"] = "参数错误，没有配置读写方式";
        return error;
    }
    String way = params["way"].as<String>();
    String message;
    JsonObject result = doc.to<JsonObject>();
    if (way == "read") {
        Preferences mqttPreferences;
        mqttPreferences.begin("mqtt", false);
        mqttHost = mqttPreferences.getString("host", "");
        mqttPort = mqttPreferences.getInt("port", 0);
        mqttUsername = mqttPreferences.getString("username", "");
        mqttPassword = mqttPreferences.getString("password", "");
        mqttPreferences.end();
        
        Preferences wifiPreferences;
        wifiPreferences.begin("wifi_config", false);
        for (i = 0; i < maxWiFiConfigs; i++) {
            wifiSSID[i] = wifiPreferences.getString(("wifi_" + String(i) + "_ssid").c_str(), "");
            wifiPassword[i] = wifiPreferences.getString(("wifi_" + String(i) + "_password").c_str(), "");
        }
        wifiPreferences.end();

        JsonObject content = result.createNestedObject("content");
        JsonObject mqttConfig = content.createNestedObject("mqttConfig");
        mqttConfig["mqttUsername"] = mqttUsername;
        mqttConfig["mqttPassword"] = mqttPassword;
        mqttConfig["mqttHost"] = mqttHost;
        mqttConfig["mqttPort"] = mqttPort;

        JsonArray wifiConfigs = content.createNestedArray("wifiConfigs");
        for (i = 0; i < maxWiFiConfigs; i++) {
            if (wifiSSID[i] == "") 
                continue;
            JsonObject wifiConfig = wifiConfigs.createNestedObject();
            wifiConfig["SSID"] = wifiSSID[i];
            wifiConfig["password"] = wifiPassword[i];
            wifiConfig["id"] = i;
        }
        message = "获取设备信息成功";
    } else if (way == "write") {
        if (!params.containsKey("content")) {
            error["error"]["message"] = "参数错误，没有content参数";
            return error;
        }
        JsonObject content = params["content"];
        if (content.containsKey("mqttConfig")) {
            JsonObject mqttConfig = content["mqttConfig"];
            mqttHost = mqttConfig["mqttHost"].as<String>();
            mqttPort = mqttConfig["mqttPort"].as<String>();
            mqttUsername = mqttConfig["mqttUsername"].as<String>();
            mqttPassword = mqttConfig["mqttPassword"].as<String>();
            Preferences mqttPreferences;
            mqttPreferences.begin("mqtt", false);
            mqttPreferences.putString("host", mqttHost);
            mqttPreferences.putInt("port", mqttPort.toInt());
            mqttPreferences.putString("username", mqttUsername);
            mqttPreferences.putString("password", mqttPassword);
            mqttPreferences.end();
        }
        if (content.containsKey("wifiConfigs")) {
            JsonArray wifiConfigs = content["wifiConfigs"];
            for (JsonObject wifiConfig : wifiConfigs) {
                int number = wifiConfig["id"].as<int>();
                wifiSSID[number] = wifiConfig["SSID"].as<String>();
                wifiPassword[number] = wifiConfig["password"].as<String>();
            }
            
            Preferences wifiPreferences;
            wifiPreferences.begin("wifi_config", false);
            for (i = 0; i < maxWiFiConfigs; i++) {
                wifiPreferences.putString((wifiKeyPrefix + String(i) + "_ssid").c_str(), wifiSSID[i]);
                wifiPreferences.putString((wifiKeyPrefix + String(i) + "_password").c_str(), wifiPassword[i]);
            }
            wifiPreferences.end();
            message = "保存设备信息成功";
        }
    }
    
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}

JsonObject configFiringInfomation(JsonObject params)
{
    static DynamicJsonDocument errorDoc(128);
    JsonObject errorObj = errorDoc.to<JsonObject>();
    if (!params.containsKey("way")) {
        errorObj["error"]["message"] = "参数错误，没有配置读写方式";
        return errorObj;
    }
    String way = params["way"].as<String>();
    if (way == "read") {
        return towerConfig.readFiringConfig(params);
    } else if (way == "write") {
        return towerConfig.saveFiringConfig(params);
    } else {
        errorObj["error"]["message"] = "参数way错误,未定义的方式";
        return errorObj;
    }
}

JsonObject shoot(JsonObject params)
{
    static DynamicJsonDocument resultDoc(128); 
    static DynamicJsonDocument errorDoc(128);
    JsonObject resultObj = resultDoc.to<JsonObject>();
    JsonObject errorObj = errorDoc.to<JsonObject>();
    String message = "";
    float PTpanAngle;      /* 设备当前水平角度 */
    float PTtiltAngle;        /* 设备当前垂直角度 */
    uint32_t lastTime = 0;

    if (!params.containsKey("panAngle") || !params.containsKey("tiltAngle")) {
        errorObj["error"]["message"] = "参数错误，没有配置方位";
        return errorObj;
    }

    /* 装弹 */
    lastTime = millis();
    /* 调用0号预设点 */
    if (!PT.control(12, 0, 0)) {
        errorObj["error"]["message"] = "调用0号预设点失败，与云台通讯异常";
        return errorObj;
    }
           
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(300));
        PTpanAngle = PT.getPanAngle();
        PTtiltAngle = PT.getTiltAngle();
        if ((PTpanAngle > (60.0f - angleAllowsRangeErrors) && PTpanAngle < (60.0f + angleAllowsRangeErrors))
            && (PTtiltAngle > (360.0f - angleAllowsRangeErrors) || PTtiltAngle < 0.0f + angleAllowsRangeErrors)) {
            Serial.printf("云台时效内到达装弹位置\n");
            Serial.printf("到达装弹位置 水平角度：%f,俯仰角度：%f\n", PTpanAngle, PTtiltAngle);
            break;
        }
        if (millis() - lastTime > 10000) {
            Serial.printf("云台到达装弹位置超时\n");
            break;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (!stepper.load()) {
        errorObj["error"]["message"] = "无法控制步进电机装弹";
        return errorObj;
    }
    vTaskDelay(pdMS_TO_TICKS(2000));

    float tiltAngle = params["tiltAngle"].as<float>();
    if (tiltAngle >= 0.0) {
        tiltAngle /= 2.56f;
    } else {
        tiltAngle += 360.0f;
        tiltAngle /= 2.56f;
    }
    int tiltAngleIntegerPart = static_cast<int>(std::floor(tiltAngle));
    int tiltAngleDecimalPart = static_cast<int>((tiltAngle - tiltAngleIntegerPart) * 100);
    tiltAngle *= 2.56f;
    if (!PT.control(21, tiltAngleIntegerPart, tiltAngleDecimalPart)) {
        errorObj["error"]["message"] = "设置俯仰角度失败，与云台通讯异常";
        return errorObj;
    }
    resultObj["tiltAngleIntegerPart"] = tiltAngleIntegerPart;
    resultObj["tiltAngleDecimalPart"] = tiltAngleDecimalPart;

    float panAngle = params["panAngle"].as<float>();
    if (panAngle >= 0.0) {
        panAngle /= 2.56f;
    } else {
        panAngle += 360.0f;
        panAngle /= 2.56f;
    }
    int panAngleIntegerPart = static_cast<int>(std::floor(panAngle));
    int panAngleDecimalPart = static_cast<int>((panAngle - panAngleIntegerPart) * 100);
    panAngle *= 2.56;
    if (!PT.control(20, panAngleIntegerPart, panAngleDecimalPart)) {
        errorObj["error"]["message"] = "设置水平角度失败，与云台通讯异常";
        return errorObj;
    }
    resultObj["panAngleIntegerPart"] = panAngleIntegerPart;
    resultObj["panAngleDecimalPart"] = panAngleDecimalPart;

    lastTime = millis();
    if (params.containsKey("waitTime")) {
        int waitTime = params["waitTime"].as<int>();
        vTaskDelay(pdMS_TO_TICKS(waitTime));
    } else {
        while (1) {
            PTpanAngle = PT.getPanAngle();
            PTtiltAngle = PT.getTiltAngle();
            Serial.printf("水平角度为%f,俯仰角度为%f\n", PTpanAngle, PTtiltAngle);
            if ((PTpanAngle > (panAngle - angleAllowsRangeErrors) && PTpanAngle < (panAngle + angleAllowsRangeErrors))
                && (PTtiltAngle > (tiltAngle - angleAllowsRangeErrors) && PTtiltAngle < (tiltAngle + angleAllowsRangeErrors))) {
                Serial.printf("击发角度调整正常\n");
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
            }
            if (millis() - lastTime > 10000) {
                Serial.printf("云台击发调整角度超时\n");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }

    if (params.containsKey("ignitionTime")) {
        int ignitionTime = params["ignitionTime"].as<int>();
        digitalWrite(ignitionPin, ignitionPinValidBit);
        vTaskDelay(pdMS_TO_TICKS(ignitionTime));
        digitalWrite(ignitionPin, !ignitionPinValidBit);
        message += "点火" + String(ignitionTime) + "ms；";
    } else {
        message += "无点火时间参数；";
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    if (params.containsKey("cleanTime")) {
        int cleanTime = params["cleanTime"].as<int>();
        digitalWrite(cleanBarrelPin, cleanBarrelPinValidBit);
        vTaskDelay(pdMS_TO_TICKS(cleanTime));
        digitalWrite(cleanBarrelPin, !cleanBarrelPinValidBit);
        message += "清理炮管" + String(cleanTime) + "ms；";
    } else {
        message += "无清理炮管时间参数；";
    }
    
    /* 调用250号预设点 */
    if (!PT.control(12, 0, 250)) {
        errorObj["error"]["message"] = "调用250号预设点失败，与云台通讯异常";
        return errorObj;
    }
    message += "运行成功！";
    resultObj["message"] = message;
    return resultObj;
}


// JsonObject setAngleReturn(JsonObject params)
// {
//     static DynamicJsonDocument doc(128); 
//     static DynamicJsonDocument errorDoc(128);
//     static bool timerStatus = false;
//     float panAngle;         /* 水平角度 */
//     float tiltAngle;        /* 垂直角度 */
//     if (params.containsKey("status")) {
//         JsonObject error = errorDoc.to<JsonObject>();
//         error["error"]["message"] = "无设置status值";
//         return error;
//     }
//     panAngle = PT.getPanAngle();
//     if (panAngle == NAN) {
//         JsonObject error = errorDoc.to<JsonObject>();
//         error["error"]["message"] = "与云台通讯异常,角度回传设置失败";
//         return error;
//     }

//     String status = params["status"];
//     if (status == "close" && timerStatus) {
//         // 停止定时器
//         xTimerStop(angleReturnTimer, 0);
//         timerStatus = false;
//         String message = "云台角度回传已关闭";
//         JsonObject result = doc.to<JsonObject>();
//         result["message"] = message;
//         result["deviceID"] = deviceID;
//         return result;
//     } else if (status == "open" && !timerStatus) {
//         // 创建定时器，回调函数每5秒触发一次
//         angleReturnTimer = xTimerCreate("AngleReturnTimer", pdMS_TO_TICKS(500), pdTRUE, (void*)0, angleReturnTimerCallback);
//         if (angleReturnTimer != NULL) {
//             // 启动定时器
//             xTimerStart(angleReturnTimer, 0);
//             timerStatus = true;
//         } else {
//             Serial.println("定时器创建失败！");
//             JsonObject error = errorDoc.to<JsonObject>();
//             error["error"]["message"] = "定时器创建失败";
//             return error;
//         }
//     } else {
//         JsonObject error = errorDoc.to<JsonObject>();
//         error["error"]["message"] = "参数status错误";
//         return error;
//     }

//     JsonObject result = doc.to<JsonObject>();
//     result["message"] = message;
//     result["deviceID"] = deviceID;
//     return result;
// }

/* 任务模板
JsonObject template(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    if (error) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "错误报错信息";
        return error;
    }
    String message = "成功报告信息";
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}
*/