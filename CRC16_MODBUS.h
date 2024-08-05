//
// Created by Karlren on 2024-07-12.
//

#ifndef CRC_C_CRC16_MODBUS_H
#define CRC_C_CRC16_MODBUS_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
extern const uint8_t g_ADDR;    /* 设备的地址 */
extern uint8_t g_command[][8];

int crc16_modbus(uint8_t *data, uint8_t length);
void Command_Copy(uint8_t *destination, uint8_t *source);
uint8_t* Control(uint8_t operand, uint8_t data1, uint8_t data2);
uint8_t CRC_Check(uint8_t *re_val, uint8_t operand, char *result);
#endif //CRC_C_CRC16_MODBUS_H
