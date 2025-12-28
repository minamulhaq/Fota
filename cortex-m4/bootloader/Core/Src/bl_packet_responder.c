#include "bl_packet_responder.h"
#include "stm32l4xx_hal_usart.h"

int send_packet_response(comms_packet_t const *const packet)
{
	int total_bytes = sizeof(packet->command_id) + sizeof(packet->length);
	if (packet->length > 0) {
		total_bytes += packet->length;
	}
	total_bytes += sizeof(packet->crc);
	return 0;
}