#ifndef _H_NF_01_S
#define _H_NF_01_S

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <nrf01s_conf.h>

// SPI Chip Select
#ifndef NF01S_CS_HIGH
#error "NF01S_CS_HIGH() must be defined !"
#endif

#ifndef NF01S_CS_LOW
#error "NF01S_CS_LOW() must be defined !"
#endif

// Chip Enable
#ifndef NF01S_EN_HIGH
#error "NF01S_EN_HIGH() must be defined !"
#endif

#ifndef NF01S_EN_LOW
#error "NF01S_EN_LOW() must be defined !"
#endif

// Interrupt Pin
#ifndef NF01S_IRQ_READ
#error "NF01S_IRQ_READ() must be defined !"
#endif

// inline keyword
#ifndef __STATIC_INLINE
#define __STATIC_INLINE
#endif

// 数据传输速度
#define NF01S_SPEED_1Mbps 0x00
#define NF01S_SPEED_2Mbps 0x08
#define NF01S_SPEED_250Kbps 0x20

// 发射功率
#define NF01S_PWR_7dBm 0x07
#define NF01S_PWR_4dBm 0x06
#define NF01S_PWR_3dBm 0x05
#define NF01S_PWR_1dBm 0x04
#define NF01S_PWR_0dBm 0x03

// 工作模式
#define NF01S_MODE_TX 0x00
#define NF01S_MODE_RX 0x01

// --

typedef uint8_t (*NF01S_WriteByteCallBk)(uint8_t);

#define NF01S_BUF_SIZE 32 // must be 32. can't modify it
typedef uint8_t NF01S_Buffer[NF01S_BUF_SIZE];

typedef struct
{
    uint8_t RF_ChannalOffset; // 发射频段偏移, 0~125, 对应 2400MHz~2525MHz
    uint8_t transferSpeed;    // 传输速度, NF01S_SPEED_...
    uint8_t transferPower;    // 发射功率, NF01S_PWR_...
    NF01S_WriteByteCallBk writeDataCallBk;
} NF01S_InitTypeDef;

void NF01S_Init(NF01S_InitTypeDef *configInfo);

void NF01S_SwitchMode(uint8_t mode, uint16_t addr);

int8_t NF01S_Rx_ReceiveData(NF01S_Buffer buffer);

uint8_t NF01S_Tx_SendData(NF01S_Buffer buffer);

#ifdef __cplusplus
}
#endif

#endif
