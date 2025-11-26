#ifndef _INC_BOOTLOADER_FSM_H__
#define _INC_BOOTLOADER_FSM_H__

#include "sm_common.h"

typedef status_t (*bootloader_fsm_handler)(Event const *const e, uint8_t *byte,
					   packet_status_t *packet_status);

typedef struct bootloader_context {
	bootloader_fsm_handler handler;
} bootloader_context_t;

status_t bootloader_fsm_dispatch(bootloader_context_t *b_context,
				 Event const *const e,
				 packet_status_t *packet_status);

status_t bootloader_fsm_init(bootloader_context_t *b_context,
			     Event const *const e,
			     packet_status_t *packet_status);

#endif // _INC_BOOTLOADER_FSM_H__
