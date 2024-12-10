#ifndef __PANTILT_H__
#define __PANTILT_H__

#include <SoftwareSerial.h>
#include <ArduinoJson.h>

class PanTilt {
public:
    PanTilt(int rxPin, int txPin);
    bool begin(int baud = 9600);
    bool control(byte command, byte param1 = 0, byte param2 = 0);
    bool control(JsonObject params);
    float getPanAngle(void);
    float getTiltAngle(void);
    bool setLoading(void);
private:
    // 创建互斥锁句柄
    SemaphoreHandle_t mutex;
    int baud;
    int sendInterval;
    int maxSendNumber;
    float panAngle;                 /* 水平角度 */
    float tiltAngle;                /* 垂直角度 */
    SoftwareSerial PanTiltSerial;   // RX, TX
    byte PTaddr;
    int readPanTiltData(byte *receivedData);
    bool comparisonCommand(byte *command1, byte *command2, byte length);
    void queryCommandParsing(byte *command, int lastCommand);
    bool getControlCommand(byte *cmd, byte operand, byte param1, byte param2);
    bool crc16_modbus(byte *data, byte length);

    void printData(String showString, byte *data, byte dataLen);
};

#endif // !__PANTILT_H__