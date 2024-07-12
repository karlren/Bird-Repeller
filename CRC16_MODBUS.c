/*
 * Created by Karlren on 2024-07-12.
 */

#include "CRC16_MODBUS.h"

const uint8_t g_ADDR = 0x01;    /* 设备的地址 */

/* 存放所有命令的二维数组，通过命令的下标进行寻址,空白{}为占位数组 */
uint8_t g_command[][8] = {
        {g_ADDR, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},     // 0：运动停止
        {g_ADDR, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},     // 1：运动停止
        {g_ADDR, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},     // 2：水平运动停止
        {g_ADDR, 0x06, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00},     // 3：俯仰运动停止
        {g_ADDR, 0x06, 0x00, 0x07, 0x00, 0x77, 0x00, 0x00},     // 4：调用自检
//        {g_ADDR, 0x06, 0x00, 0x03, 0x00, 0x77, 0x00, 0x00},     // 5：设置上电自检
//        {g_ADDR, 0x06, 0x00, 0x05, 0x00, 0x77, 0x00, 0x00},     // 6：删除上电自检
        {},{},{},{},{},
        {g_ADDR, 0x06, 0x00, 0x02, 0x00, 0x40, 0x00, 0x00},     // 10：水平向右控制（[4]为速度）
        {g_ADDR, 0x06, 0x00, 0x04, 0x00, 0x40, 0x00, 0x00},     // 11：水平向左控制（[4]为速度）
        {g_ADDR, 0x06, 0x00, 0x08, 0x00, 0x40, 0x00, 0x00},     // 12：俯仰向上控制（[4]为速度）
        {g_ADDR, 0x06, 0x00, 0x08, 0x00, 0x40, 0x00, 0x00},     // 13：俯仰向下控制（[4]为速度）
        {},{},{},
        {g_ADDR, 0x03, 0x00, 0x51, 0x00, 0x01, 0x00, 0x00},     // 20：查询水平角度
        {g_ADDR, 0x03, 0x00, 0x53, 0x00, 0x01, 0x00, 0x00},     // 21：查询俯仰角度
        {g_ADDR, 0x03, 0x00, 0xA1, 0x00, 0x01, 0x00, 0x00},     // 22：查询云台地址
        {g_ADDR, 0x03, 0x00, 0xA3, 0x00, 0x01, 0x00, 0x00},     // 23：查询云台波特率
};

/********************************************************
 * Function name     :   crc16_modbus
 * Description       :   进行CRC校验，并将校验结果放入输入数组的后两位
 *                       使用时请务必保证传入数组的大小至少有两位的冗余
 *                       如果输入数组后两位不为0，则代表进行CRC检验，检验成功返回1，不成功返回0
 * Parameter         :
 * @data             :   存放发送数据的数组地址
 * @length           :   发送数据数组的长度（包含两位冗余位）
 * Return            :   1 --检验成功  ,  0 -- 检验失败
**********************************************************/
int crc16_modbus(uint8_t *data, uint8_t length)
{
    if ((data[length-2] == 0) && (data[length-1] == 0))
    {
//        printf("CRC detection!\n");
        length -= 2;
    }
    uint16_t uCRC = 0xFFFF;             /* 初始值为0xFFFF */
    for(uint8_t num = 0; num < length; num++)
    {
        uCRC = (data[num]) ^ uCRC;      /* 把数据与16位的CRC寄存器的低8位相异或，结果存放于CRC寄存器。 */
        for(uint8_t x = 0; x < 8; x++)  /* 循环8次 */
        {
            if(uCRC & 0x0001)           /*判断最低位为：“1” */
            {
                uCRC = uCRC >> 1;	    /* 先右移 */
                uCRC = uCRC ^ 0xA001;   /* 再与0xA001异或 */
            }
            else                        /* 判断最低位为：“0” */
            {
                uCRC = uCRC >>1 ;	    /* 右移 */
            }
        }
    }
    data[6] = uCRC & 0x00FF;            /* 取出CRC校验结果的低八位 */
    data[7] = (uCRC & 0xFF00) >> 8;     /* 取出CRC校验结果的高八位 */
    return uCRC == 0;
}
/********************************************************
 * Function name     :   Command_Copy
 * Description       :   将source数组中的内容复制到destination中
 * Parameter         :
 * @destination      :   被复制的数组地址
 * @source           :   复制的源数组地址
 * Return            :   NULL
**********************************************************/
void Command_Copy(uint8_t *destination, uint8_t *source)
{
    for (int i = 0; i < 8; ++i) {
        destination[i] = source[i];
    }
}
/********************************************************
 * Function name     :   Control
 * Description       :   根据operand（操作数）返回最终发送的控制指令数组
 * Parameter         :
 * @destination      :   被复制的数组地址
 * @source           :   复制的源数组地址
 * Return            :   NULL
**********************************************************/
uint8_t* Control(uint8_t operand, uint8_t data1, uint8_t data2)
{
    uint8_t *cmd = malloc(sizeof(uint8_t) * 8);             /* cmd为最终发送的控制指令 */
    const uint8_t cmdLen = 8;

    Command_Copy(cmd, g_command[operand]);      /* 将对应的操作指令取出 */
    if (cmd[4] == 0 && data1 != 0)       /* 如果原操作指令参数1为0，且传入参数1不为0，则代表参数1为变量 */
    {
        cmd[4] = data1;
    }
    if (cmd[5] == 0 && data2 != 0)      /* 如果原操作指令参数2为0，且传入参数2不为0，则代表参数2为变量 */
    {
        cmd[5] = data2;
    }
    crc16_modbus(cmd, cmdLen);  /* 添加冗余校验 */
    return cmd;
}