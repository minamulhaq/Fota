#ifndef INC_BOOTLOADER_H__
#define INC_BOOTLOADER_H__

#include "bootloader_cmds.h"
#include <stdint.h>
#include "stdlib.h"
#include <stdbool.h>

#define MAJOR_VERSION 1
#define MINOR_VERSION 2
#define PATCH_VERSION 3

#define BOOTLOADER_RECEIVE_BUFFER_SIZE 128

typedef struct
{
    uint8_t length;
    uint8_t id;
    uint8_t payload_length;
    uint8_t payload[BOOTLOADER_RECEIVE_BUFFER_SIZE];
    uint8_t crc[4];
} command_packet_t;

extern uint8_t bootloader_version[3];

typedef struct BootloaderVersion
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} BootloaderVersion;
void printmsg(char *format, ...);

void bootloader_packet_setup(void);
void bootloader_packet_update(void);
bool bootloader_is_packet_available(void);
void bootloader_read(command_packet_t *packet);
void bootloader_write(command_packet_t *packet);

extern uint8_t bootloader_receive_buffer[];
void bootloader_jump_to_user_app(void);
void bootloader_uart_read(void);
CRC_VERIFICATION bootloader_verify_crc(uint8_t *buffer, uint32_t length, uint32_t crc_host);

void bootloader_decide(void);

#endif // INC_BOOTLOADER_H__