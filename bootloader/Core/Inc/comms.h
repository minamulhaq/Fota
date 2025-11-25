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

typedef enum comms_state {
	COMM_STATE_ID,
	COMM_STATE_LENGTH,
	COMM_STATE_PAYLOAD,
	COMM_STATE_CRC,
	COMM_STATE_PACKET_READY,
	COMM_STATE_PACKET_INVALID
} comms_state_t;


typedef struct comm_context comm_context_t;

/* State function pointers */
typedef comms_state_t (*comm_state_function)(uint8_t byte);

struct comm_context {
	comm_state_function state;
};

/* State function declarations */
comms_state_t comm_state_init();
comms_state_t comm_state_process_byte(uint8_t byte);
comms_state_t comm_state_id(uint8_t byte);
comms_state_t comm_state_length(uint8_t byte);
comms_state_t comm_state_payload(uint8_t byte);
comms_state_t comm_state_crc(uint8_t byte);
comms_state_t comm_state_crc_verify(void);

#endif // _INC_BL_COMMAND_PACKET_H__
