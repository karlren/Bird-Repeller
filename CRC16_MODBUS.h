//
// Created by Karlren on 2024-07-12.
//

#ifndef CRC_C_CRC16_MODBUS_H
#define CRC_C_CRC16_MODBUS_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int crc16_modbus(uint8_t *data, uint8_t length);
void Command_Copy(uint8_t *destination, uint8_t *source);
uint8_t* Control(uint8_t operand, uint8_t data1, uint8_t data2);

extern uint8_t g_command[][8];
#endif //CRC_C_CRC16_MODBUS_H
