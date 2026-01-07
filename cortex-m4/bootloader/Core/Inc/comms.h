#ifndef _INC_BL_COMMAND_PACKET_H__
#define _INC_BL_COMMAND_PACKET_H__

#include "common_defines.h"
#include "sm_common.h"

typedef struct comm_context comm_context_t;

typedef struct bootloader_fsm bootloader_fsm_t;

status_t comm_state_init(bootloader_fsm_t *me, StateHandler initial);
status_t comm_state_frame(bootloader_fsm_t *const me, Event const *const e);
status_t comm_state_id(bootloader_fsm_t *const me, Event const *const e);
status_t comm_state_length(bootloader_fsm_t *const me, Event const *const e);
status_t comm_state_payload(bootloader_fsm_t *const me, Event const *const e);
status_t comm_state_crc(bootloader_fsm_t *const me, Event const *const e);

comms_packet_t *comm_get_last_packet(void);

#endif // _INC_BL_COMMAND_PACKET_H__
