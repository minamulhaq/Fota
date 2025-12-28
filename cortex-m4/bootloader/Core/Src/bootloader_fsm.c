
#include "bootloader_fsm.h"
#include "bootloader.h"
#include "msg_printer.h"

extern Event byte_received_event;
extern Event event_sync_requested;
extern Event entry_event;
extern Event exit_event;
extern Event init_event;

static fw_update_state_t fwupdatestate = {
	.started = false,
	.next_expected_id = B_CMD_FW_SYNC,
	.cmd_seq_broken = false,
};

bool set_fw_update_next_handle(const bootloader_packet_id_t id)
{
	bool switch_to_fsm_handler = false;

	if (id < B_CMD_FW_SYNC || id > B_CMD_FW_SEND_BIN_IN_PACKETS) {
		return switch_to_fsm_handler;
	}

	if (fwupdatestate.started) {
		if (fwupdatestate.next_expected_id != id) {
			fwupdatestate.cmd_seq_broken = true;
			fwupdatestate.started = false;
			switch_to_fsm_handler = false;
		} else {
			fwupdatestate.started = true;
			fwupdatestate.cmd_seq_broken = false;
			switch_to_fsm_handler = true;
		}
	} else {
		if (id == B_CMD_FW_SYNC) {
			fwupdatestate.started = true;
			fwupdatestate.cmd_seq_broken = false;
			switch_to_fsm_handler = true;
		}
	}
	return switch_to_fsm_handler;
}

void bootloader_fsm_ctr(bootloader_fsm_t *me, StateHandler initial)
{
	Fsm_ctor(&me->fsm, initial);
}

static bool bootloader_handle_packet(void)
{
	bool status = false;
	comms_packet_t *last_received_packet = comm_get_last_packet();
	bootloader_cmd_t *handle = get_command_handle(last_received_packet);
	if (handle != NULL) {
		comms_packet_t response_packet = { 0 };
		status =
			handle->process(last_received_packet, &response_packet);
		if (handle->send_response) {
			bootlader_send_response_packet(&response_packet);
		}
	}
	return status;
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
		// me->packet_status = SIGNAL_PACKET_NOT_READY;
		status = STATE_HANDLED;
		break;
	}

	case SIGNAL_PACKET_VALID: {
		comms_packet_t *last_received_packet = comm_get_last_packet();
		bootloader_cmd_t *handle =
			/* TODO: Set timer and send finished packet to end flash process*/
			get_command_handle(last_received_packet);

		if (set_fw_update_next_handle(handle->command_id)) {
			status = FSM_TRANSIT_TO(
				bootloader_fw_update_sync_request);
		} else {
			if (true == fwupdatestate.cmd_seq_broken) {
				fwupdatestate.next_expected_id = B_CMD_FW_SYNC;
				fwupdatestate.started = false;
			}
			bootloader_handle_packet();
			// if (handle != NULL) {
			// 	comms_packet_t response_packet = { 0 };
			// 	handle->process(last_received_packet,
			// 			&response_packet);
			// 	bootlader_send_response_packet(
			// 		&response_packet);
			// }
			status = FSM_TRANSIT_TO(comm_state_id);
		}
		break;
	}

	case SIGNAL_PACKET_INVALID: {
		bootloader_cmd_t *handle = cmd_send_retransmit_last_cmd();
		if (handle == NULL) {
			status = FSM_TRANSIT_TO(comm_state_id);
		} else {
			bootloader_handle_packet();
			// comms_packet_t response_packet = { 0 };
			// handle->process(NULL, &response_packet);
			// bootlader_send_response_packet(&response_packet);
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

status_t bootloader_fw_update_sync_request(bootloader_fsm_t *me,
					   Event const *const e)
{
	status_t status;
	switch (e->sig) {
	case SIGNAL_PACKET_VALID: {
		bootloader_handle_packet();
		me->packet_status = SIGNAL_PACKET_NOT_READY;
		status = FSM_TRANSIT_TO(comm_state_id);
		break;
	}
	case SIGNAL_ENTRY: {
		fwupdatestate.cmd_seq_broken = false;
		fwupdatestate.started = true;
		status = STATE_HANDLED;
		break;
	}
	case SIGNAL_EXIT: {
		bootloader_packet_id_t id;

		switch (fwupdatestate.next_expected_id) {
		case B_CMD_FW_SYNC: {
			id = B_CMD_FW_VERIFY_DEVICE_ID;
			break;
		}

		case B_CMD_FW_VERIFY_DEVICE_ID: {
			id = B_CMD_FW_SEND_BIN_SIZE;
			break;
		}

		case B_CMD_FW_SEND_BIN_SIZE: {
			id = B_CMD_FW_SEND_BIN_IN_PACKETS;
			break;
		}

		case B_CMD_FW_SEND_BIN_IN_PACKETS: {
			id = bootloader_is_app_flash_finished() ?
				     B_CMD_FW_SEND_BIN_IN_PACKETS :
				     B_CMD_FW_SYNC;
			break;
		}

		default: {
			id = B_CMD_FW_SYNC;
			break;
		}
		}
		fwupdatestate.next_expected_id = id;
	}
	}
	return status;
}
