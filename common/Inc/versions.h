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
	uint32_t padding0;
	uint32_t padding1;
	uint32_t padding2;
	uint32_t padding3;
	uint32_t app_size;
	uint32_t crc;
} fw_info_t;

#endif // _INC_VERSIONS_H__
