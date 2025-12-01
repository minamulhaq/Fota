
#include "bootloader_fsm.h"
#include "bootloader.h"
#include "msg_printer.h"

extern Event byte_received_event;
extern Event entry_event;
extern Event exit_event;
extern Event init_event;

void bootloader_fsm_ctr(bootloader_fsm_t *me, StateHandler initial)
{
	Fsm_ctor(&me->fsm, initial);
}

status_t bootloader_fsm_verify_packet_id(bootloader_fsm_t *me,
					 Event const *const e)
{
	status_t status;
	switch (e->sig) {
	case SIGNAL_ENTRY: {
		status = STATE_HANDLED;
		break;
	}

	case SIGNAL_EXIT: {
		me->packet_status = SIGNAL_PACKET_NOT_READY;
		status = STATE_HANDLED;
		break;
	}

	case SIGNAL_PACKET_VALID: {
		comms_packet_t *last_received_packet = comm_get_last_packet();
		bootloader_cmd_t *handle =
			get_command_handle(last_received_packet);
		if (handle == NULL) {
			status = FSM_TRANSIT_TO(comm_state_id);
		} else {
			comms_packet_t response_packet = { 0 };
			handle->process(last_received_packet, &response_packet);
			bootlader_send_response_packet(&response_packet);

			status = FSM_TRANSIT_TO(comm_state_id);
		}
		break;
	}

	case SIGNAL_PACKET_INVALID: {
		bootloader_cmd_t *handle = cmd_send_retransmit_last_cmd();
		if (handle == NULL) {
			status = FSM_TRANSIT_TO(comm_state_id);
		} else {
			comms_packet_t response_packet = { 0 };
			handle->process(NULL, &response_packet);
			bootlader_send_response_packet(&response_packet);
			status = FSM_TRANSIT_TO(comm_state_id);
		}
		break;
	}

	default:
		status = STATE_IGNORED;
		break;
	}
	return status;
}