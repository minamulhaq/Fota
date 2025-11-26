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

uint8_t bootloader_receive_buffer[BOOTLOADER_RECEIVE_BUFFER_SIZE];
uint8_t bootloader_version[3] = { MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION };

static int8_t elapsed_time = 3;
void bootloader_jump_to_user_app(void)
{
	printmsg("bootloader_jump_to_user_app\r\n");

	/*
     * 1. Configure the MSP by reading the value from the base address of the application
     */

	__disable_irq(); // Disable interrupts
	for (int i = 0; i < 8; i++) // Clear all NVIC pending registers
		NVIC->ICPR[i] = 0xFFFFFFFF;
	__set_PRIMASK(0); // Re-enable interrupts if needed

	uint32_t msp_value =
		*(volatile uint32_t *)FLASH_SECTOR_APP_START_ADDRESS;
	printmsg("MSP value: %#x\r\n", msp_value);

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
	printmsg("Jumping to user application\r\n");
	app_reset_hander();
}

void run_bootloader_uart_statemachine(void)
{
	printmsg("run_bootloader_uart_statemachine\r\n");
	comm_state_init(NULL);
	while (1) {
		if (bootlader_is_data_available()) {
			uint8_t byte = bootloader_read_byte();
			comms_state_t state = comm_state_process_byte(&byte);
			if (state == COMM_STATE_PACKET_READY) {
				printmsg("Successfully verified packet\r\n");
			} else if (state == COMM_STATE_PACKET_INVALID) {
				printmsg("Packet invalid\r\n");
			}
		}
	}
}

void bootloader_decide(void)
{
	run_bootloader_uart_statemachine();
	while (elapsed_time > 0) {
		HAL_Delay(1);
	}

	if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
		run_bootloader_uart_statemachine();
		return; // exit after entering bootloader
	}
	bootloader_jump_to_user_app();
}

bool bootloader_verify_crc(comms_packet_t *packet)
{
	uint32_t uwCRCValue = bootloader_compute_crc(packet);
	printmsg("uwCRCValue = 0x%08lX\r\n", (unsigned long)uwCRCValue);
	printmsg("packet->crc = 0x%08lX\r\n", (unsigned long)packet->crc);

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
		printmsg("Time to jump to app: %d | press button\r\n",
			 elapsed_time);
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
	HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart2, &buf, 1);
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
		printmsg("UART Error: Overrun!\r\n");
	}

	if (huart->ErrorCode & HAL_UART_ERROR_FE) {
		printmsg("UART Error: Framing!\r\n");
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

uint8_t bootloader_read_byte(void)
{
	uint8_t buf;
	(void)bootloader_read_bytes(&buf, 1);
	return buf;
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