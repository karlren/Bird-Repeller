#include "jsonrpc.h"

jsonrpc rpc;

jsonrpc::jsonrpc(void)
{
    procedures = nullptr;
}

bool jsonrpc::jsonParse(DynamicJsonDocument& doc, String source)
{
    int i;
    int id = 0;
    JsonObject params = JsonObject(); // 创建一个空的JsonObject

    if (procedureCount == 0) {
        Serial.printf("未注册函数，收到来自%s的数据：\n", source.c_str());
        serializeJson(doc, Serial);
        Serial.println("");
        return false;
    } else {
        Serial.printf("收到来自%s的数据：", source.c_str());
        serializeJson(doc, Serial);
        Serial.println("");
    }

    if (doc.isNull() || doc.size() == 0) {
        return false;
    }
    if (doc.containsKey("id")) {
        id = doc["id"].as<int>();
        Serial.printf("id为%d\n", id);
    } else {
        Serial.printf("无任务ID数据\n");
    }
    /* 判断键deviceID是否存在 */
    if (!doc.containsKey("deviceID") && source == "mqtt") {
        // if (deviceID == 1){
            sendError("error : deviceID does not exist", source, id);
        // }
        return false;
    } else if (doc.containsKey("deviceID") && source == "mqtt") {
        JsonVariant devID = doc["deviceID"];
        /* 如果命令的控制设备号不为当前设备号，则不响应命令（0表示广播控制）*/
        if (devID.as<int>() != deviceID && devID.as<int>() != 0) {
            Serial.printf("当前设备号为%d，控制命令设备号为%d\n", deviceID, devID.as<int>());
            return true;
        }
    }
    if (doc.containsKey("params")) {
        params = doc["params"].as<JsonObject>(); /* 获取params */ 
    }
    /* 判断键method是否存在 */
    if (!doc.containsKey("method")) {
        sendError("error : method does not exist", source, id);
        return false;
    }
    /* 判断JSON中method属性的类型 */
    JsonVariant jsonVarmethod = doc["method"];
    if (jsonVarmethod.is<String>()) {
        String method = jsonVarmethod.as<String>();
        // Serial.printf("method = %s\n", method.c_str());
        for (i = 0; i < procedureCount; i++) {
            if (strcmp(procedures[i].name, method.c_str()) == 0) {
                JsonObject result = procedures[i].function(params);
                if (id != 0) {
                    result["id"] = id;
                }
                sendResult(result, source, id);
                break;
            }
            // Serial.printf("procedures[i].name = %s\n", procedures[i].name);
        }
        if (i >= procedureCount) {
            sendError("Undefined command", source, id);
            return false;
        }
    } else {
        sendError("Undefined command", source, id);
        return false;
    }
    return true;
}

bool jsonrpc::registerProcedure(jrpcFunction functionPointer, String name, void *data)
{
    int i = procedureCount++;
    if (procedures == nullptr) {
        procedures = (struct jrpcProcedure *)malloc(sizeof(struct jrpcProcedure));
        if (!procedureCount)
            return false;
    } else {
        /* realloc动态调整malloc分配的内存 */
        struct jrpcProcedure * ptr = (struct jrpcProcedure *)realloc(procedures, sizeof(struct jrpcProcedure) * procedureCount);
		if (!ptr)
			return false;
		procedures = ptr;
    }
    if ((procedures[i].name = strdup(name.c_str())) == nullptr) {
        return false;
    }
    procedures[i].function = functionPointer;
	procedures[i].data = data;
    return true;
}

bool jsonrpc::sendError(String message, String source, int id, int code)
{
    if (source == "serial") {
        Serial.printf("Error : %s\n", message.c_str());
    } else if (source == "mqtt") {
        DynamicJsonDocument resultDoc(1024);
        JsonObject resultJson = resultDoc.to<JsonObject>();
        resultJson["error"]["message"] = message;
        resultJson["error"]["code"] = code;
        resultJson["deviceID"] = deviceID;
        if (id != 0) {
            resultJson["id"] = id;
        }
        return mqttSendResponse("esp32/error", resultJson);
    } else {
        return false;
    }
    return true;
}

bool jsonrpc::sendResult(JsonObject result, String source, int id)
{
    if (result.containsKey("error")) {
        return sendError(result["error"]["message"].as<String>(), source);
    }

    if (source == "serial") {
        String resultMessage = result["message"];
        Serial.println(resultMessage);
    } else if (source == "mqtt") {
        return mqttSendResponse("esp32/result", result);
    }
}

bool mqttSendResponse(String topic, JsonObject response)
{
    size_t messageSize = measureJson(response) + 1; // +1 为了添加字符串结束符
    char message[messageSize];
    serializeJson(response, message, messageSize);
    return MQTT_Publish_Message(topic.c_str(), 2, 0, message);
}