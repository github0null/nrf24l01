#include "nf01s.h"

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

// ===============================================================

#define _WR_OFFSET 0x20

#define _CMD_FLUSH_TX 0xE1
#define _CMD_FLUSH_RX 0xE2
#define _CMD_NOP 0xFF

#define _FIFO_WRITE_ADDR 0xA0
#define _FIFO_READ_ADDR 0x61

#define _ADDR_PREFIX 0xE7E70000U

NF01S_WriteByteCallBk _WriteDataCallBk;

// ======================== internal function ======================

__STATIC_INLINE uint8_t _WriteCmd(uint8_t cmd)
{
    uint8_t res;
    NF01S_CS_LOW();
    res = _WriteDataCallBk(cmd);
    NF01S_CS_HIGH();
    return res;
}

__STATIC_INLINE void _WriteReg(uint8_t addr, uint8_t dat)
{
    NF01S_CS_LOW();
    _WriteDataCallBk(_WR_OFFSET | addr);
    _WriteDataCallBk(dat);
    NF01S_CS_HIGH();
}

__STATIC_INLINE uint8_t _ReadReg(uint8_t addr)
{
    uint8_t dat;
    NF01S_CS_LOW();
    _WriteDataCallBk(addr);
    dat = _WriteDataCallBk(0);
    NF01S_CS_HIGH();
    return dat;
}

//==================================================

void NF01S_Init(NF01S_InitTypeDef *configInfo)
{
    uint8_t i;
    _WriteDataCallBk = configInfo->writeDataCallBk;

    NF01S_EN_LOW();
    _WriteReg(NF01S_CONFIG_REG, NF01S_CONFIG_IT_RX_EN |
                                    NF01S_CONFIG_IT_TX_EN |
                                    NF01S_CONFIG_IT_MAX_RT_EN |
                                    NF01S_CONFIG_PWR_EN |
                                    NF01S_CONFIG_CRC_LEN_2BYTE |
                                    NF01S_CONFIG_CRC_EN);    // 开启所有中断,电源, 16位 CRC, 进入IDLE-TX模式
    _WriteReg(NF01S_ADDR_WIDTH_REG, NF01S_ADDR_WIDTH_4BYTE); // 地址宽度 4 byte
    _WriteReg(NF01S_AUTO_ACK_REG, 0x00);                     // 关闭所有自动应答
    _WriteReg(NF01S_RX_PIPE_EN_REG, 0x00);                   // 关闭所有管道
    _WriteReg(NF01S_RETRY_CONFIG_REG, 0x04);                 // 重发延迟 250us, 4 次, 1ms
    _WriteReg(NF01S_RF_CHANNAL_REG, configInfo->RF_ChannalOffset);
    _WriteReg(NF01S_RF_CONFIG_REG, configInfo->transferSpeed | configInfo->transferPower);

    // set data width
    for (i = 0; i < 6; i++)
        _WriteReg(NF01S_RX_PIPEx_WIDTH_REG(i), 32);

    // reset pipe 1 addr to 0xE7E7E7E7E7
    NF01S_CS_LOW();
    _WriteDataCallBk(_WR_OFFSET + NF01S_RX_PIPEx_ADDR_REG(1));
    for (i = 0; i < 5; i++)
        _WriteDataCallBk(0xE7);
    NF01S_CS_HIGH();

    // clear all
    _WriteCmd(_CMD_FLUSH_RX);
    _WriteCmd(_CMD_FLUSH_TX);
}
/* 
uint32_t NF01S_Rx_GetPipeAddr(uint8_t pipe_x)
{
    uint8_t i;
    uint32_t addr = 0x00;

    if (pipe_x < 2)
    {
        NF01S_CS_LOW();
        _WriteDataCallBk(NF01S_RX_PIPEx_ADDR_REG(pipe_x));
        for (i = 0; i < 4; i++)
            addr |= (((uint32_t)_WriteDataCallBk(0)) << (i * 8));
        NF01S_CS_HIGH();
    }
    else
    {
        NF01S_CS_LOW();
        _WriteDataCallBk(NF01S_RX_PIPEx_ADDR_REG(1));
        for (i = 0; i < 4; i++)
            addr |= (((uint32_t)_WriteDataCallBk(0)) << (i * 8));
        NF01S_CS_HIGH();

        addr &= 0xFFFFFF00;

        NF01S_CS_LOW();
        _WriteDataCallBk(NF01S_RX_PIPEx_ADDR_REG(pipe_x));
        addr |= _WriteDataCallBk(0);
        NF01S_CS_HIGH();
    }

    return addr;
}
 */
