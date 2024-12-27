#include "bsp_flash.h"

void flash_module_init(void)
{
    Flash_WaitCycle(FlashWaitCycle1);
		Flash_Init(12, TRUE);
}

void flash_erase_sector(uint32_t addr)
{
		//设置flash操作模式清扇区模式
		Flash_OpModeConfig(FlashSectorEraseMode);

    __set_PRIMASK(1);		
    Flash_SectorErase(addr);
    __set_PRIMASK(0);
}

uint16_t cal_crc16(uint8_t* data, uint32_t size)
{
    uint16_t crc = 0xffff;
    uint8_t* dataEnd = data + size;
    uint8_t cyc = 0, sign = 0;

    while(data < dataEnd)
    {
        crc = crc^*data++;
        for(cyc = 0; cyc < 8; cyc ++)
        {
            sign = crc & 0x0001;
            crc >>= 1;
            if(sign)
                crc ^= 0xa001;
        }
    }
    crc = ((crc & 0xff00) >> 8) | ((crc & 0x00ff) << 8);

    return (crc&0xffffu);
}

uint16_t flash_read_half_word_16bit(uint32_t addr)
{
    return *(volatile uint16_t*)addr;
}

void flash_write_data(uint32_t write_addr, uint16_t *data, uint16_t byte_len)
{
    uint16_t i;
		
		//设置flash操作模式写入模式
		Flash_OpModeConfig(FlashWriteMode);

    __set_PRIMASK(1);
    for(i = 0; i < byte_len / 2; i ++)
    {
        Flash_WriteHalfWord(write_addr, data[i]);
        write_addr += 2;//地址增加2.
    }
    __set_PRIMASK(0);
}

void flash_read_data(uint32_t read_addr,uint16_t *data,uint16_t byte_len)
{
    uint16_t i;

		//设置flash操作模式读取模式
		Flash_OpModeConfig(FlashReadMode);

    for(i = 0; i < byte_len / 2; i ++)
    {
        data[i] = flash_read_half_word_16bit(read_addr);
        read_addr += 2;
    }
}

