#ifndef _INC_SM_COMMON_H__
#define _INC_SM_COMMON_H__

#include "common_defines.h"

#define MAX_PAYLOAD_SIZE (16)

#define PACKET_BYTES_ID (1)
#define PACKET_BYTES_LENGTH (1)
#define PACKET_BYTES_PAYLOAD (MAX_PAYLOAD_SIZE)
#define PACKET_BYTES_CRC (4)
#define PACKET_LENGTH                                          \
	(LENGTH_PACKET_BYTES_ID + LENGTH_PACKET_BYTES_LENGTH + \
	 LENGTH_PACKET_BYTES_PAYLOAD + LENGTH_PACKET_BYTES_CRC)

typedef enum {
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

typedef enum status {
	STATE_TRANSITION,
	STATE_HANDLED,
	STATE_IGNORED,
	STATE_INIT,
	STATE_SUPER,
	MAX_STATES
} status_t;

typedef struct Event {
	Signal sig;
} Event;

typedef enum EventSignals {
	SIGNAL_BYTE_RECEIVED = SIGNAL_USER,
	SIGNAL_PACKET_NOT_READY,
	SIGNAL_PACKET_VALID,
	SIGNAL_PACKET_INVALID,
	SIGNAL_SYNC_REQUESTED,
	SIGNAL_TIMEOUT,
	/* ... */
	SIGNAL_MAX_COUNT
} EventSignals;

typedef struct Fsm Fsm;

typedef status_t (*StateHandler)(Fsm *const me, Event const *const e);
status_t hsm_top_status(Fsm *const me, Event const *const e);

struct Fsm {
	StateHandler state;
	StateHandler super_state;
};

void Fsm_ctor(Fsm *const me, StateHandler initial);
void Fsm_init(Fsm *const me, Event const *const e);
void Fsm_dispatch(Fsm *const me, Event const *const e);

#endif // _INC_SM_COMMON_H__
