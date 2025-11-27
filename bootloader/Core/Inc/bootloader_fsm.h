#ifndef _INC_BOOTLOADER_FSM_H__
#define _INC_BOOTLOADER_FSM_H__

#include "comms.h"
#include "sm_common.h"

typedef struct bootloader_fsm bootloader_fsm_t;
/* typedef status_t (*bootloader_fsm_handler)(bootloader_fsm_t *me,
					   Event const *const e) */
struct bootloader_fsm {
	Fsm fsm;
	// bootloader_fsm_handler handler;
	uint8_t uart_byte;
	packet_status_t packet_status;
};

void bootloader_fsm_ctr(bootloader_fsm_t *me, StateHandler initial);

status_t bootloader_fsm_dispatch(bootloader_fsm_t *me, Event const *const e);

status_t bootloader_fsm_wait_for_packet(bootloader_fsm_t *me,
					Event const *const e);

#define FSM_TRANSIT_TO(target_) \
	(((Fsm *)me)->state = (StateHandler)(target_), STATE_TRANSITION)

#endif // _INC_BOOTLOADER_FSM_H__
