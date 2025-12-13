#ifndef _INC_PACKET_CONTROLLER_H__
#define _INC_PACKET_CONTROLLER_H__

#include "common_defines.h"

typedef struct packet_controller {
	uint32_t fw_size;
	uint32_t total_packets;
	uint32_t current_packet_number;
	uint32_t current_flash_address;
	bool error_occured;
} packet_controller_t;

void packet_controller_init(packet_controller_t *const pcontroller,
			    const uint32_t fw_size);
void packet_controller_reset(packet_controller_t *const pcontroller);

#endif // _INC_PACKET_CONTROLLER_H__
