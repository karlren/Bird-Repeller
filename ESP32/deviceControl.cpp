#include "deviceControl.h"
#include "MyStepper.h"
#include "jsonrpc.h"
#include "mqtt.h"

#define CIRCLE_Puls          6400         //电机旋转一圈的脉冲数

const int stepPin = 4;
const int dirPin = 5;
const int resetPin = 12;
const int shellDetPin = 13;

Preferences devicePreferences;

PanTilt PT(18, 19);
MyStepper stepper(stepPin, dirPin, resetPin, shellDetPin, CIRCLE_Puls);

void GetDeviceData(void);
JsonObject PanTiltControl(JsonObject params);
JsonObject getPTAngle(JsonObject params);
JsonObject Stepperload(JsonObject params);
JsonObject StepperHoming(JsonObject params);
JsonObject getDeviceData(JsonObject params);

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
        error["error"]["message"] = "Failed to control the PT";
        return error;
    }
}

JsonObject getPTAngle(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    String message = "Obtaining the PT Angle succeeded";
    float horizontalAngle;      /* 水平角度 */
    float verticalAngle;        /* 垂直角度 */

    horizontalAngle = PT.getHorizontalAngle();
    verticalAngle = PT.getVerticalAngle();

    if (horizontalAngle == 0 && verticalAngle == 0) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "Communication with PT failed";
        return error;
    }

    JsonObject result = doc.to<JsonObject>();

    result["message"] = message;
    result["method"] = "returnPTAngle";
    result["deviceID"] = deviceID;
    result["panAngle"] = horizontalAngle;
    result["tiltAngle"] = verticalAngle;

    return result;
}

JsonObject Stepperload(JsonObject params)
{
    static DynamicJsonDocument doc(128); 
    static DynamicJsonDocument errorDoc(128);
    if (stepper.load()) {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "Stepper motor loading failure";
        return error;
    }
    String message = "Stepper motor loading Successful";
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
        error["error"]["message"] = "Stepper motor loading failure";
        return error;
    }
    String message = "Stepper motor loading Successful";
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
    PT.control(12, 0, 0);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        float horizontalAngle;      /* 水平角度 */
        float verticalAngle;        /* 垂直角度 */
        horizontalAngle = PT.getHorizontalAngle();
        verticalAngle = PT.getVerticalAngle();
        if (horizontalAngle > 64.0f && horizontalAngle < 66.0f
            && (verticalAngle > 359.0f || verticalAngle < 1.0f)) {
            break;
        }
        if (millis() - lastTime > 20000) {  /* 如果超过20秒还没移动结束就报错 */
            JsonObject error = errorDoc.to<JsonObject>();
            error["error"]["message"] = "PT movement timed out";
            return error;
        }
    }
    if (stepper.load()) {
        JsonObject result = doc.to<JsonObject>();
        result["message"] = "loaded";
        return result;
    } else {
        JsonObject error = errorDoc.to<JsonObject>();
        error["error"]["message"] = "Stepper motor loading failure";
        return error;
    }
}
