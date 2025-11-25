
/*
State machine setup to receive command packets
*/

#include "comms.h"
#include "bootloader.h"

static comms_state_t state = COMM_STATE_ID;
static uint8_t bytes_collected_counter = 0;
static comms_packet_t temp_packet = { 0 };
static uint8_t payload_buffer;

static comms_packet_t ack_packet = { .command_id = B_ACK, .length = 0 };
static comms_packet_t nack_packet = { .command_id = B_NACK, .length = 0 };

void comms_setup(void)
{
	printmsg("Comms setup\r\n");
	ack_packet.crc = bootloader_verify_crc(&ack_packet);
	nack_packet.crc = bootloader_verify_crc(&ack_packet);
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
			if (bytes_collected_counter < temp_packet.length) {
				uint8_t b;
				if (bootloader_read_bytes(&b, 1) == 1) {
					temp_packet.payload
						[bytes_collected_counter++] = b;
				}
			}
			if (bytes_collected_counter >= temp_packet.length) {
				bytes_collected_counter = 0;
				state = COMM_STATE_CRC;
				bytes_collected_counter =
					0; // reuse counter for CRC
			}
		} break;

		case COMM_STATE_CRC: {
			uint8_t *crc_bytes = (uint8_t *)&temp_packet.crc;
			if (bytes_collected_counter < 4) {
				uint8_t b;
				if (bootloader_read_bytes(&b, 1) == 1) {
					crc_bytes[bytes_collected_counter++] =
						b;
				}
			}
			if (bytes_collected_counter >= 4) {
				bytes_collected_counter = 0;
				if (bootloader_verify_crc(&temp_packet)) {
					printmsg(
						"Succesful command received: %d\r\n",
						temp_packet.command_id);
				} else {
					printmsg(
						"CRC verification Failed: %d\r\n",
						temp_packet.command_id);
				}
				temp_packet.length = 0;
				temp_packet.command_id = 0;
				temp_packet.crc = 0;
				state = COMM_STATE_ID;
			}
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
