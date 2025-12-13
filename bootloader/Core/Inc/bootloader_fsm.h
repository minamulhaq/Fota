#ifndef _INC_BOOTLOADER_FSM_H__
#define _INC_BOOTLOADER_FSM_H__

#include "common_defines.h"
#include "comms.h"
#include "sm_common.h"
#include "bootloader_cmds.h"

typedef struct bootloader_fsm bootloader_fsm_t;
/* typedef status_t (*bootloader_fsm_handler)(bootloader_fsm_t *me,
					   Event const *const e) */
struct bootloader_fsm {
	Fsm fsm;
	// bootloader_fsm_handler handler;
	uint8_t uart_byte;
	EventSignals packet_status;
};
void bootloader_fsm_ctr(bootloader_fsm_t *me, StateHandler initial);

status_t bootloader_fsm_verify_packet_id(bootloader_fsm_t *me,
					 Event const *const e);

status_t bootloader_fw_update_sync_request(bootloader_fsm_t *me,
					   Event const *const e);

#define FSM_TRANSIT_TO(target_) \
	(((Fsm *)me)->state = (StateHandler)(target_), STATE_TRANSITION)

#define FSM_SUPER_TRANSIT_TO(target_) \
	(((Fsm *)me)->super_state = (StateHandler)(target_), STATE_SUPER)

#endif // _INC_BOOTLOADER_FSM_H__
