#include "nrf24l01.h"

//----------------------寄存器地址----------------------
//只能在 Shutdown、Standby 和 Idle-TX 模式下才能对寄存器进行配置

#define NRF24L01_CONFIG_REG 0x00           //配置寄存器
#define NRF24L01_CONFIG_IT_RX_EN 0x00      //开启接收中断
#define NRF24L01_CONFIG_IT_RX_DIS 0x40     //关闭接收中EN断
#define NRF24L01_CONFIG_IT_TX_EN 0x00      //开启发送中断
#define NRF24L01_CONFIG_IT_TX_DIS 0x20     //关闭发送中断
#define NRF24L01_CONFIG_IT_MAX_RT_EN 0x00  //开启最大重发计数中断
#define NRF24L01_CONFIG_IT_MAX_RT_DIS 0x10 //关闭最大重发计数中断
#define NRF24L01_CONFIG_CRC_EN 0x08
#define NRF24L01_CONFIG_CRC_LEN_1BYTE 0x00
#define NRF24L01_CONFIG_CRC_LEN_2BYTE 0x04
#define NRF24L01_CONFIG_PWR_EN 0x02
#define NRF24L01_CONFIG_MODE_TX 0x00
#define NRF24L01_CONFIG_MODE_RX 0x01

#define NRF24L01_CONFIG_MODE_MASK 0x01

//--------------------------------------

#define NRF24L01_AUTO_ACK_REG 0x01 // 使能自动ACK, 位[0:5], Enable: 1, Disable: 0

//----------------------------------------

#define NRF24L01_RX_PIPE_EN_REG 0x02 // 使能接收管道, 位[0:5], Enable: 1, Disable: 0

//------------------------------------------

#define NRF24L01_ADDR_WIDTH_REG 0x03 // 地址宽度配置
#define NRF24L01_ADDR_WIDTH_3BYTE 0x01
#define NRF24L01_ADDR_WIDTH_4BYTE 0x02
#define NRF24L01_ADDR_WIDTH_5BYTE 0x03

//---------------------------------------------

/**
 * 重发延迟和次数配置
 * 
 * 位[7:4]: 重发延时, (250 * (x+1))us,  x = 0~15
 * 位[3:0]: 重发次数, x 次,  x = 0~15
*/
#define NRF24L01_RETRY_CONFIG_REG 0x04

//----------------------------------------------

#define NRF24L01_RF_CHANNAL_REG 0x05 // 信道频段, 2400MHz + x, x: 最大值 125

#define NRF24L01_RF_CONFIG_REG 0x06
#define NRF24L01_RF_CONFIG_CONST_WAVE_EN 0x80 // 恒载波发射模式
#define NRF24L01_RF_CONFIG_SPEED_1Mbps 0x00   // 数据传输速度 1Mbps
#define NRF24L01_RF_CONFIG_SPEED_2Mbps 0x08
#define NRF24L01_RF_CONFIG_SPEED_250Kbps 0x20
#define NRF24L01_RF_CONFIG_PWR_7dBm 0x07
#define NRF24L01_RF_CONFIG_PWR_4dBm 0x06
#define NRF24L01_RF_CONFIG_PWR_3dBm 0x05
#define NRF24L01_RF_CONFIG_PWR_1dBm 0x04
#define NRF24L01_RF_CONFIG_PWR_0dBm 0x03

//---------------------------------------------

#define NRF24L01_STATUS_REG 0x07 // 状态寄存器
#define NRF24L01_STATUS_RX_DAT_READY 0x40
#define NRF24L01_STATUS_TX_SEND_DONE 0x20
#define NRF24L01_STATUS_TX_MAX_RETRY 0x10
#define NRF24L01_STATUS_TX_SEND_DONE_OR_FAILED_MASK 0x30
#define NRF24L01_STATUS_RX_PIPE_NUMBER 0x0E
#define NRF24L01_STATUS_TX_FIFO_FULL 0x01

//-------------------------------------

