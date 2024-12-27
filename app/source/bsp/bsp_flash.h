#ifndef __BSP_FLASH_H__
#define __BSP_FLASH_H__

#include "flash.h"

#define PAMA_BACKUP_START_ADDR 				0x0000FE00 		//出厂备份地址 长512字节
#define PAMA_ZONE_0_START_ADDR 				0x0000FC00 		//参数区0  长512字节
#define PAMA_ZONE_1_START_ADDR 				0x0000FA00 		//参数区1  长512字节
#define PAMA_INFO_START_ADDR 				  0x0000F800 		//参数索引 长512字节
#define Record_START_ADDR_1 				  0x0000F600 		//记录索引1 长512字节
#define Record_START_ADDR_2 				  0x0000F400 		//记录索引1 长512字节

#define DEFAULT_DEVICE_MODLE				"GT30"

void flash_erase_sector(uint32_t addr);
void flash_write_data(uint32_t write_addr, uint16_t *data, uint16_t byte_len);
void flash_read_data(uint32_t read_addr,uint16_t *data,uint16_t byte_len);
uint16_t cal_crc16(uint8_t* data, uint32_t size);
void flash_module_init(void);

#endif
