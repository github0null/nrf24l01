#ifndef _H_NF_01_S
#define _H_NF_01_S

//====================== pin config =====================

#include "BITBAND.h"

// SPI Chip Select
#define NF01S_CS_Write(val) PBout(12) = (val)

// Chip Enable
#define NF01S_EN_Write(val) PAout(9) = (val)

// Interrupt Pin
#define NF01S_IRQ_Read() PAin(8)

//========================================================

#include "stdint.h"

//----------------------寄存器地址----------------------
//只能在 Shutdown、Standby 和 Idle-TX 模式下才能对寄存器进行配置

#define NF01S_CONFIG_REG 0x00           //配置寄存器
#define NF01S_CONFIG_IT_RX_EN 0x00      //开启接收中断
#define NF01S_CONFIG_IT_RX_DIS 0x40     //关闭接收中EN断
#define NF01S_CONFIG_IT_TX_EN 0x00      //开启发送中断
#define NF01S_CONFIG_IT_TX_DIS 0x20     //关闭发送中断
#define NF01S_CONFIG_IT_MAX_RT_EN 0x00  //开启最大重发计数中断
#define NF01S_CONFIG_IT_MAX_RT_DIS 0x10 //关闭最大重发计数中断
#define NF01S_CONFIG_CRC_EN 0x08
#define NF01S_CONFIG_CRC_LEN_1BYTE 0x00
#define NF01S_CONFIG_CRC_LEN_2BYTE 0x04
#define NF01S_CONFIG_PWR_EN 0x02
#define NF01S_CONFIG_MODE_TX 0x00
#define NF01S_CONFIG_MODE_RX 0x01

//--------------------------------------

#define NF01S_AUTO_ACK_REG 0x01 // 使能自动ACK, 位[0:5], Enable: 1, Disable: 0

//----------------------------------------

#define NF01S_RX_PIPE_EN_REG 0x02 // 使能接收管道, 位[0:5], Enable: 1, Disable: 0

//------------------------------------------

#define NF01S_ADDR_WIDTH_REG 0x03 // 地址宽度配置
#define NF01S_ADDR_WIDTH_3BYTE 0x01
#define NF01S_ADDR_WIDTH_4BYTE 0x02
#define NF01S_ADDR_WIDTH_5BYTE 0x03

//---------------------------------------------

/**
 * 重发延迟和次数配置
 * 
 * 位[7:4]: 重发延时, (250 * (x+1))us,  x = 0~15
 * 位[3:0]: 重发次数, x 次,  x = 0~15
*/
#define NF01S_RETRY_CONFIG_REG 0x04

//----------------------------------------------

#define NF01S_RF_CHANNAL_REG 0x05 // 信道频段, 2400MHz + x, x: 最大值 125

#define NF01S_RF_CONFIG_REG 0x06
#define NF01S_RF_CONFIG_CONST_WAVE_EN 0x80 // 恒载波发射模式
#define NF01S_RF_CONFIG_SPEED_1Mbps 0x00   // 数据传输速度 1Mbps
#define NF01S_RF_CONFIG_SPEED_2Mbps 0x08
#define NF01S_RF_CONFIG_SPEED_250Kbps 0x20
#define NF01S_RF_CONFIG_PWR_7dBm 0x07
#define NF01S_RF_CONFIG_PWR_4dBm 0x06
#define NF01S_RF_CONFIG_PWR_3dBm 0x05
#define NF01S_RF_CONFIG_PWR_1dBm 0x04
#define NF01S_RF_CONFIG_PWR_0dBm 0x03

//---------------------------------------------

#define NF01S_STATUS_REG 0x07 // 状态寄存器
#define NF01S_STATUS_DAT_READY 0x40
#define NF01S_STATUS_DAT_SEND 0x20
#define NF01S_STATUS_MAX_RETRY 0x10
#define NF01S_STATUS_RX_PIPE_NUMBER 0x0E
#define NF01S_STATUS_TX_FIFO_FULL 0x01

//-------------------------------------

// 信号强度检测，小于 -60dBm 为 0, 否则为 1
#define NF01S_SIGNAL_STRENGTH_REG 0x09

//----------------------------------------------------

//接收管道x的接收地址
#define NF01S_RX_PIPEx_ADDR_REG(x) (0x0A + (x))

//接收管道x的接收宽度
#define NF01S_RX_PIPEx_WIDTH_REG(x) (0x11 + (x))

//发送的地址
#define NF01S_TX_ADDR_REG 0x10

//---------------------------------------------

#define NF01S_FIFO_STATUS_REG 0x17 //先入先出队列的状态寄存器
#define NF01S_FIFO_STATUS_TX_FULL 0x20
#define NF01S_FIFO_STATUS_TX_EMPTY 0x10
#define NF01S_FIFO_STATUS_RX_FULL 0x02
#define NF01S_FIFO_STATUS_RX_EMPTY 0x01

//-----------------------------------------------

#define NF01S_BUF_SIZE 32

typedef uint8_t (*NF01S_WriteByteCallBk)(uint8_t);

typedef uint8_t NF01S_Buffer[NF01S_BUF_SIZE];

typedef struct
{
    uint8_t RF_ChannalOffset; // 发射频段偏移, 0~125, 对应 2400MHz~2525MHz
    uint8_t transferSpeed;    // 传输速度, NF01S_RF_CONFIG_SPEED_...
    uint8_t transferPower;    // 发射功率, NF01S_RF_CONFIG_PWR_...
    NF01S_WriteByteCallBk writeDataCallBk;
} NF01S_InitTypeDef;

void NF01S_Init(NF01S_InitTypeDef *configInfo);

void NF01S_SwitchMode(uint8_t mode, uint16_t addr);

int8_t NF01S_Rx_ReceiveData(NF01S_Buffer buffer);

uint8_t NF01S_Tx_SendData(NF01S_Buffer buffer);

#endif
