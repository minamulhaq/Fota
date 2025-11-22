#pragma once

#include "stdint.h"

typedef enum {
    B_CMD_GET_VERSION = 0x01,
    B_CMD_GET_HELP = 0x02,
    B_CMD_GET_CID = 0x03,
    B_CMD_GET_RDP_LVL = 0x04,
    B_CMD_JMP_TO_ADDR = 0x05,
    B_CMD_ERASE_FLASH = 0x06
} BootloaderCommandId;

typedef enum {
    B_ACK = 0x0A,
    B_NACK = 0x0B
} BootloaderResponseCode;

typedef enum {
    VERIFY_CRC_SUCCESS = 0x00,
    VERIFY_CRC_FAILED = 0x01,
} CRC_VERIFICATION;

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
