#ifndef INC_BOOTLOADER_H__
#define INC_BOOTLOADER_H__

#include <stdint.h>
#include <stdbool.h>
#include "bootloader_cmds.h"
#include "bootloader_fsm.h"
#include "sm_common.h"
#include "usart.h"
#include "comms.h"
#include "versions.h"
#include "bl_serrif.h"

#define MAJOR_VERSION 1
#define MINOR_VERSION 2
#define PATCH_VERSION 3

#define BOOTLOADER_RECEIVE_BUFFER_SIZE 256

typedef void (*app_reset_hander_t)(void);

typedef enum {
	IDLE,
	COMMAND_INITIATED,

} bootloader_state;

extern uint8_t bootloader_version[3];

typedef struct BootloaderVersion {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
} BootloaderVersion;

void bootloader_check_elapsed_time(void);

extern uint8_t bootloader_receive_buffer[];
void bootloader_jump_to_user_app(void);
void run_bootloader_main_fsm(void);

bool bootloader_verify_crc(comms_packet_t *packet);
uint32_t bootloader_compute_crc(const comms_packet_t *const packet);

void bootloader_decide(void);

void bootloader_setup(const bl_handle_t *bl_handle);

void bootloader_byte_received(const uint8_t byte);
void bootloader_send_byte(const uint8_t data);
uint32_t bootloader_read_bytes(uint8_t *data, const uint32_t length);
void bootloader_read_byte(uint8_t *const byte);
void bootloader_send_bytes(uint8_t *data, uint16_t length);
bool bootlader_is_data_available(void);
void bootlader_send_response_packet(comms_packet_t const *packet);
void bootlader_get_last_transmitted_packet(comms_packet_t *const packet);

void bootloader_read_app_version(fw_version_t *const version);
bool bootloader_erase_shared_plus_app(void);
bool bootloader_flash_double_word(uint32_t address, uint64_t data);

#endif // INC_BOOTLOADER_H__