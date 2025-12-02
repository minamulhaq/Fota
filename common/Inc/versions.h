#ifndef _INC_VERSIONS_H__
#define _INC_VERSIONS_H__

#include "common_defines.h"

typedef struct version {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	uint8_t padding[5];
} version_t;

#endif // _INC_VERSIONS_H__
