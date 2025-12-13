#include "packet_controller.h"
#include "sm_common.h"
#include "flash.h"
#include "math.h"

static void setup_fw_packet(packet_controller_t *const pcontroller)
{
	pcontroller->total_packets =
		(pcontroller->fw_size + (MAX_PAYLOAD_SIZE - 1)) /
		MAX_PAYLOAD_SIZE;
	pcontroller->current_packet_number = 0U;
}

void packet_controller_init(packet_controller_t *const pcontroller,
			    const uint32_t fw_size)
{
	memcpy(pcontroller, 0, sizeof(packet_controller_t));
	pcontroller->current_flash_address = FOTA_SHARED_START;
	pcontroller->fw_size = fw_size;
	setup_fw_packet(pcontroller);
}

void packet_controller_reset(packet_controller_t *const pcontroller)
{
	memcpy(pcontroller, 0, sizeof(packet_controller_t));
}