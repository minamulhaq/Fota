#pragma once
#include "sm_common.h"
#include "bootloader.h"

typedef void (*process_packet)(comms_packet_t *const last_packet,
			       comms_packet_t *const response_packet);

typedef enum {
	B_ACK = 0xE0,
	B_NACK = 0xE1,
	B_RETRANSMIT = 0xE2,

} bootloader_response_type_t;

typedef enum {
	B_CMD_GET_VERSION = 0xB1,
	B_CMD_GET_HELP = 0xB2,
	B_CMD_GET_CID = 0xB3,
	B_CMD_GET_RDP_LVL = 0xB4,
	B_CMD_JMP_TO_ADDR = 0xB5,
	B_CMD_ERASE_FLASH = 0xB6,
} bootloader_packet_id_t;

typedef struct bootloader_cmd {
	uint8_t command_id;
	process_packet process;
} bootloader_cmd_t;

bootloader_cmd_t *get_command_handle(comms_packet_t const *const packet);
bootloader_cmd_t *get_retransmit_response_handle(void);

/*

typedef uint8_t CommandID;
typedef struct bootloader_cmd bootloader_cmd;
struct bootloader_cmd {
    CommandID id;
    uint8_t response_length;
    void (*handler)(bootloader_cmd*, uint8_t*);
    void (*response_buffer)(uint8_t*);
};

void bcmd_default_handle(bootloader_cmd* cmd, uint8_t* data);

void bcmd_get_version_response_buffer(uint8_t* buffer);

void bcmd_get_help_handle(bootloader_cmd* cmd, uint8_t* data);
void bcmd_get_help_response_buffer(uint8_t* buffer);

void bcmd_get_cid_response_buffer(uint8_t* buffer);
void bcmd_get_rdp_level_response_buffer(uint8_t* buffer);

void bcmd_jump_to_addr_handle(bootloader_cmd* cmd, uint8_t* data);
void bcmd_jump_to_addr_response_buffer(uint8_t* buffer);

void bcmd_erase_flash_handle(bootloader_cmd* cmd, uint8_t* data);
void bcmd_erase_flash_response_buffer(uint8_t* buffer);

void bootloader_send_command_response(CRC_VERIFICATION v, bootloader_cmd* cmd);
extern bootloader_cmd* cmd_table[];
extern uint8_t cmd_table_size;
*/