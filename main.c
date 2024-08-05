#include <stdio.h>
#include "CRC16_MODBUS.h"
#include "Parse_data.h"


int main() {
    while (1)
    {
        int operand = 0, ret = 0;
        uint8_t *cmd = malloc(sizeof(uint8_t) * 8);
        uint8_t cmdLen = 8;
        unsigned int buf;
        printf("0、自行输入控制指令\n");
        printf("1、停止功能\n");
        printf("2、调用自检\n");
        printf("3、水平向右控制\n");
        printf("4、水平向左控制\n");
        printf("5、水平角度查询\n");
        printf("请输入操作数：");
        scanf("%d", &operand);
        switch (operand) {
            case 0:
                printf("请输入指令:\n");
                for (int i = 0; i < 6; ++i) {
                    scanf("%x", &buf);
                    cmd[i] = buf;
                }
                cmd = Control(operand, 0, 0);
                break;
            case 3:
            case 4:
                printf("输入速度(01-40H):");
                scanf("%02x", &buf);
                g_command[operand][4] = buf;
//            break;
            default:
                cmd = Control(operand, 0, 0);
        }
        for (int i = 0; i < 8; ++i) {
            printf("%02X ", cmd[i]);
        }
        printf("\n");

        char result[100];
        uint8_t revedata[8];
        if (cmd[1] == 0x06) {
            for (int i = 0; i < 8; ++i) {
                scanf("%2x", &revedata[i]);
            }
            if (crc16_modbus(revedata, 8)) {
                printf("发送控制命令成功\n");
            } else {
                printf("发送控制命令失败\n");
            }
        } else if (cmd[1] == 0x03) {
            for (int i = 0; i < 7; ++i) {
                scanf("%2x", &revedata[i]);
            }
            if (crc16_modbus(revedata, 8)) {
                printf("发送查询命令成功\n");
            } else {
                printf("发送查询命令失败\n");
            }
        }
        parse_data(revedata, operand, result);
        printf("%s", result);

    }

    return 0;
}
