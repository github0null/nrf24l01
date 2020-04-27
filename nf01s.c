#include "nf01s.h"

#define _WR_OFFSET 0x20

#define _CMD_FLUSH_TX 0xE1
#define _CMD_FLUSH_RX 0xE2
#define _CMD_NOP 0xFF

#define _FIFO_WRITE_ADDR 0xA0
#define _FIFO_READ_ADDR 0x61

#define _ADDR_PREFIX 0xE7E70000U

NF01S_WriteByteCallBk _WriteDataCallBk;

//======================== internal function ======================

#define NF01S_EN(val) NF01S_EN_Write((val))

uint8_t _WriteCmd(uint8_t cmd)
{
    uint8_t res;
    NF01S_CS_Write(0);
    res = _WriteDataCallBk(cmd);
    NF01S_CS_Write(1);
    return res;
}

void _WriteReg(uint8_t addr, uint8_t dat)
{
    NF01S_CS_Write(0);
    _WriteDataCallBk(_WR_OFFSET | addr);
    _WriteDataCallBk(dat);
    NF01S_CS_Write(1);
}

uint8_t _ReadReg(uint8_t addr)
{
    uint8_t dat;
    NF01S_CS_Write(0);
    _WriteDataCallBk(addr);
    dat = _WriteDataCallBk(0);
    NF01S_CS_Write(1);
    return dat;
}

//==================================================

void NF01S_Init(NF01S_InitTypeDef *configInfo)
{
    uint8_t i;
    _WriteDataCallBk = configInfo->writeDataCallBk;

    NF01S_EN(0);
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
    NF01S_CS_Write(0);
    _WriteDataCallBk(_WR_OFFSET + NF01S_RX_PIPEx_ADDR_REG(1));
    for (i = 0; i < 5; i++)
        _WriteDataCallBk(0xE7);
    NF01S_CS_Write(1);

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
        NF01S_CS_Write(0);
        _WriteDataCallBk(NF01S_RX_PIPEx_ADDR_REG(pipe_x));
        for (i = 0; i < 4; i++)
            addr |= (((uint32_t)_WriteDataCallBk(0)) << (i * 8));
        NF01S_CS_Write(1);
    }
    else
    {
        NF01S_CS_Write(0);
        _WriteDataCallBk(NF01S_RX_PIPEx_ADDR_REG(1));
        for (i = 0; i < 4; i++)
            addr |= (((uint32_t)_WriteDataCallBk(0)) << (i * 8));
        NF01S_CS_Write(1);

        addr &= 0xFFFFFF00;

        NF01S_CS_Write(0);
        _WriteDataCallBk(NF01S_RX_PIPEx_ADDR_REG(pipe_x));
        addr |= _WriteDataCallBk(0);
        NF01S_CS_Write(1);
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
        NF01S_CS_Write(0);
        _WriteDataCallBk(_WR_OFFSET + NF01S_RX_PIPEx_ADDR_REG(pipe_x));
        for (i = 0; i < 4; i++)
        {
            _WriteDataCallBk((uint8_t)addr);
            addr >>= 8;
        }
        NF01S_CS_Write(1);
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

    NF01S_CS_Write(0);
    _WriteDataCallBk(_WR_OFFSET + NF01S_TX_ADDR_REG);
    for (i = 0; i < 4; i++)
    {
        _WriteDataCallBk((uint8_t)addr);
        addr >>= 8;
    }
    NF01S_CS_Write(1);
}
/* 
uint32_t NF01S_Tx_GetTargetAddr(void)
{
    uint8_t i;
    uint32_t addr = 0x00;

    NF01S_CS_Write(0);
    _WriteDataCallBk(NF01S_TX_ADDR_REG);
    for (i = 0; i < 4; i++)
        addr |= (((uint32_t)_WriteDataCallBk(0)) << (i * 8));
    NF01S_CS_Write(1);

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
            NF01S_CS_Write(0);
            _WriteDataCallBk(_FIFO_READ_ADDR);
            for (status = 0; status < NF01S_BUF_SIZE; status++)
                buffer[status] = _WriteDataCallBk(0);
            NF01S_CS_Write(1);
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

    NF01S_EN(0);
    NF01S_CS_Write(0);
    _WriteDataCallBk(_FIFO_WRITE_ADDR);
    for (status = 0; status < NF01S_BUF_SIZE; status++)
        _WriteDataCallBk(buffer[status]);
    NF01S_CS_Write(1);
    NF01S_EN(1);

    while (NF01S_IRQ_Read())
        ;

    NF01S_EN(0);
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

    NF01S_EN(0);
    oldMode = _ReadReg(NF01S_CONFIG_REG);

    NF01S_Rx_SetPipeAddr(0, addr);
    NF01S_Rx_PipeCmd(0, 1);

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
        NF01S_EN(1);
    }
}
