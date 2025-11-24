#ifndef _INC_FLASH_H__
#define _INC_FLASH_H__

#include "stm32l4xx.h"
#define BOOTLOADER_SISE 0x10000U

#define FLASH_SECTOR_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SISE)

#endif // _INC_FLASH_H__