// 信号强度检测，小于 -60dBm 为 0, 否则为 1
#define NRF24L01_SIGNAL_STRENGTH_REG 0x09

//----------------------------------------------------

//接收管道x的接收地址
#define NRF24L01_RX_PIPEx_ADDR_REG(x) (0x0A + (x))

//接收管道x的接收宽度
#define NRF24L01_RX_PIPEx_WIDTH_REG(x) (0x11 + (x))

//发送的地址
#define NRF24L01_TX_ADDR_REG 0x10

//---------------------------------------------

#define NRF24L01_FIFO_STATUS_REG 0x17 //先入先出队列的状态寄存器
#define NRF24L01_FIFO_STATUS_TX_FULL 0x20
#define NRF24L01_FIFO_STATUS_TX_EMPTY 0x10
#define NRF24L01_FIFO_STATUS_RX_FULL 0x02
#define NRF24L01_FIFO_STATUS_RX_EMPTY 0x01

// ===============================================================

#define _WR_OFFSET 0x20

#define _CMD_FLUSH_TX 0xE1
#define _CMD_FLUSH_RX 0xE2
#define _CMD_NOP 0xFF

#define _FIFO_WRITE_ADDR 0xA0
#define _FIFO_READ_ADDR 0x61

#define _ADDR_PREFIX 0xE7E70000U

NRF24L01_WriteByteCallBk _SPI_WriteByte;

// ======================== internal function ======================

__STATIC_INLINE uint8_t _WriteCmd(uint8_t cmd)
{
    uint8_t res;
    NRF24L01_CS_LOW();
    res = _SPI_WriteByte(cmd);
    NRF24L01_CS_HIGH();
    return res;
}

__STATIC_INLINE void _WriteReg(uint8_t addr, uint8_t dat)
{
    NRF24L01_CS_LOW();
    _SPI_WriteByte(_WR_OFFSET | addr);
    _SPI_WriteByte(dat);
    NRF24L01_CS_HIGH();
}

__STATIC_INLINE uint8_t _ReadReg(uint8_t addr)
{
    uint8_t dat;
    NRF24L01_CS_LOW();
    _SPI_WriteByte(addr);
    dat = _SPI_WriteByte(0);
    NRF24L01_CS_HIGH();
    return dat;
}

//==================================================

uint8_t NRF24L01_Init(NRF24L01_InitTypeDef *configInfo)
{
    uint8_t i, tmp;

    _SPI_WriteByte = configInfo->writeDataCallBk;

    NRF24L01_CS_HIGH(); // disable spi chip select
    NRF24L01_EN_LOW();  // disable nrf24l01

#ifdef NRF24L01_USE_IT
    tmp = NRF24L01_CONFIG_IT_RX_EN | NRF24L01_CONFIG_IT_TX_EN | NRF24L01_CONFIG_IT_MAX_RT_EN |
          NRF24L01_CONFIG_CRC_LEN_2BYTE | NRF24L01_CONFIG_CRC_EN |
          NRF24L01_CONFIG_PWR_EN; // 开启中断, 开启 16位 CRC, 电源, 进入待机模式
#else
    tmp = NRF24L01_CONFIG_IT_RX_DIS | NRF24L01_CONFIG_IT_TX_DIS | NRF24L01_CONFIG_IT_MAX_RT_DIS |
          NRF24L01_CONFIG_CRC_LEN_2BYTE | NRF24L01_CONFIG_CRC_EN |
          NRF24L01_CONFIG_PWR_EN; // 关闭中断, 开启 16位 CRC, 电源, 进入待机模式
#endif

    _WriteReg(NRF24L01_CONFIG_REG, tmp);                           // config nrf24l01
    _WriteReg(NRF24L01_ADDR_WIDTH_REG, NRF24L01_ADDR_WIDTH_4BYTE); // 地址宽度 4 byte
    _WriteReg(NRF24L01_AUTO_ACK_REG, 0x00);                        // 关闭所有自动应答
    _WriteReg(NRF24L01_RX_PIPE_EN_REG, 0x00);                      // 关闭所有管道
    _WriteReg(NRF24L01_RETRY_CONFIG_REG, 0x02);                    // 重发延迟 250us, 2 次, 500us
    _WriteReg(NRF24L01_RF_CHANNAL_REG, configInfo->RF_ChannalOffset);
    _WriteReg(NRF24L01_RF_CONFIG_REG, configInfo->transferSpeed | configInfo->transferPower);

    // set data width
    for (i = 0; i < 6; i++)
        _WriteReg(NRF24L01_RX_PIPEx_WIDTH_REG(i), 32);

    // reset pipe 1 addr to 0xE7E7E7E7E7
    NRF24L01_CS_LOW();
    _SPI_WriteByte(_WR_OFFSET + NRF24L01_RX_PIPEx_ADDR_REG(1));
    for (i = 0; i < 5; i++)
        _SPI_WriteByte(0xE7);
    NRF24L01_CS_HIGH();

    // clear all
    _WriteCmd(_CMD_FLUSH_RX);
    _WriteCmd(_CMD_FLUSH_TX);

    if (_ReadReg(NRF24L01_CONFIG_REG) == tmp) // check if config done !
        return NRF24L01_CODE_DONE;
    else
        return NRF24L01_CODE_FAILED;
}

