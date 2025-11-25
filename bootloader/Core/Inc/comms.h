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
} comms_state_t;

void comms_setup(void);
void comms_update(void);

bool comms_packets_available(void);
void comms_write_packet(comms_packet_t *packet);
void comms_read_packet(comms_packet_t *packet);

#endif // _INC_BL_COMMAND_PACKET_H__
