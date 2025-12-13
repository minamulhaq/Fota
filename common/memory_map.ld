/* FOTA COMMON MEMORY MAP */

FLASH_BASE = 0x08000000;
FLASH_TOTAL_LENGTH = 1024K;

/* Bootloader region definitions */
BOOT_FLASH_LENGTH = 64K;
FOTA_SHARED_LENGTH = 2K;

/* Calculate the start addresses based on constants */
BOOT_FLASH_ORIGIN = FLASH_BASE;
FOTA_SHARED_ORIGIN = BOOT_FLASH_ORIGIN + BOOT_FLASH_LENGTH;
APP_FLASH_ORIGIN = FOTA_SHARED_ORIGIN + FOTA_SHARED_LENGTH;

APP_FLASH_LENGTH = 256K;


MEMORY
{
    /* Common RAM regions */
    RAM (xrw)             : ORIGIN = 0x20000000, LENGTH = 96K
    RAM2 (xrw)            : ORIGIN = 0x10000000, LENGTH = 32K
    /* BOOTLOADER-specific Flash region */
    BOOT_FLASH (rx)       : ORIGIN = BOOT_FLASH_ORIGIN, LENGTH = BOOT_FLASH_LENGTH
    /* Shared FOTA region for both images to map */
    FOTA_SHARED (rxw)      : ORIGIN = FOTA_SHARED_ORIGIN, LENGTH = FOTA_SHARED_LENGTH
    /* APPLICATION-specific Flash region */
    APP_FLASH (rx)        : ORIGIN = APP_FLASH_ORIGIN, LENGTH = APP_FLASH_LENGTH
}


_fota_shared_data_start = ORIGIN(FOTA_SHARED);