void NF01S_Rx_SetPipeAddr(uint8_t pipe_x, uint16_t _addr)
{
    uint8_t i;
    uint32_t addr = _ADDR_PREFIX | _addr;

    if (pipe_x < 2)
    {
        NF01S_CS_LOW();
        _WriteDataCallBk(_WR_OFFSET + NF01S_RX_PIPEx_ADDR_REG(pipe_x));
        for (i = 0; i < 4; i++)
        {
            _WriteDataCallBk((uint8_t)addr);
            addr >>= 8;
        }
        NF01S_CS_HIGH();
    }
    else
    {
        _WriteReg(NF01S_RX_PIPEx_ADDR_REG(pipe_x), (uint8_t)_addr);
    }
}

void NF01S_Rx_PipeCmd(uint8_t pipe_x, uint8_t state)
{
    uint8_t oldConfig;

    oldConfig = _ReadReg(NF01S_AUTO_ACK_REG);
    oldConfig = state ? ((1 << pipe_x) | oldConfig) : ((~(1 << pipe_x)) & oldConfig);
    _WriteReg(NF01S_AUTO_ACK_REG, oldConfig);

    oldConfig = _ReadReg(NF01S_RX_PIPE_EN_REG);
    oldConfig = state ? ((1 << pipe_x) | oldConfig) : ((~(1 << pipe_x)) & oldConfig);
    _WriteReg(NF01S_RX_PIPE_EN_REG, oldConfig);
}

void NF01S_Tx_SetTargetAddr(uint16_t _addr)
{
    uint8_t i;
    uint32_t addr = _ADDR_PREFIX | _addr;

    NF01S_CS_LOW();
    _WriteDataCallBk(_WR_OFFSET + NF01S_TX_ADDR_REG);
    for (i = 0; i < 4; i++)
    {
        _WriteDataCallBk((uint8_t)addr);
        addr >>= 8;
    }
    NF01S_CS_HIGH();
}
/* 
uint32_t NF01S_Tx_GetTargetAddr(void)
{
    uint8_t i;
    uint32_t addr = 0x00;

    NF01S_CS_LOW();
    _WriteDataCallBk(NF01S_TX_ADDR_REG);
    for (i = 0; i < 4; i++)
        addr |= (((uint32_t)_WriteDataCallBk(0)) << (i * 8));
    NF01S_CS_HIGH();

    return addr;
}
 */
int8_t NF01S_Rx_ReceiveData(NF01S_Buffer buffer)
{
    int8_t pipex = -1;
    uint8_t status;

    status = _WriteCmd(_CMD_NOP);
    _WriteReg(NF01S_STATUS_REG, status);

    if (status & NF01S_STATUS_DAT_READY)
    {
        pipex = (status & NF01S_STATUS_RX_PIPE_NUMBER) >> 1;
        if (pipex < 6)
        {
            NF01S_CS_LOW();
            _WriteDataCallBk(_FIFO_READ_ADDR);
            for (status = 0; status < NF01S_BUF_SIZE; status++)
                buffer[status] = _WriteDataCallBk(0);
            NF01S_CS_HIGH();
        }
        else
            pipex = -1;
        _WriteCmd(_CMD_FLUSH_RX);
    }

    return pipex;
}

uint8_t NF01S_Tx_SendData(NF01S_Buffer buffer)
{
    uint8_t status;

    NF01S_EN_LOW();
    NF01S_CS_LOW();
    _WriteDataCallBk(_FIFO_WRITE_ADDR);
    for (status = 0; status < NF01S_BUF_SIZE; status++)
        _WriteDataCallBk(buffer[status]);
    NF01S_CS_HIGH();
    NF01S_EN_HIGH();

    while (NF01S_IRQ_READ())
        ;

    NF01S_EN_LOW();
    status = _WriteCmd(_CMD_NOP);
    _WriteReg(NF01S_STATUS_REG, status);

    if (status & NF01S_STATUS_DAT_SEND)
    {
        return 1;
    }
    else
    {
        _WriteCmd(_CMD_FLUSH_TX);
        return 0;
    }
}

void NF01S_SwitchMode(uint8_t _mode, uint16_t addr)
{
    uint8_t oldMode;

    NF01S_EN_LOW();
    oldMode = _ReadReg(NF01S_CONFIG_REG);

    // 要设置接收管道 0 地址与自身发送地址相同，以便接收 ACK 信号
    NF01S_Rx_SetPipeAddr(0, addr);
    NF01S_Rx_PipeCmd(0, 1);

    if ((oldMode & 0x01) == _mode) // if old mode == require mode, refresh addr
    {
        if (_mode == NF01S_CONFIG_MODE_TX)
        {
            NF01S_Tx_SetTargetAddr(addr);
        }
        return;
    }

    if (_mode == NF01S_CONFIG_MODE_TX)
    {
        NF01S_Tx_SetTargetAddr(addr);
        _WriteReg(NF01S_CONFIG_REG, oldMode & 0xFE);
    }
    else
    {
        _WriteReg(NF01S_CONFIG_REG, oldMode | NF01S_CONFIG_MODE_RX);
        _WriteReg(NF01S_STATUS_REG, 0xFF);
        _WriteCmd(_CMD_FLUSH_RX);
        NF01S_EN_HIGH();
    }
}
