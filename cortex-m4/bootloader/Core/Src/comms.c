
/*
State machine setup to receive command packets
*/

#include "common_defines.h"
#include "comms.h"
#include "bootloader.h"
#include "memory.h"

static uint8_t bytes_collected_counter = 0;
static comms_packet_t temp_packet = { 0 };
static comms_packet_t last_received_packet = { 0 };

extern Event byte_received_event;
extern Event entry_event;
extern Event exit_event;

status_t comm_state_init(bootloader_fsm_t *me, StateHandler initial)
{
	(void)initial;
	return FSM_TRANSIT_TO(comm_state_frame);
}

static const uint8_t packet_frame[4] = { 0xA5, 0xAA, 0xBB, 0xA5 };

status_t comm_state_frame(bootloader_fsm_t *const me, Event const *const e)
{
	status_t state;
	switch (e->sig) {
	case SIGNAL_ENTRY: {
		bytes_collected_counter = 0;
		me->packet_status = SIGNAL_PACKET_NOT_READY;
		state = STATE_HANDLED;
		break;
	}
	case SIGNAL_BYTE_RECEIVED: {
		if (me->uart_byte == packet_frame[bytes_collected_counter]) {
			bytes_collected_counter++;
			if (bytes_collected_counter == 4) {
				state = FSM_TRANSIT_TO(comm_state_id);
			}
		}
		else {
			bytes_collected_counter = 0;
		}
		state = STATE_HANDLED;
		break;
	}

	default: {
		state = STATE_IGNORED;
		break;
	}
	}
	return state;
}

status_t comm_state_id(bootloader_fsm_t *const me, Event const *const e)
{
	status_t state;
	switch (e->sig) {
	case SIGNAL_ENTRY: {
		bytes_collected_counter = 0;
		memset(&temp_packet, 0, sizeof(temp_packet));
		memset(&temp_packet.payload, 0xFF, MAX_PAYLOAD_SIZE);
		me->packet_status = SIGNAL_PACKET_NOT_READY;
		state = STATE_HANDLED;
		break;
	}
	case SIGNAL_BYTE_RECEIVED: {
		temp_packet.command_id = me->uart_byte;
		state = FSM_TRANSIT_TO(comm_state_length);
		break;
	}

	default: {
		state = STATE_IGNORED;
		break;
	}
	}
	return state;
}
status_t comm_state_length(bootloader_fsm_t *const me, Event const *const e)
{
	status_t state;
	switch (e->sig) {
	case SIGNAL_BYTE_RECEIVED: {
		if (me->uart_byte < 1) {
			temp_packet.length = 0;
			state = FSM_TRANSIT_TO(comm_state_crc);
		} else {
			temp_packet.length = me->uart_byte;
			state = FSM_TRANSIT_TO(comm_state_payload);
		}
		break;
	}
	case SIGNAL_EXIT: {
		me->packet_status = SIGNAL_PACKET_NOT_READY;
		break;
	}

	default: {
		state = STATE_IGNORED;
		break;
	}
	}
	return state;
}
status_t comm_state_payload(bootloader_fsm_t *const me, Event const *const e)
{
	status_t status;
	switch (e->sig) {
	case SIGNAL_ENTRY: {
		bytes_collected_counter = 0;
		status = STATE_HANDLED;
		break;
	}

	case SIGNAL_BYTE_RECEIVED: {
		if (bytes_collected_counter < temp_packet.length) {
			temp_packet.payload[bytes_collected_counter++] =
				me->uart_byte;
			status = STATE_HANDLED;

			// Check if payload collection is complete
			if (bytes_collected_counter >= temp_packet.length) {
				status = FSM_TRANSIT_TO(comm_state_crc);
			}
		}
		break;
	}

	case SIGNAL_EXIT: {
		me->packet_status = SIGNAL_PACKET_NOT_READY;
		status = STATE_HANDLED;
		break;
	}

	default: {
		status = STATE_IGNORED;
		break;
	}
	}

	return status;
}

status_t comm_state_crc(bootloader_fsm_t *const me, Event const *const e)
{
	status_t state;
	switch (e->sig) {
	case SIGNAL_ENTRY: {
		bytes_collected_counter = 0;
		state = STATE_HANDLED;
		break;
	}
	case SIGNAL_BYTE_RECEIVED: {
		uint8_t *crc_bytes = (uint8_t *)&temp_packet.crc;
		if (bytes_collected_counter < 4) {
			crc_bytes[bytes_collected_counter++] = me->uart_byte;
			state = STATE_HANDLED;
			if (bytes_collected_counter >= 4) {
				bool valid =
					bootloader_verify_crc(&temp_packet);
				me->packet_status =
					valid ? SIGNAL_PACKET_VALID :
						SIGNAL_PACKET_INVALID;
				state = FSM_TRANSIT_TO(
					bootloader_fsm_verify_packet_id);
			}
		}
		break;
	}

	case SIGNAL_EXIT: {
		if (me->packet_status == SIGNAL_PACKET_VALID) {
			memcpy(&last_received_packet, &temp_packet,
			       sizeof(temp_packet));
		}
		state = STATE_HANDLED;
		break;
	}

	default: {
		state = STATE_IGNORED;
		break;
	}
	}
	return state;
}

comms_packet_t *comm_get_last_packet(void)
{
	return &last_received_packet;
}