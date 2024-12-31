#include "PanTilt.h"


PanTilt::PanTilt(int rxPin, int txPin) : PanTiltSerial(rxPin, txPin)
{
    PTaddr = 0x01;          /* 云台地址 */
    sendInterval = 100;     /* 发送间隔100ms */
    maxSendNumber = 3;      /* 最大重复发送次数3次 */
}

bool PanTilt::begin(int baud)
{
    this->baud = baud;
    PanTiltSerial.begin(baud);
    mutex = xSemaphoreCreateMutex();
    return true;
}

bool PanTilt::control(byte command, byte param1, byte param2)
{
    int i, recvDataLen = 0, sendCount = 0, lastCommand = -1;
    uint32_t lastSendTime = 0;
    byte *receivedData = (byte *)malloc(sizeof(byte) * 8);
    byte *SendCommand = (byte *)malloc(sizeof(byte) * 8);
    String commandAttribute;

    // 获取锁
    if (xSemaphoreTake(mutex, 1000) != pdTRUE) {
        Serial.println("无法获取互斥锁");
        free(SendCommand);
        free(receivedData);
        return false;
    }

    if (command == 0x00 || command > 48) {
        goto controlFail;
    }

    getControlCommand(SendCommand, command, param1, param2);
    if (SendCommand[1] == 0x03) {           /* 查询命令 */
        commandAttribute = "inquire";
    } else if (SendCommand[1] == 0x06) {    /* 控制命令 */
        commandAttribute = "Controls";
    } else {                                /* 未定义命令 */
        commandAttribute = "undefine"; 
        Serial.println("未定义的命令");
        goto controlFail;
    }
    lastCommand = command;

    while (1) {
        if (millis() - lastSendTime > sendInterval) {
            if (sendCount >= maxSendNumber) {
                Serial.printf("设备无响应\n");
                goto controlFail;
            }
            printData("发送给云台的数据:", SendCommand, 8);
            PanTiltSerial.write(SendCommand, 8);    /* 发送控制命令数组 */
            lastSendTime = millis();
            sendCount++;
        }

        recvDataLen = readPanTiltData(receivedData);
        if (commandAttribute == "inquire" && recvDataLen == 7) {
            printData("收到云台返回查询命令", receivedData, recvDataLen);
            queryCommandParsing(receivedData, lastCommand);
            break;
        } else if (commandAttribute == "Controls" && recvDataLen == 8) {
            if (receivedData == nullptr) {
                Serial.println("是空的");
            }
            if (!comparisonCommand(SendCommand, receivedData, 8)) {
                Serial.printf("返回命令与发送命令不匹配");
            }
            Serial.printf("命令匹配\n");
            break;
        } else if (recvDataLen != 0) {
            Serial.printf("\n云台返回数据错误，recvDataLen = %d\n", recvDataLen);
            goto controlFail;
        }
        vTaskDelay(pdMS_TO_TICKS(10));  /* 释放CPU一段时间，避免IDLE任务无法喂狗 */
    }
    free(SendCommand);
    free(receivedData);
    xSemaphoreGive(mutex);  // 释放锁
    return true;
controlFail:
    free(SendCommand);
    free(receivedData);
    xSemaphoreGive(mutex);  // 释放锁
    return false;
}

float PanTilt::getPanAngle(void)
{
    if (control(22, 0, 0)) {
        return this->panAngle;
    } else {
        return NAN;
    }
}

float PanTilt::getTiltAngle(void)
{
    control(23, 0, 0);
    return this->tiltAngle;
}

void PanTilt::queryCommandParsing(byte *command, int lastCommand)
{
    switch (lastCommand)
    {
    case 22:        /* 水平角度查询 */
        panAngle = ((command[3] << 8) + command[4]) / 100.0f;
        Serial.printf("水平角度:%.2f\n", panAngle);
        break;
    case 23:        /* 垂直角度查询 */
        tiltAngle = ((command[3] << 8) + command[4]) / 100.0f;
        Serial.printf("垂直角度:%.2f\n", tiltAngle);
        break;
    case 43:        /* 查询云台地址 */
        Serial.printf("云台地址为%02X\n", command[4]);
        break;
    case 44:        /* 查询云台波特率 */
        switch (command[4])
        {
        case 0x00:
            baud = 2400;
            break;
        case 0x01:
            baud = 4800;
            break;
        case 0x02:
            baud = 9600;
            break;
        case 0x03:
            baud = 19200;
            break;
        case 0x04:
            baud = 384000;
            break;
        case 0x05:
            baud = 115200;
            break;
        default:
            Serial.printf("未定义的波特率:%d\n", command[4]);
            break;
        }
        Serial.printf("云台波特率为%d\n", baud);
        break;
    default:
        break;
    }
}

