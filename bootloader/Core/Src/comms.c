
/*
State machine setup to receive command packets
*/

#include "comms.h"
#include "bootloader.h"

static comms_state_t state = COMM_STATE_ID;
static uint8_t bytes_collected_counter = 0;
static comms_packet_t temp_packet = { 0 };
static uint8_t payload_buffer;

void comms_setup(void)
{
}

void comms_update(void)
{
	while (bootlader_is_data_available()) {
		switch (state) {
		case COMM_STATE_ID: {
			temp_packet.command_id = bootloader_read_byte();
			state = COMM_STATE_LENGTH;
		} break;

		case COMM_STATE_LENGTH: {
			uint8_t len = bootloader_read_byte();
			if ((uint8_t)len < 1) {
				temp_packet.length = 0;
				state = COMM_STATE_CRC;
			} else {
				temp_packet.length = (uint8_t)len;
				bytes_collected_counter = 0;
				state = COMM_STATE_PAYLOAD;
			}
		} break;

		case COMM_STATE_PAYLOAD: {
			bootloader_read_bytes(temp_packet.payload,
					      temp_packet.length);
			state = COMM_STATE_CRC;
		} break;

		case COMM_STATE_CRC: {
			bootloader_read_bytes((uint8_t *)temp_packet.crc, 4);

			if (

				bootloader_verify_crc(
					(uint8_t *)(&temp_packet),
					(PACKET_LENGTH - PACKET_BYTES_CRC),
					temp_packet.crc)) {
				printmsg("Succesful command received: %d\r\n",
					 temp_packet.command_id);
			}
			temp_packet.length = 0;
			temp_packet.command_id = 0;
			temp_packet.crc = 0;
			state = COMM_STATE_ID;
		} break;

		default: {
			state = COMM_STATE_ID;
			break;
		}
		}
	}
}

bool comms_packets_available(void)
{
}

void comms_write_packet(comms_packet_t *packet)
{
}

void comms_read_packet(comms_packet_t *packet)
{
}
