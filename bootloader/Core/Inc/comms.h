#ifndef _INC_BL_COMMAND_PACKET_H__
#define _INC_BL_COMMAND_PACKET_H__

#include "sm_common.h"

typedef struct comm_context comm_context_t;

/* State function pointers */
typedef status_t (*comm_state_handler)(Event const *const e, uint8_t *byte,
				       packet_status_t *packet_status);

struct comm_context {
	comm_state_handler state;
};

#define TRANSIT_TO(target_) (COMM_FSM.state = (target_), STATE_TRANSITION)
#define TRANSIT_TO_AND_SET_STATE(target_, state_) \
	(COMM_FSM.state = (target_), (state_))

/* State function declarations */
status_t comm_state_process_byte(uint8_t *byte, packet_status_t *packet_status);

status_t comm_state_init(Event const *const e, uint8_t *byte,
			 packet_status_t *packet_status);
status_t comm_state_id(Event const *const e, uint8_t *byte,
		       packet_status_t *packet_status);
status_t comm_state_length(Event const *const e, uint8_t *byte,
			   packet_status_t *packet_status);
status_t comm_state_payload(Event const *const e, uint8_t *byte,
			    packet_status_t *packet_status);
status_t comm_state_crc(Event const *const e, uint8_t *byte,
			packet_status_t *packet_status);

comms_packet_t *comm_get_last_packet(void);

#endif // _INC_BL_COMMAND_PACKET_H__
