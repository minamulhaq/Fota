#include "common_defines.h"
#include "versions.h"
#include "flash.h"

const fota_shared_t fota_shared FOTA_SHARED_REGION = {
    .info = {
        .version = {1, 1, 6, 0},
        .padding ={0},
        .app_size = 0x00,
    },
    .firmware_signature = {0},
    .crc = 0,
    .senital = 0xDEADBEEF
};