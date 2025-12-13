#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "bootloader_cmds.h"
#include "crc.h"
#include "gpio.h"
#include "main.h"
#include "stm32l4xx_hal.h"
#include "usart.h"
#include "flash.h"
#include "msg_printer.h"
#include "is_ringbuffer.h"
#include "stm32l4xx_hal_usart.h"
#include "comms.h"
#include "bootloader_fsm.h"
#include "fota_api.h"
#include "stm32l4xx_hal_flash.h"
#include "stm32l4xx_hal_gpio.h"

uint8_t bootloader_receive_buffer[BOOTLOADER_RECEIVE_BUFFER_SIZE];
uint8_t bootloader_version[3] = { MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION };
static comms_packet_t last_sent_packet = { 0 };

static int8_t elapsed_time = 3;

#define FLASH_ERASED_VALUE 0xFFFFFFFFU

static bool is_msp_valid(uint32_t msp_val)
{
	/* Check for erased/zero */
	if (msp_val == 0x00000000UL || msp_val == FLASH_ERASED_VALUE)
		return false;

	/* Must be 4-byte aligned */
	if (msp_val & 0x3U)
		return false;

	/* Calculate RAM boundaries */
	const uint32_t SRAM1_START = SRAM1_BASE;
	const uint32_t SRAM1_END =
		SRAM1_BASE + SRAM1_SIZE_MAX; /* Changed: removed -1 */
	const uint32_t SRAM2_START = SRAM2_BASE;
	const uint32_t SRAM2_END =
		SRAM2_BASE + SRAM2_SIZE; /* Changed: removed -1 */

	/* MSP can point to top of RAM (one past last valid address) */
	bool in_sram1_range = (msp_val >= SRAM1_START) &&
			      (msp_val <= SRAM1_END);
	bool in_sram2_range = (msp_val >= SRAM2_START) &&
			      (msp_val <= SRAM2_END);

	return (in_sram1_range || in_sram2_range);
}

void bootloader_jump_to_user_app(void)
{
	/*
     * 1. Configure the MSP by reading the value from the base address of the application
     */

	uint32_t msp = *(volatile uint32_t *)FLASH_SECTOR_APP_START_ADDRESS;
	if (!is_msp_valid(msp)) {
		return;
	}

	__disable_irq(); // Disable interrupts
	for (int i = 0; i < 8; i++) // Clear all NVIC pending registers
		NVIC->ICPR[i] = 0xFFFFFFFF;
	__set_PRIMASK(0); // Re-enable interrupts if needed

	uint32_t msp_value =
		*(volatile uint32_t *)FLASH_SECTOR_APP_START_ADDRESS;

	__set_MSP(msp_value);

	/*
     * 2. Now fetch the reset handler address of the user application
     * from the location FLASH_SECTOR_APP_START + 4
     */
	uint32_t reset_handler_address =
		*(volatile uint32_t *)(FLASH_SECTOR_APP_START_ADDRESS + 4);
	app_reset_hander_t app_reset_hander =
		(void (*)(void))reset_handler_address;

	/*
     * 3. Jump to application reset handler
     */
	app_reset_hander();
}

static bootloader_fsm_t bootloader_fsm = { 0 };

extern Event byte_received_event;
extern Event event_packet_valid;
extern Event event_packet_invalid;
extern Event entry_event;
extern Event exit_event;
extern Event init_event;

void run_bootloader_main_fsm(void)
{
	bootloader_fsm_ctr(&bootloader_fsm, (StateHandler)&comm_state_init);
	Fsm_init((Fsm *)&bootloader_fsm, NULL);

	while (1) {
		switch (bootloader_fsm.packet_status) {
		case SIGNAL_PACKET_NOT_READY: {
			if (bootlader_is_data_available()) {
				bootloader_read_byte(&bootloader_fsm.uart_byte);
				Fsm_dispatch((Fsm *)&bootloader_fsm,
					     &byte_received_event);
			}

			break;
		}

		case SIGNAL_PACKET_VALID: {
			Fsm_dispatch((Fsm *)&bootloader_fsm,
				     &event_packet_valid);
			break;
		}

		case SIGNAL_PACKET_INVALID: {
			Fsm_dispatch((Fsm *)&bootloader_fsm,
				     &event_packet_invalid);
			break;
		}

		default: {
			break;
		}
		}
	}
}

void bootloader_decide(void)
{
	while (elapsed_time > 0) {
		HAL_Delay(1);
	}

	if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
		// TODO: Return ack or nack
		bootloader_jump_to_user_app();
	}
	run_bootloader_main_fsm();
}

bool bootloader_verify_crc(comms_packet_t *packet)
{
	uint32_t uwCRCValue = bootloader_compute_crc(packet);
	// printmsg("uwCRCValue = 0x%08lX\r\n", (unsigned long)uwCRCValue);
	// printmsg("packet->crc = 0x%08lX\r\n", (unsigned long)packet->crc);

	if (uwCRCValue == packet->crc) {
		return true;
	}
	return false;
}

