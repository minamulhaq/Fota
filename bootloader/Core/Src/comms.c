
/*
State machine setup to receive command packets
*/

#include "comms.h"
#include "bootloader.h"

static uint8_t bytes_collected_counter = 0;
static comms_packet_t temp_packet = { 0 };
static uint8_t payload_buffer;

static comms_packet_t ack_packet = { .command_id = B_ACK, .length = 0 };
static comms_packet_t nack_packet = { .command_id = B_NACK, .length = 0 };

static comm_context_t COMM_FSM = { 0 };

/*
void comms_update(void)
{

		case COMM_STATE_PAYLOAD: {
		} break;

		case COMM_STATE_CRC: {
		} break;

		default: {
			state = COMM_STATE_ID;
			break;
		}
		}
	}
}
*/

comms_state_t comm_state_init()
{
	printmsg("Comms setup\r\n");
	COMM_FSM.state = comm_state_id;
	ack_packet.crc = bootloader_verify_crc(&ack_packet);
	nack_packet.crc = bootloader_verify_crc(&nack_packet);
	bytes_collected_counter = 0;
	return COMM_STATE_ID;
}
comms_state_t comm_state_process_byte(uint8_t byte)
{
	return COMM_FSM.state(byte);
}
comms_state_t comm_state_id(uint8_t byte)
{
	temp_packet.command_id = byte;
	/* TODO: Verify ID? */
	COMM_FSM.state = comm_state_length;
	return COMM_STATE_LENGTH;
}
comms_state_t comm_state_length(uint8_t byte)
{
	if ((uint8_t)byte < 1) {
		temp_packet.length = 0;
		COMM_FSM.state = comm_state_crc;
		return COMM_STATE_CRC;
	} else {
		temp_packet.length = (uint8_t)byte;
		bytes_collected_counter = 0;
		COMM_FSM.state = comm_state_payload;
		return COMM_STATE_PAYLOAD;
	}
}
comms_state_t comm_state_payload(uint8_t byte)
{
	// Collect payload byte
	if (bytes_collected_counter < temp_packet.length) {
		temp_packet.payload[bytes_collected_counter++] = byte;

		// Check if payload collection is complete
		if (bytes_collected_counter >= temp_packet.length) {
			bytes_collected_counter = 0; // Reset for CRC collection
			COMM_FSM.state = comm_state_crc;
			return COMM_STATE_CRC;
		}
		return COMM_STATE_PAYLOAD; // Still collecting
	}
}
comms_state_t comm_state_crc(uint8_t byte)
{
	uint8_t *crc_bytes = (uint8_t *)&temp_packet.crc;

	if (bytes_collected_counter < 4) {
		crc_bytes[bytes_collected_counter++] = byte;

		if (bytes_collected_counter == 4) {
			return comm_state_crc_verify();
		}
		return COMM_STATE_CRC;
	}
	return comm_state_crc_verify();
}
comms_state_t comm_state_crc_verify(void)
{
	bool valid = bootloader_verify_crc(&temp_packet);

	printmsg("CRC Verify: %s (expected: 0x%08lX, received: 0x%08lX)\r\n",
		 valid ? "PASS" : "FAIL",
		 (unsigned long)bootloader_compute_crc(&temp_packet),
		 (unsigned long)temp_packet.crc);

	// Reset FSM for next packet
	bytes_collected_counter = 0;
	memset(&temp_packet, 0, sizeof(temp_packet));
	COMM_FSM.state = comm_state_id;

	return valid ? COMM_STATE_PACKET_READY : COMM_STATE_PACKET_INVALID;
}