void NRF24L01_Rx_SetPipeAddr(uint8_t pipe_x, uint16_t _addr)
{
    uint8_t i;
    uint32_t addr = _ADDR_PREFIX | _addr;

    if (pipe_x < 2)
    {
        NRF24L01_CS_LOW();
        _SPI_WriteByte(_WR_OFFSET + NRF24L01_RX_PIPEx_ADDR_REG(pipe_x));
        for (i = 0; i < 4; i++)
        {
            _SPI_WriteByte((uint8_t)addr);
            addr >>= 8;
        }
        NRF24L01_CS_HIGH();
    }
    else
    {
        _WriteReg(NRF24L01_RX_PIPEx_ADDR_REG(pipe_x), (uint8_t)_addr);
    }
}

void NRF24L01_Rx_PipeCmd(uint8_t pipe_x, uint8_t state)
{
    uint8_t oldConfig;

#ifdef NRF24L01_USE_AUTO_REPLY
    oldConfig = _ReadReg(NRF24L01_AUTO_ACK_REG);
    oldConfig = state ? ((1 << pipe_x) | oldConfig) : ((~(1 << pipe_x)) & oldConfig);
    _WriteReg(NRF24L01_AUTO_ACK_REG, oldConfig);
#endif // NRF24L01_USE_AUTO_REPLY

    oldConfig = _ReadReg(NRF24L01_RX_PIPE_EN_REG);
    oldConfig = state ? ((1 << pipe_x) | oldConfig) : ((~(1 << pipe_x)) & oldConfig);
    _WriteReg(NRF24L01_RX_PIPE_EN_REG, oldConfig);
}

void NRF24L01_Tx_SetTargetAddr(uint16_t _addr)
{
    uint8_t i;
    uint32_t addr = _ADDR_PREFIX | _addr;

    NRF24L01_CS_LOW();
    _SPI_WriteByte(_WR_OFFSET + NRF24L01_TX_ADDR_REG);
    for (i = 0; i < 4; i++)
    {
        _SPI_WriteByte((uint8_t)addr);
        addr >>= 8;
    }
    NRF24L01_CS_HIGH();
}

