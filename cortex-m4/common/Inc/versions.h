#ifndef _INC_VERSIONS_H__
#define _INC_VERSIONS_H__

#include "common_defines.h"

typedef struct fw_version {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	uint8_t padding;
} fw_version_t;

typedef struct fw_info {
	fw_version_t version;
	uint32_t padding[2];
	uint32_t app_size;
} fw_info_t;

typedef struct __attribute__((packed, aligned(16))) {
	fw_info_t info;
	uint8_t firmware_signature[16];
	uint32_t crc;
	uint32_t padding[2];
	uint32_t senital;
} fota_shared_t;

#endif // _INC_VERSIONS_H__
