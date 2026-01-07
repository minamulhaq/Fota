#ifndef _INC_BL_SERRIF_H__
#define _INC_BL_SERRIF_H__

#include "common_defines.h"
#include "bootloader.h"

typedef void (*bl_send_bytes_fn_t)(const uint8_t *data, const uint32_t size);
typedef void (*bl_deinit_fn_t)(void);

typedef struct bl_serrif_t {
	bl_send_bytes_fn_t wb;
} bl_serrif_t;

typedef struct bl_handle {
	bl_serrif_t serrif;
	bl_deinit_fn_t deinit;
} bl_handle_t;

#endif // _INC_BL_SERRIF_H__
