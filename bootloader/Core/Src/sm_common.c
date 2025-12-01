
#include "sm_common.h"

Event const byte_received_event = { SIGNAL_BYTE_RECEIVED };
Event const event_packet_valid = { SIGNAL_PACKET_VALID };
Event const event_packet_invalid = { SIGNAL_PACKET_INVALID };
Event const entry_event = { SIGNAL_ENTRY };
Event const exit_event = { SIGNAL_EXIT };
Event const init_event = { SIGNAL_INIT };

status_t hsm_top_status(Fsm *const me, Event const *const e)
{
	return STATE_IGNORED;
}
void Fsm_ctor(Fsm *const me, StateHandler initial)
{
	me->state = initial;
}

void Fsm_init(Fsm *const me, Event const *const e)
{
	assert(me->state != NULL);
	(*me->state)(me, e);
	(*me->state)(me, &entry_event);
}

void Fsm_dispatch(Fsm *const me, Event const *const e)
{
	status_t status;
	StateHandler prev = me->state;
	status = (*me->state)(me, e);

	if (status == STATE_TRANSITION) {
		(prev)(me, &exit_event);
		(*me->state)(me, &entry_event);
	}

	while (status == STATE_SUPER) {
		status = (*me->super_state)(me, e);
	}
}
