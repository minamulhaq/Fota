#pragma once
#include "sm_common.h"
#include "bootloader.h"

#include "packet_controller.h"

typedef bool (*process_packet)(comms_packet_t *const last_received_packet,
			       comms_packet_t *const response_packet);

typedef enum {
	B_ACK = 0xE0,
	B_NACK = 0xE1,
	B_RETRANSMIT = 0xE2,

} bootloader_response_type_t;

typedef enum bootloader_cmd_error_codes {
	ERROR_INVALID_COMMAND = 0x11
} bootloader_cmd_error_codes_t;

typedef enum {
	B_CMD_RETRANSMIT_LAST_PACKET_TO_CLIENT = 0xB0,
	B_CMD_GET_BOOTLOADER_VERSION,
	B_CMD_GET_APP_VERSION,
	B_CMD_GET_CHIP_ID,
	B_CMD_FW_SYNC,
	B_CMD_FW_VERIFY_DEVICE_ID,
	B_CMD_FW_SEND_BIN_SIZE,
	B_CMD_FW_SEND_BIN_IN_PACKETS,
	// B_CMD_GET_HELP = 0xB2,
	// B_CMD_GET_CID = 0xB3,
	// B_CMD_GET_RDP_LVL = 0xB4,
	// B_CMD_JMP_TO_ADDR = 0xB5,
	// B_CMD_ERASE_FLASH = 0xB6,
} bootloader_packet_id_t;

typedef struct fw_update_state {
	bool started;
	bool cmd_seq_broken;
	bootloader_packet_id_t next_expected_id;
} fw_update_state_t;

typedef struct bootloader_cmd {
	uint8_t command_id;
	process_packet process;
	bool send_response;
} bootloader_cmd_t;

bootloader_cmd_t *get_command_handle(comms_packet_t const *const packet);
bootloader_cmd_t *cmd_send_retransmit_last_cmd(void);
bool bootloader_is_app_flash_finished(void);