uint32_t bootloader_compute_crc(const comms_packet_t *const packet)
{
	if (packet == NULL) {
		return 0;
	}

	/* Prepare a small stack buffer containing only fields covered by CRC:
     * [ command_id ][ length ][ payload (only if length>0) ]
     */
	uint8_t buf[2 + MAX_PAYLOAD_SIZE];
	uint32_t idx = 0;

	/* command_id */
	buf[idx++] = packet->command_id;

	/* length */
	buf[idx++] = packet->length;

	/* payload only when length > 0 and within bounds */
	if (packet->length > 0 && packet->length <= MAX_PAYLOAD_SIZE) {
		for (uint32_t i = 0; i < packet->length; ++i) {
			buf[idx++] = packet->payload[i];
		}
	}

	/* Call your existing crc32() exactly as-is */
	return crc32(buf, idx);
}

extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;

/**
  * @brief  Period elapsed callback in non-blocking mode
  * @param  htim: TIM handle structure
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM6) {
		bootloader_check_elapsed_time();
	}
}

void bootloader_check_elapsed_time(void)
{
	if (elapsed_time > 0) {
		elapsed_time--;
	}
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}

static uint8_t buf;
static ring_buffer_t rb;
static uint8_t usart_buf[BOOTLOADER_RECEIVE_BUFFER_SIZE] = { 0U };

void bootloader_setup(void)
{
	ring_buffer_setup(&rb, usart_buf, BOOTLOADER_RECEIVE_BUFFER_SIZE);

	HAL_UART_RegisterCallback(&huart2, HAL_UART_RX_COMPLETE_CB_ID,
				  bootloader_usart_rx_cb);
	HAL_UART_RegisterCallback(&huart2, HAL_UART_ERROR_CB_ID,
				  bootloader_usart_error_cb);
	register_rx_it();
}

void register_rx_it(void)
{
	HAL_UART_Receive_IT(&huart2, &buf, 1);
}

void bootloader_usart_rx_cb(UART_HandleTypeDef *huart)
{
	if (&huart2 == huart) {
		ring_buffer_write(&rb, (uint8_t)buf);
		register_rx_it();
	}
}

void bootloader_usart_error_cb(UART_HandleTypeDef *huart)
{
	if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
	}

	if (huart->ErrorCode & HAL_UART_ERROR_FE) {
	}

	// The HAL's internal IRQ handler clears the error flags and attempts
	// to put the peripheral back into a ready state before calling this.
	// If you need to re-arm reception, consider calling register_rx_it() here
	// only if the error state has been fully resolved by the HAL.

	register_rx_it();
	huart->ErrorCode =
		HAL_UART_ERROR_NONE; // Clear the error status after handling
}

void bootloader_send_byte(const uint8_t data)
{
	HAL_UART_Transmit(&huart2, &data, 1, 100);
}

void bootloader_send_bytes(uint8_t *data, uint16_t length)
{
	HAL_UART_Transmit(&huart2, data, length, 100);
}

bool bootlader_is_data_available(void)
{
	return !ring_buffer_empty(&rb);
}

void bootloader_read_byte(uint8_t *const byte)
{
	bootloader_read_bytes(byte, 1);
}
uint32_t bootloader_read_bytes(uint8_t *data, const uint32_t length)
{
	if (length == 0) {
		return 0;
	}
	for (uint32_t bytes_read = 0; bytes_read < length; bytes_read++) {
		if (!ring_buffer_read(&rb, &data[bytes_read])) {
			/* Error Handling */
			return bytes_read;
		}
	}
	return length;
}

static volatile int x = 0;
void bootlader_send_response_packet(comms_packet_t const *packet)
{
	// send command id
	HAL_UART_Transmit(&huart2, (uint8_t *)&packet->command_id,
			  sizeof(packet->command_id), HAL_MAX_DELAY);

	// send length
	HAL_UART_Transmit(&huart2, (uint8_t *)&packet->length,
			  sizeof(packet->length), HAL_MAX_DELAY);

	// send payload only if length > 0
	if (packet->length > 0) {
		// safety: never transmit beyond MAX_PAYLOAD_SIZE
		uint8_t len = packet->length <= MAX_PAYLOAD_SIZE ?
				      packet->length :
				      MAX_PAYLOAD_SIZE;

		HAL_UART_Transmit(&huart2, (uint8_t *)packet->payload, len,
				  HAL_MAX_DELAY);
	}

	HAL_UART_Transmit(&huart2, (uint8_t *)&packet->crc, sizeof(packet->crc),
			  HAL_MAX_DELAY);

	memcpy(&last_sent_packet, packet, sizeof(comms_packet_t));
	return;
}

void bootlader_get_last_transmitted_packet(comms_packet_t *const packet)
{
	memcpy(packet, &last_sent_packet, sizeof(comms_packet_t));
}

void bootloader_read_app_version(version_t *const version)
{
	fota_api_get_app_version(version);
}

bool bootloader_erase_shared_plus_app(void)
{
	uint32_t error;
	FLASH_EraseInitTypeDef erase = { .TypeErase = FLASH_TYPEERASE_PAGES,
					 .Banks = FOTA_SHARED_APP_BANK,
					 .Page = FOTA_SHARED_APP_PAGE,
					 .NbPages = FOTA_SHARED_APP_NBPAGES

	};
	HAL_FLASH_Unlock();
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	HAL_StatusTypeDef ret = HAL_FLASHEx_Erase(&erase, &error);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	// HAL_FLASH_Lock();
	return ret == HAL_OK ? true : false;
}

bool bootloader_flash_double_word(uint32_t address, uint64_t data)
{
	// HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
	// HAL_FLASH_Lock();
	return true;
}