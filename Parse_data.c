//
// Created by karlren on 2024-08-03.
//

#include "Parse_data.h"
#include "CRC16_MODBUS.h"

int g_baud[] = {2400, 4800, 9600, 19200, 38400, 115200};

int parse_data(uint8_t *data, uint8_t operand, char *result)
{
    /* 若没有分配空间则分配空间 */
    if (result == NULL) {
        result = malloc(sizeof(char) * 50);
    }
    /* 地址不正确 */
    if (data[0] != g_ADDR) {
        goto Fail;
    }
    if (data[1] == 0x06) {          /* 如果是控制命令 */
        if (crc16_modbus(data, 8)) {
            sprintf(result, "命令发送成功");
            return 1;
        } else {        /* CRC校验失败 */
            goto Fail;
        }
    } else if (data[1] == 0x03) {   /* 如果是查询命令 */
        if (crc16_modbus(data, 7)) {
            if (operand == 22) {
                float angle = (data[3] * 256.0f + data[4]) / 100.0f;
                sprintf(result, "云台水平角度为%.2f°\n", angle);
            } else if (operand == 23) {
                float angle = (data[3] * 256.0f + data[4]) / 100.0f;
                sprintf(result, "云台俯仰角度为%.2f°\n", angle);
            } else if (operand == 43) {
                sprintf(result, "云台地址为%2X\n", data[4]);
            } else if (operand == 44 ) {
                sprintf(result, "云台波特率为%d\n", g_baud[data[4]]);
            }
        } else {        /* CRC校验失败 */
            goto Fail;
        }
    } else {
        goto Fail;
    }
    return 1;
Fail:
    sprintf(result, "接收命令错误\n");
    return 0;
}