
#include "bootloader_fsm.h"
#include "msg_printer.h"

status_t bootloader_fsm_dispatch(bootloader_context_t *b_context,
				 Event const *const e,
				 packet_status_t *packet_status)
{
	status_t state;
	bootloader_fsm_handler prev = b_context->handler;
	state = (*b_context->handler)(b_context, e, packet_status);
	if (state == STATE_TRANSITION) {
		(prev)(b_context, &exitEvt, packet_status);
		(*b_context->handler)(b_context, &entryEvt, packet_status);
	}
	return state;
}

status_t bootloader_fsm_init(bootloader_context_t *b_context,
			     Event const *const e,
			     packet_status_t *packet_status)
{
	printmsg("bootloader fsm init\r\n");
	status_t state;
	switch (e->sig) {
	case SIGNAL_INIT: {
		if (packet_status == PACKET_READY) {
			state = STATE_HANDLED;
		} else {
			state = STATE_IGNORED;
		}
		break;
	}

	default:
		state = STATE_IGNORED;
		break;
	}
	return state;
}