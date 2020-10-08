#ifndef _H_NRF24L01
#define _H_NRF24L01

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <nrf24l01_conf.h>

// SPI Chip Select
#ifndef NRF24L01_CS_HIGH
#error "NRF24L01_CS_HIGH() must be defined !"
#endif

#ifndef NRF24L01_CS_LOW
#error "NRF24L01_CS_LOW() must be defined !"
#endif

// Chip Enable
#ifndef NRF24L01_EN_HIGH
#error "NRF24L01_EN_HIGH() must be defined !"
#endif

#ifndef NRF24L01_EN_LOW
#error "NRF24L01_EN_LOW() must be defined !"
#endif

#ifdef NRF24L01_USE_IT
// Interrupt Pin
#ifndef NRF24L01_IsActive_IT
#error "NRF24L01_IsActive_IT() must be defined !"
#endif
#endif

// inline keyword
#ifndef __STATIC_INLINE
#define __STATIC_INLINE
#endif

// 数据传输速度
#define NRF24L01_SPEED_1Mbps 0x00
#define NRF24L01_SPEED_2Mbps 0x08
#define NRF24L01_SPEED_250Kbps 0x20

// 发射功率
#define NRF24L01_PWR_7dBm 0x07
#define NRF24L01_PWR_4dBm 0x06
#define NRF24L01_PWR_3dBm 0x05
#define NRF24L01_PWR_1dBm 0x04
#define NRF24L01_PWR_0dBm 0x03

// 工作模式
#define NRF24L01_MODE_TX 0x00
#define NRF24L01_MODE_RX 0x01

// --

typedef uint8_t (*NRF24L01_WriteByteCallBk)(uint8_t);

#define NRF24L01_BUF_SIZE 32 // must be 32. can't modify it
typedef uint8_t NRF24L01_Buffer[NRF24L01_BUF_SIZE];

typedef struct
{
    uint8_t RF_ChannalOffset; // 发射频段偏移, 0~125, 对应 2400MHz~2525MHz
    uint8_t transferSpeed;    // 传输速度, NRF24L01_SPEED_...
    uint8_t transferPower;    // 发射功率, NRF24L01_PWR_...
    NRF24L01_WriteByteCallBk writeDataCallBk;
} NRF24L01_InitTypeDef;

/**
 * Initialize NRF24L01
 * 
 * @param conf NRF24L01 init config
 * @return non-zero if init done, otherwise init failed
*/
uint8_t NRF24L01_Init(NRF24L01_InitTypeDef *conf);

/**
 * Switch NRF24L01 to TX or RX Mode
 * 
 * @param mode NRF24L01 mode, like: NRF24L01_MODE_TX
 * @param addr 16 bit address
*/
void NRF24L01_SwitchMode(uint8_t mode, uint16_t addr);

/**
 * Receive data, blocking mode if use interrupt, otherwise no-blocking mode
 * 
 * @param buffer data buffer, used to store the received data
 * @return channel id, -1 if receive failed, >= 0 if receive done
*/
int8_t NRF24L01_ReceiveData(NRF24L01_Buffer buffer);

/**
 * Send data with blocking mode
 * 
 * @param buffer data buffer to send
 * @return non-zero if send done, otherwise send failed
*/
uint8_t NRF24L01_SendData(NRF24L01_Buffer buffer);

#ifdef __cplusplus
}
#endif

#endif