bool PanTilt::comparisonCommand(byte *command1, byte *command2, byte length)
{
    int i;
    for(i = 0; i < length; i++) {
        if (command1[i] != command2[i]) {
            Serial.printf("%d ? %d\n", command1[i], command2[i]);
            return false;
        }
    }
    return true;
}

void PanTilt::printData(String showString, byte *data, byte dataLen)
{
    int i;
    Serial.print(showString);
    for (i = 0; i < dataLen; i++)
    {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println("");
}

bool PanTilt::control(JsonObject params)
{
    byte command, param1 = 0, param2 = 0;
    if (params.containsKey("command")) {
        JsonVariant jsonValueCommand = params["command"];
        if (jsonValueCommand.is<String>()) {
            command = jsonValueCommand.as<String>().toInt();
        } else if (jsonValueCommand.is<int>()) {
            command = jsonValueCommand.as<int>();
        } else {
            return false;
        }
    } else {
        return false;
    }
    if (params.containsKey("param1")) {
        JsonVariant jsonValueparam1 = params["param1"];
        if (jsonValueparam1.is<String>()) {
            param1 = jsonValueparam1.as<String>().toInt();
        } else if (jsonValueparam1.is<int>()) {
            param1 = jsonValueparam1.as<int>();
        } else {
            return false;
        }
    }
    if (params.containsKey("param2")) {
        JsonVariant jsonValueparam2 = params["param2"];
        if (jsonValueparam2.is<String>()) {
            param2 = jsonValueparam2.as<String>().toInt();
        } else if (jsonValueparam2.is<int>()) {
            param2 = jsonValueparam2.as<int>();
        } else {
            return false;
        }
    }
    return control(command, param1, param2);
}

int PanTilt::readPanTiltData(byte *receivedData)
{
    if (PanTiltSerial.available()) {  /* 如果串口的缓冲区有数据 */
        byte recebuf[8] = {0};   /* 定义数据储存数组 */
        PanTiltSerial.readBytes(recebuf, 2);
        receivedData[0] = recebuf[0];
        receivedData[1] = recebuf[1];

        if (recebuf[1] == 0x06) {  /* 如果是控制命令 */
            PanTiltSerial.readBytes(&receivedData[2], 6);
            if (PanTiltSerial.available()) {
                byte temporaryBuffer[100];
                PanTiltSerial.readBytes(temporaryBuffer, PanTiltSerial.available());
            }
// printData("Received data:", receivedData, 8);
            if (crc16_modbus(receivedData, 8)) {
                return 8;
            } else {
                Serial.printf("云台返回数据CRC校验错误");
                return -1;
            }
        } else if (recebuf[1] == 0x03){
            PanTiltSerial.readBytes(&receivedData[2], 5);
            if (PanTiltSerial.available()) {
                byte temporaryBuffer[100];
                PanTiltSerial.readBytes(temporaryBuffer, PanTiltSerial.available());
            }
// printData("Received data:", receivedData, 7);
            if (crc16_modbus(receivedData, 7)) {
                return 7;
            } else {
                Serial.printf("云台返回数据CRC校验错误");
                return -1;
            }
        } else {        /* 如果都不是则发生错误，读取缓冲区所有数据并返回-1 */
            if (PanTiltSerial.available()) {
                byte temporaryBuffer[100];
                PanTiltSerial.readBytes(temporaryBuffer, PanTiltSerial.available());
            }
            Serial.print("\n错误数据");
            while (PanTiltSerial.available()) {
                char c = PanTiltSerial.read();
                Serial.printf("%d ", (int)c);
            }
            Serial.println("");
            return -1;
        }
    }
    return 0;
}

bool PanTilt::setLoading(void)
{
    if (control(11, 0, 0xFF)) {
        if (control(12, 0, 0)) {
            Serial.println("到达状态位置");
            vTaskDelay(pdMS_TO_TICKS(1000));
            Serial.println("装弹成功");
        }
    }
    return false;
}

/********************************************************
 * Function name     :   getControlCommand
 * Description       :   根据operand（操作数）返回最终发送的控制指令数组
 * Parameter         :
 * @operand          :   操作数
 * @param1           :   参数1（默认为0）
 * @param2           :   参数1（默认为0）
 * Return            :   最终发送的控制指令数组
**********************************************************/
bool PanTilt::getControlCommand(byte *cmd, byte operand, byte param1, byte param2)
{
    int i = 0;
    byte commands[][8] = {  {},
        {0x00, 0x06, 0x00, 0x07, 0x00, 0x77, 0x00, 0x00},     /* 1: 调用自检 */
        {0x00, 0x06, 0x00, 0x03, 0x00, 0x77, 0x00, 0x00},     /* 2: 设置上电自检 */
        {0x00, 0x06, 0x00, 0x05, 0x00, 0x77, 0x00, 0x00},     /* 3: 删除上电自检 */
        {0x00, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00},     /* 4: 水平向右控制( 参数1 为设置速度) */
        {0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00},     /* 5: 水平向左控制( 参数1 为设置速度) */
        {0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00},     /* 6: 俯仰向上控制( 参数1 为设置速度) */
        {0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00},     /* 7: 俯仰向下控制( 参数1 为设置速度) */
        {0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},     /* 8: 停止功能 */
        {0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00},     /* 9: 水平停止 */
        {0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00},     /* 10: 俯仰停止 */
        {0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00},     /* 11: 普通预置点设置( 参数2 为设置的预置点号) */
        {0x00, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00},     /* 12: 普通预置点调用( 参数2 为设置的预置点号) */
        {0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00},     /* 13: 普通预置点删除( 参数2 为设置的预置点号) */
        {0x00, 0x06, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00},     /* 14: 线扫起点设置 */
        {0x00, 0x06, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00},     /* 15: 线扫终点设置 */
        {0x00, 0x06, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x00},     /* 16: 打开线扫功能 */
        {0x00, 0x06, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x00},     /* 17: 关闭线扫功能 */
        {0x00, 0x06, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00},     /* 18: 恢复出厂设置 */
        {0x00, 0x06, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00},     /* 19: 扫描速度设置( 参数1 为PS, 参数2 为TS) */
        {0x00, 0x06, 0x00, 0x4B, 0x00, 0x00, 0x00, 0x00},     /* 20: 云台水平角度设置( 参数1 为MSB, 参数2 为LSB) */
        {0x00, 0x06, 0x00, 0x4D, 0x00, 0x00, 0x00, 0x00},     /* 21: 云台俯仰角度设置( 参数1 为MSB, 参数2 为LSB) */
        {0x00, 0x03, 0x00, 0x51, 0x00, 0x01, 0x00, 0x00},     /* 22：云台水平角度查询 */
        {0x00, 0x03, 0x00, 0x53, 0x00, 0x01, 0x00, 0x00},     /* 23：云台俯仰角度查询 */
        {0x00, 0x06, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x00},     /* 24：云台自动归位时间( 参数2 为设置的归位时间（分钟）) */
        {0x00, 0x06, 0x00, 0x07, 0x00, 0x70, 0x00, 0x00},     /* 25：自动归位开启 */
        {0x00, 0x06, 0x00, 0x07, 0x00, 0x71, 0x00, 0x00},     /* 26：自动归位关闭 */
        {0x00, 0x06, 0x00, 0x03, 0x00, 0x75, 0x00, 0x00},     /* 27：自动归位调用一号预置 */
        {0x00, 0x06, 0x00, 0x05, 0x00, 0x75, 0x00, 0x00},     /* 28：自动归位调用巡航 */
        {0x00, 0x06, 0x00, 0x07, 0x00, 0x75, 0x00, 0x00},     /* 29：自动归位调用线扫 */
        {0x00, 0x06, 0x00, 0x67, 0x00, 0x00, 0x00, 0x00},     /* 30：设置预置位巡航速度 */
        {0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00},     /* 31：零点设置 */
        {0x00, 0x06, 0x00, 0x07, 0x00, 0x5B, 0x00, 0x00},     /* 32：参数恢复出厂设置 */
        {0x00, 0x06, 0x00, 0x05, 0x00, 0x61, 0x00, 0x00},     /* 33：减小预置位速度 */
        {0x00, 0x06, 0x00, 0x03, 0x00, 0x61, 0x00, 0x00},     /* 34：增大预置位速度 */
        {0x00, 0x06, 0x00, 0x03, 0x00, 0x62, 0x00, 0x00},     /* 35：增大扫描速度 */
        {0x00, 0x06, 0x00, 0x03, 0x00, 0x62, 0x00, 0x00},     /* 36：减小扫描速度 */
        {0x00, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00},     /* 37：巡航功能开启( 参数2 为Number) */
        {0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00},     /* 38：巡航时间间隔设置( 参数2 为time1) */
        {0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00},     /* 39：自动归位时间间隔设置( 参数2 为time2) */
        {/*0x00, 0x06, 0x00, 0x07, 0x00, 0x78, 0x00, 0x00*/},     /* 40：远程重启 */
        {0x00, 0x06, 0x00, 0x93, 0x00, 0x00, 0x00, 0x00},     /* 41：设置云台地址( 参数2 为设置的地址) */
        {0x00, 0x06, 0x00, 0x95, 0x00, 0x00, 0x00, 0x00},     /* 42：设置云台波特率( 参数2 为设置的波特率) */
        {0x00, 0x03, 0x00, 0xA1, 0x00, 0x01, 0x00, 0x00},     /* 43：查询云台地址 */
        {0x00, 0x03, 0x00, 0xA3, 0x00, 0x01, 0x00, 0x00},     /* 44：查询云台波特率 */
        {0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00},     /* 45：地址回传格式( 参数2 为地址) */
        {0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00},     /* 46：地址回传格式( 参数2 为波特率) */
        {0x00, 0x06, 0x00, 0x97, 0x00, 0x0A, 0x00, 0x00},     /* 47：打开实时回传 */
        {0x00, 0x06, 0x00, 0x99, 0x00, 0x0A, 0x00, 0x00},     /* 48：关闭实时回传 */
    };
    if (cmd == nullptr)
        cmd = (byte *)malloc(sizeof(byte) * 8);             /* cmd为最终发送的控制指令 */
    const byte cmdLen = 8;

    for(i = 0; i < cmdLen; i++) {
        cmd[i] = commands[operand][i];
    }
    cmd[0] = PTaddr;                    /* 设置云台地址 */
    if (cmd[4] == 0 && param1 != 0)      /* 如果原操作指令参数1为0，且传入参数1不为0，则代表参数1为变量 */
    {
        cmd[4] = param1;
    }
    if (cmd[5] == 0 && param2 != 0)      /* 如果原操作指令参数2为0，且传入参数2不为0，则代表参数2为变量 */
    {
        cmd[5] = param2;
    }
    crc16_modbus(cmd, cmdLen);  /* 添加冗余校验 */
    return true;
}

/********************************************************
 * Function name     :   crc16_modbus
 * Description       :   进行CRC校验，并将校验结果放入输入数组的后两位
 *                       使用时请务必保证传入数组的大小至少有两位的冗余
 *                       如果输入数组后两位不为0，则代表进行CRC检验，检验成功返回1，不成功返回0
 * Parameter         :
 * @data             :   存放发送数据的数组地址
 * @length           :   发送数据数组的长度（包含两位冗余位）
 * Return            :   true --检验成功  ,  false -- 检验失败
**********************************************************/
bool PanTilt::crc16_modbus(byte *data, byte length)
{
    int detection = 0;
    if ((data[length-2] == 0) && (data[length-1] == 0))
    {
        detection = 1;
        length -= 2;
    }
    uint16_t uCRC = 0xFFFF;             /* 初始值为0xFFFF */
    for(byte num = 0; num < length; num++)
    {
        uCRC = (data[num]) ^ uCRC;      /* 把数据与16位的CRC寄存器的低8位相异或，结果存放于CRC寄存器。 */
        for(byte x = 0; x < 8; x++)  /* 循环8次 */
        {
            if(uCRC & 0x0001)           /*判断最低位为：“1” */
            {
                uCRC = uCRC >> 1;	    /* 先右移 */
                uCRC = uCRC ^ 0xA001;   /* 再与0xA001异或 */
            }
            else                        /* 判断最低位为：“0” */
            {
                uCRC = uCRC >> 1 ;	    /* 右移 */
            }
        }
    }
    if (detection) {
        data[length] = uCRC & 0x00FF;            /* 取出CRC校验结果的低八位 */
        data[length+1] = (uCRC & 0xFF00) >> 8;     /* 取出CRC校验结果的高八位 */
    }

    return uCRC == 0;
}