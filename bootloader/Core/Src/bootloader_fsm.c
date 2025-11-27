
#include "bootloader_fsm.h"
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
		printmsg("bootloader packet ready\r\n");

		me->packet_status = PACKET_NOT_READY;
		status = FSM_TRANSIT_TO(bootloader_fsm_wait_for_packet);
		FSM_TRANSIT_TO(bootloader_fsm_wait_for_packet);
	}

	return status;
}