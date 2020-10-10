#ifndef _H_NRF24L01
#define _H_NRF24L01

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <nrf24l01_conf.h>

//==================== user interface ====================

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

/**
 * Check if the interrupt has occurred. if true, must clear the interrupt flag
 * 
 * @return return non-zero if interrupt occurred, otherwise not occurred
*/
#ifndef NRF24L01_Check_IT_Flag
#error "NRF24L01_Check_IT_Flag() must be defined !"
#endif

#endif

//==================== global config ====================

// inline keyword
#ifndef __STATIC_INLINE
#define __STATIC_INLINE
#endif

// wait timeout
#ifndef NRF24L01_MAX_TIMEOUT
#define NRF24L01_MAX_TIMEOUT 600
#endif

// auto ack, default state is enable
#ifndef NRF24L01_DISABLE_ACK
#define NRF24L01_USE_ACK
#endif

// transmission packet size, range: 1 ~ 32
#ifndef NRF24L01_PACKET_SIZE
#define NRF24L01_PACKET_SIZE 32
#endif

//==================== exit code type ====================

#define NRF24L01_CODE_DONE 0
#define NRF24L01_CODE_FAILED 1
#define NRF24L01_CODE_TIMEOUT 2

//==================== init config options ====================

// transmission speed
#define NRF24L01_SPEED_1Mbps 0x00
#define NRF24L01_SPEED_2Mbps 0x08
#define NRF24L01_SPEED_250Kbps 0x20

// transmission power
#define NRF24L01_PWR_7dBm 0x07
#define NRF24L01_PWR_4dBm 0x06
#define NRF24L01_PWR_3dBm 0x05
#define NRF24L01_PWR_1dBm 0x04
#define NRF24L01_PWR_0dBm 0x03

// work mode
#define NRF24L01_MODE_TX 0x00
#define NRF24L01_MODE_RX 0x01

//==================== type define ====================

typedef uint8_t (*NRF24L01_WriteByteCallBk)(uint8_t);

typedef uint8_t NRF24L01_Buffer[NRF24L01_PACKET_SIZE];

typedef struct
{
    uint8_t channelOffset;    // frequency offset. range: 0~125 (2400MHz~2525MHz)
    uint8_t transferSpeed;    // transmission speed. ref: NRF24L01_SPEED_...
    uint8_t transferPower;    // transmission power. ref: NRF24L01_PWR_...
    uint8_t retryDelay;       // retransmission delay. range: 0~15, delay time: 250us * (retryDelay + 1)
    uint8_t retryTimes;       // retransmission times. range: 0~15 times
    uint16_t networkId;       // network ID. Used to identify different networks
    NRF24L01_WriteByteCallBk writeDataCallBk;
} NRF24L01_InitTypeDef;

/**
 * Initialize NRF24L01
 * 
 * @param conf NRF24L01 init config
 * @return NRF24L01_CODE_DONE if init done, otherwise init failed
*/
uint8_t NRF24L01_Init(NRF24L01_InitTypeDef *conf);

/**
 * receive signal strength detection, less than -60dbm is 0, otherwise it is 1
 * 
 * @return RSSI value
*/
uint8_t NRF24L01_GetRSSI(void);

/**
 * Switch NRF24L01 to TX or RX Mode
 * 
 * nrf24l01 address structure (5 bytes): [ADDR_FIXED_PREFIX: 1 byte] [networkId: 2 bytes] [addr: 2 bytes]
 * 
 *                              example: 0xE70F010F02, (ADDR_FIXED_PREFIX == 0xE7, networkId == 0x0F01, addr == 0x0F02)
 * 
 * @param mode NRF24L01 mode, like: NRF24L01_MODE_TX
 * @param addr target address(mode == NRF24L01_MODE_TX) or rx address(mode == NRF24L01_MODE_RX)
*/
void NRF24L01_SwitchMode(uint8_t mode, uint16_t addr);

/**
 * Receive a packet with no-blocking mode
 * 
 * @param buffer data buffer, used to store the received packet
 * @return channel id, -1 if receive failed, >= 0 if receive done
*/
int8_t NRF24L01_ReceivePacket(NRF24L01_Buffer buffer);

/**
 * Send a packet with blocking mode
 * 
 * @param buffer packet to send
 * @return NRF24L01_CODE_DONE if send done, otherwise send failed
*/
uint8_t NRF24L01_SendPacket(NRF24L01_Buffer buffer);

#ifdef __cplusplus
}
#endif

#endif
