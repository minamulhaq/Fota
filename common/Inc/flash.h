#ifndef _INC_FLASH_H__
#define _INC_FLASH_H__

#define ALIGNED \
	(address, alignment)(((alignment) - 1U + (alignment)) & -(alignment))

#include "stm32l4xx.h"
#include "stm32l4xx_hal_flash.h"

#define BOOTLOADER_SISE 0x10000U
#define FOTA_SHARED_START (FLASH_BASE + BOOTLOADER_SISE)
#define FOTA_SHARED_SIZE 0x800
#define FLASH_SECTOR_APP_START_ADDRESS \
	(FLASH_BASE + BOOTLOADER_SISE + FOTA_SHARED_SIZE)

#define FOTA_SHARED_REGION __attribute__((section(".API_SHARED")))

/* Shared region: page 32, 1 page */
#define FOTA_SHARED_PAGE 32
#define FOTA_SHARED_NBPAGES 1
#define FOTA_SHARED_BANK FLASH_BANK_1

/* App region: page 33, 128 pages */
#define FOTA_APP_PAGE 33
#define FOTA_APP_NBPAGES 128
#define FOTA_APP_BANK FLASH_BANK_1

/* Shared + App region: page 32, 129 pages */
#define FOTA_SHARED_APP_PAGE 32
#define FOTA_SHARED_APP_NBPAGES 129
#define FOTA_SHARED_APP_BANK FLASH_BANK_1

#endif // _INC_FLASH_H__
