#ifndef _INC_FLASH_H__
#define _INC_FLASH_H__

#include "stm32l4xx.h"
#define BOOTLOADER_SISE 0x10000U
#define FOTA_SHARED_SIZE 0x800
#define FLASH_SECTOR_APP_START_ADDRESS \
	(FLASH_BASE + BOOTLOADER_SISE + FOTA_SHARED_SIZE)

#define FOTA_SHARED_REGION __attribute__((section(".API_SHARED")))

#endif // _INC_FLASH_H__
