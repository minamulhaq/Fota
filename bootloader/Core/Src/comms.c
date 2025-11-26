
/*
State machine setup to receive command packets
*/

#include "common_defines.h"
#include "comms.h"
#include "bootloader.h"

static uint8_t bytes_collected_counter = 0;
static comms_packet_t temp_packet = { 0 };
static uint8_t payload_buffer;

static comm_context_t COMM_FSM = { 0 };
static Event const byteReceivedEvent = { SIGNAL_BYTE_RECEIVED };

status_t comm_state_process_byte(uint8_t *byte, packet_status_t *packet_status)
{
	printmsg("Byte received: %x\r\n", *byte);
	status_t state;
	comm_state_handler prev = COMM_FSM.state;
	state = (*COMM_FSM.state)(&byteReceivedEvent, byte, packet_status);
	if (state == STATE_TRANSITION) {
		(prev)(&exitEvt, byte, packet_status);
		(COMM_FSM.state)(&entryEvt, byte, packet_status);
	}

	return state;
}

status_t comm_state_init(Event const *const e, uint8_t *byte, packet_status_t *packet_status)
{
	status_t state;
	switch (e->sig) {
	case SIGNAL_INIT:
		printmsg("Comms setup\r\n");
		TRANSIT_TO(comm_state_id);
		(*COMM_FSM.state)(&entryEvt, NULL, packet_status);
		state = STATE_HANDLED;
		break;
	default:
		state = STATE_IGNORED;
		break;
	}
	*packet_status = PACKET_NOT_READY;
	return state;
}
status_t comm_state_id(Event const *const e, uint8_t *byte,
		       packet_status_t *packet_status)
{
	status_t state;
	switch (e->sig) {
	case SIGNAL_ENTRY: {
		bytes_collected_counter = 0;
		memset(&temp_packet, 0, sizeof(temp_packet));
		TRANSIT_TO(comm_state_id);
		state = STATE_HANDLED;
		break;
	}
	case SIGNAL_BYTE_RECEIVED: {
		temp_packet.command_id = *byte;
		state = STATE_HANDLED;
		state = TRANSIT_TO(comm_state_length);
		break;
	}

	default: {
		state = STATE_IGNORED;
		break;
	}
	}
	*packet_status = PACKET_NOT_READY;
	return state;
}
status_t comm_state_length(Event const *const e, uint8_t *byte,
			   packet_status_t *packet_status)
{
	status_t state;
	switch (e->sig) {
	case SIGNAL_BYTE_RECEIVED: {
		if (*byte < 1) {
			temp_packet.length = 0;
			TRANSIT_TO(comm_state_crc);
		} else {
			temp_packet.length = *byte;
			TRANSIT_TO(comm_state_payload);
		}
		state = STATE_TRANSITION;
		break;
	}
	case SIGNAL_EXIT: {
		break;
	}

	default: {
		state = STATE_IGNORED;
		break;
	}
	}
	*packet_status = PACKET_NOT_READY;
	return state;
}
status_t comm_state_payload(Event const *const e, uint8_t *byte,
			    packet_status_t *packet_status)
{
	status_t state;
	switch (e->sig) {
	case SIGNAL_ENTRY: {
		bytes_collected_counter = 0;
		break;
	}

	case SIGNAL_BYTE_RECEIVED: {
		if (bytes_collected_counter < temp_packet.length) {
			temp_packet.payload[bytes_collected_counter++] = *byte;
			state = STATE_HANDLED;

			// Check if payload collection is complete
			if (bytes_collected_counter >= temp_packet.length) {
				state = TRANSIT_TO(comm_state_crc);
			}
		}
		break;
	}

	default: {
		state = STATE_IGNORED;
		break;
	}
	}
	*packet_status = PACKET_NOT_READY;
	return state;
}
status_t comm_state_crc(Event const *const e, uint8_t *byte,
			packet_status_t *packet_status)
{
	status_t state;
	switch (e->sig) {
	case SIGNAL_ENTRY: {
		bytes_collected_counter = 0;
		break;
	}
	case SIGNAL_BYTE_RECEIVED: {
		uint8_t *crc_bytes = (uint8_t *)&temp_packet.crc;
		if (bytes_collected_counter < 4) {
			crc_bytes[bytes_collected_counter++] = *byte;
			state = STATE_HANDLED;
			if (bytes_collected_counter >= 4) {
				bool valid =
					bootloader_verify_crc(&temp_packet);
				*packet_status = valid ? PACKET_READY :
							 PACKET_INVALID;
			}
			state = STATE_HANDLED;
		}
		break;
	}

	default: {
		state = STATE_IGNORED;
		break;
	}
	}
	return state;
}