void NRF24L01_SwitchMode(uint8_t _mode, uint16_t addr)
{
    uint8_t oldMode;

    NRF24L01_EN_LOW(); // disable nrf24l01

    oldMode = _ReadReg(NRF24L01_CONFIG_REG);

#ifdef NRF24L01_USE_AUTO_REPLY
    // set pipe_0 addr == self rx addr to receive reply signal
    NRF24L01_Rx_SetPipeAddr(0, addr);
    NRF24L01_Rx_PipeCmd(0, 1);
#endif // !NRF24L01_USE_AUTO_REPLY

    // if old mode == require mode, only refresh addr
    if ((oldMode & NRF24L01_CONFIG_MODE_MASK) == _mode)
    {
        if (_mode == NRF24L01_CONFIG_MODE_TX)
        {
            NRF24L01_Tx_SetTargetAddr(addr);
        }
        return;
    }

    if (_mode == NRF24L01_CONFIG_MODE_TX)
    {
#ifndef NRF24L01_USE_AUTO_REPLY
        NRF24L01_Rx_PipeCmd(0, 0);
#endif
        NRF24L01_Tx_SetTargetAddr(addr);
        _WriteReg(NRF24L01_CONFIG_REG, oldMode & 0xFE); // switch to TX mode
    }
    else
    {
#ifndef NRF24L01_USE_AUTO_REPLY
        NRF24L01_Rx_PipeCmd(0, 1);
#endif
        _WriteReg(NRF24L01_CONFIG_REG, oldMode | NRF24L01_CONFIG_MODE_RX); // switch to RX mode
        _WriteCmd(_CMD_FLUSH_RX);
        NRF24L01_EN_HIGH(); // receive mode, enable nrf24l01
    }
}

uint8_t NRF24L01_SendPacket(NRF24L01_Buffer buffer)
{
    uint8_t status;
    uint16_t timeout = 0;

    // put data and send
    NRF24L01_EN_LOW();
    NRF24L01_CS_LOW();
    _SPI_WriteByte(_FIFO_WRITE_ADDR);
    for (status = 0; status < NRF24L01_BUF_SIZE; status++)
        _SPI_WriteByte(buffer[status]);
    NRF24L01_CS_HIGH();
    NRF24L01_EN_HIGH();

#ifdef NRF24L01_USE_IT
    // wait send interrupt
    while (NRF24L01_Check_IT_Flag() == 0)
#else
    // wait send flag
    while ((_WriteCmd(_CMD_NOP) & NRF24L01_STATUS_TX_SEND_DONE_OR_FAILED_MASK) == 0)
#endif
    {
        if (++timeout >= NRF24L01_MAX_TIMEOUT)
        {
            NRF24L01_EN_LOW();                                   // disable nrf24l01
            _WriteReg(NRF24L01_STATUS_REG, _WriteCmd(_CMD_NOP)); // clear flag
            _WriteCmd(_CMD_FLUSH_TX);                            // clear tx fifo
            return NRF24L01_CODE_TIMEOUT;
        }
    }

    NRF24L01_EN_LOW(); // disable nrf24l01

    status = _WriteCmd(_CMD_NOP);
    _WriteReg(NRF24L01_STATUS_REG, status); // clear flag
    _WriteCmd(_CMD_FLUSH_TX);               // clear tx fifo

    if (status & NRF24L01_STATUS_TX_SEND_DONE)
        return NRF24L01_CODE_DONE;
    else
        return NRF24L01_CODE_FAILED;
}

int8_t NRF24L01_ReceivePacket(NRF24L01_Buffer buffer)
{
    int8_t pipex = -1;
    uint8_t status;

    status = _WriteCmd(_CMD_NOP);
    _WriteReg(NRF24L01_STATUS_REG, status); // clear IT flag

    if (status & NRF24L01_STATUS_RX_DAT_READY)
    {
        pipex = (status & NRF24L01_STATUS_RX_PIPE_NUMBER) >> 1;

        if (pipex < 6)
        {
            NRF24L01_CS_LOW();
            _SPI_WriteByte(_FIFO_READ_ADDR);
            for (status = 0; status < NRF24L01_BUF_SIZE; status++)
                buffer[status] = _SPI_WriteByte(0);
            NRF24L01_CS_HIGH();
        }
        else
            pipex = -1;

        _WriteCmd(_CMD_FLUSH_RX);
    }

    return pipex;
}