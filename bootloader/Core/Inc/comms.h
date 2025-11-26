#ifndef _INC_BL_COMMAND_PACKET_H__
#define _INC_BL_COMMAND_PACKET_H__

#include "common_defines.h"

#define MAX_PAYLOAD_SIZE (128)

#define PACKET_BYTES_ID (1)
#define PACKET_BYTES_LENGTH (1)
#define PACKET_BYTES_PAYLOAD (128)
#define PACKET_BYTES_CRC (4)
#define PACKET_LENGTH                                                   \
	(PACKET_BYTES_ID + PACKET_BYTES_LENGTH + PACKET_BYTES_PAYLOAD + \
	 PACKET_BYTES_CRC)

#pragma pack(push, 1)
typedef struct bl_command_packet {
	uint8_t command_id;
	uint8_t length;
	uint8_t payload[MAX_PAYLOAD_SIZE];
	uint32_t crc;
} comms_packet_t;
#pragma pack(pop)

typedef uint16_t Signal;

typedef enum reserved_signals {
	SIGNAL_INIT, /* dispatched before entering event-loop */
	SIGNAL_ENTRY, /* for triggering the entry action in a state */
	SIGNAL_EXIT, /* for triggering the exit action from a state */
	SIGNAL_USER /* first signal available to the users */
} reserved_signals_t;

typedef enum EventSignals {
	SIGNAL_BYTE_RECEIVED = SIGNAL_USER,
	SIGNAL_TIMEOUT,
	/* ... */
	SIGNAL_MAX_COUNT
} EventSignals;

typedef struct Event {
	Signal sig;
} Event;

typedef enum comms_state {
	STATE_TRANSITION,
	STATE_HANDLED,
	STATE_IGNORED,
	STATE_INIT,

	COMM_STATE_PACKET_READY,
	COMM_STATE_PACKET_INVALID

} comms_state_t;

typedef struct comm_context comm_context_t;

/* State function pointers */
typedef comms_state_t (*comm_state_handler)(Event const *const e,
					    uint8_t *byte);

struct comm_context {
	comm_state_handler state;
};

#define TRANSIT_TO(target_) (COMM_FSM.state = (target_), STATE_TRANSITION)
#define TRANSIT_TO_AND_SET_STATE(target_, state_) \
	(COMM_FSM.state = (target_), (state_))

/* State function declarations */
void comm_state_init(Event const *const e);
comms_state_t comm_state_process_byte(uint8_t *byte);
comms_state_t comm_state_id(Event const *const e, uint8_t *byte);
comms_state_t comm_state_length(Event const *const e, uint8_t *byte);
comms_state_t comm_state_payload(Event const *const e, uint8_t *byte);
comms_state_t comm_state_crc(Event const *const e, uint8_t *byte);

#endif // _INC_BL_COMMAND_PACKET_H__
