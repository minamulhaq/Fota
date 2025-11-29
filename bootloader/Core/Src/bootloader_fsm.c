
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

status_t bootloader_fsm_dispatch(bootloader_fsm_t *me, Event const *const e)
{
	comm_state_process_byte(&me->uart_byte, &me->packet_status);
	Fsm_dispatch(&me->fsm, e);
}

status_t bootloader_fsm_wait_for_packet(bootloader_fsm_t *me,
					Event const *const e)
{
	status_t status;
	if (me->packet_status == PACKET_NOT_READY) {
		switch (e->sig) {
		case SIGNAL_ENTRY:
			me->packet_status = PACKET_NOT_READY;
			comm_state_init(&init_event, NULL, &me->packet_status);
			status = STATE_HANDLED;
			break;

		default:
			status = STATE_HANDLED;
			break;
		}

	} else if (me->packet_status == PACKET_READY) {
		me->packet_status = PACKET_NOT_READY;
		status = FSM_TRANSIT_TO(bootloader_fsm_verify_packet_id);
	}

	return status;
}

status_t bootloader_fsm_verify_packet_id(bootloader_fsm_t *me,
					 Event const *const e)
{
	status_t status;
	switch (e->sig) {
	case SIGNAL_ENTRY:
		status = STATE_HANDLED;
		comms_packet_t *last_packet = comm_get_last_packet();
		bootloader_cmd_t *handle = get_command_handle(last_packet);
		if (handle == NULL) {
		} else {
			comms_packet_t response_packet = { 0 };
			handle->process(last_packet, &response_packet);
			bootlader_send_response_packet(&response_packet);
		}
		break;

	default:
		status = STATE_IGNORED;
		break;
	}
	return status;
}