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

typedef enum {
	B_ACK = 0xE0,
	B_NACK = 0xE1,
	PAYLOAD_TYPE_BINARY = 0xE2,
	PAYLOAD_TYPE_STRING = 0xE3,
	PAYLOAD_TYPE_JSON = 0xE4,
	B_DEBUG_STRING = 0xF0,
	B_LOG_MESSAGE = 0xF1,
	B_ERROR_STRING = 0xF2
} payload_type_t;

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

typedef enum packet_status {
	PACKET_NOT_READY,
	PACKET_READY,
	PACKET_VALID,
	PACKET_INVALID,
} packet_status_t;

typedef enum status {
	STATE_TRANSITION,
	STATE_HANDLED,
	STATE_IGNORED,
	STATE_INIT,
} status_t;

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

#endif // _INC_BL_COMMAND_PACKET_H__
