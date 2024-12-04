#ifndef __JSONRPC_H__
#define __JSONRPC_H__

#include <ArduinoJson.h>
#include "mqtt.h"
#include "deviceData.h"

typedef JsonObject (*jrpcFunction)(JsonObject params);

struct jrpcProcedure {
	char *name;
	jrpcFunction function;
	void *data;
};



class jsonrpc {
public:
    jsonrpc(void);
    bool jsonParse(DynamicJsonDocument& doc, String source);
    bool registerProcedure(jrpcFunction functionPointer, String name, void *data = nullptr);
    bool sendError(String message, String source, int id = 0, int code = -1);
private:
    int procedureCount;        /* 注册的程序个数计数 */
    struct jrpcProcedure *procedures;  /* 注册的程序的指针 */

    bool sendResult(JsonObject result, String source, int id = 0);
};
bool mqttSendResponse(String topic, JsonObject response);

extern jsonrpc rpc;

#endif // !__JSONRPC_H__
