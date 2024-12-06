#include "deviceControl.h"
#include "MyStepper.h"
#include "jsonrpc.h"
#include "mqtt.h"

#define CIRCLE_Puls          6400         //电机旋转一圈的脉冲数

/* 装弹步进电机相关IO口 */
const int stepPin = 4;
const int dirPin = 5;
const int resetPin = 12;
const int shellDetPin = 13;

/* 点火击发IO口 */
const int IgnitionPin = 11; /* 未确定 */

Preferences devicePreferences;

PanTilt PT(18, 19);
MyStepper stepper(stepPin, dirPin, resetPin, shellDetPin, CIRCLE_Puls);

void GetDeviceData(void);
JsonObject PanTiltControl(JsonObject params);
JsonObject getPTAngle(JsonObject params);
JsonObject Stepperload(JsonObject params);
JsonObject StepperHoming(JsonObject params);
JsonObject getDeviceData(JsonObject params);
JsonObject loading(JsonObject params);
JsonObject changeDeviceID(JsonObject params);

bool deviceInit(void)
{
    PT.begin(9600);
    GetDeviceData();
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
    tiltAngle = PT.getTiltAngle();

    if (panAngle == 0 && tiltAngle == 0) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "与云台通讯异常";
        return error;
    }

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
    if (stepper.load()) {
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
    if (stepper.load()) {
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
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    JsonObject result = doc.to<JsonObject>();
    result["IP"] = WiFi.localIP().toString();
    result["edition"] = String(currentmajor) + "." + String(currentminor) + "." + String(currentpatch);
    result["deviceID"] = deviceID;
    return result;
}

JsonObject loading(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    uint32_t lastTime = millis();
    PT.control(12, 0, 0);       /* 调用0号预设点 */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        float panAngle;      /* 水平角度 */
        float tiltAngle;        /* 垂直角度 */
        panAngle = PT.getPanAngle();
        tiltAngle = PT.getTiltAngle();
        if (panAngle > 64.0f && panAngle < 66.0f
            && (tiltAngle > 359.0f || tiltAngle < 1.0f)) {
            break;
        }
        if (millis() - lastTime > 20000) {  /* 如果超过20秒还没移动结束就报错 */
            JsonObject error = errorDoc.to<JsonObject>();
            error["error"]["message"] = "云台移动超时或与云台通讯异常";
            return error;
        }
    }
    if (stepper.load()) {
        JsonObject result = doc.to<JsonObject>();
        result["message"] = "loaded";
        return result;
    } else {
        JsonObject error = errorDoc.to<JsonObject>();
        // error["error"]["message"] = "Stepper motor loading failure";
        error["error"]["message"] = "无法控制步进电机装弹";
        return error;
    }
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
    pinMode(IgnitionPin, OUTPUT);
    // String message = "The ignition was successful";
    String message = "装弹成功";
    digitalWrite(IgnitionPin, LOW);
    vTaskDelay(1000);
    digitalWrite(IgnitionPin, HIGH);
    JsonObject result = doc.to<JsonObject>();
    result["message"] = message;
    result["deviceID"] = deviceID;
    return result;
}


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
    return 
}
*/