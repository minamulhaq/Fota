#ifndef INC_BOOTLOADER_H__
#define INC_BOOTLOADER_H__

#include <stdint.h>
#include <stdbool.h>
#include "bootloader_cmds.h"
#include "usart.h"

#define MAJOR_VERSION 1
#define MINOR_VERSION 2
#define PATCH_VERSION 3

#define BOOTLOADER_RECEIVE_BUFFER_SIZE 256

// Function pointer to the reset handler of the user application
typedef void (*app_reset_hander_t)(void);

typedef enum {
	IDLE,
	COMMAND_INITIATED,

} bootloader_state;

typedef struct {
	uint8_t length;
	uint8_t id;
	uint8_t payload_length;
	uint8_t payload[BOOTLOADER_RECEIVE_BUFFER_SIZE];
	uint8_t crc[4];
} command_packet_t;

extern uint8_t bootloader_version[3];

typedef struct BootloaderVersion {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
} BootloaderVersion;

void bootloader_check_elapsed_time(void);

void bootloader_packet_setup(void);
void bootloader_packet_update(void);
bool bootloader_is_packet_available(void);

extern uint8_t bootloader_receive_buffer[];
void bootloader_jump_to_user_app(void);
void run_bootloader_uart_statemachine(void);
CRC_VERIFICATION bootloader_verify_crc(uint8_t *buffer, uint32_t length,
				       uint32_t crc_host);

void bootloader_decide(void);

void bootloader_setup(void);

/*!< pointer to an UART callback function */
void bootloader_usart_rx_cb(UART_HandleTypeDef *huart);
void bootloader_usart_error_cb(UART_HandleTypeDef *huart);
void register_rx_it(void);

void bootloader_send_byte(const uint8_t data);
uint32_t bootloader_read_bytes(uint8_t* data, const uint32_t length);
uint8_t bootloader_read_byte(void);
void bootloader_send_bytes(uint8_t *data, uint16_t length);
bool bootlader_is_data_available(void);


#endif // INC_BOOTLOADER_H__