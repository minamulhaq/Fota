#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "bootloader.h"
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

uint8_t bootloader_receive_buffer[BOOTLOADER_RECEIVE_BUFFER_SIZE];
uint8_t bootloader_version[3] = { MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION };

static uint8_t elapsed_time = 5;
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

	while (1) {
		if (bootlader_is_data_available()) {
			uint8_t data = bootloader_read_byte();
			bootloader_send_byte(data);
		}
	}
}

void bootloader_decide(void)
{
	while (elapsed_time != 0) {
	}
	if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
		run_bootloader_uart_statemachine();
	} else {
		bootloader_jump_to_user_app();
	}
}

CRC_VERIFICATION bootloader_verify_crc(uint8_t *pData, uint32_t len,
				       uint32_t crc_host)
{
	/* Reset CRC Calculation Unit */
	uint32_t uwCRCValue = 0xff;

	for (uint32_t i = 0; i < len; i++) {
		uint32_t i_data = pData[i];
		uwCRCValue = HAL_CRC_Accumulate(&hcrc, &i_data, 1);
	}
	__HAL_CRC_DR_RESET(&hcrc);

	if (uwCRCValue == crc_host) {
		return VERIFY_CRC_SUCCESS;
	}
	return VERIFY_CRC_FAILED;
}

void bootloader_packet_setup(void)
{
}
void bootloader_packet_update(void)
{
}
bool bootloader_is_packet_available(void)
{
	return false;
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
	elapsed_time--;
	printmsg("Time to jump to app: %d | press button\r\n", elapsed_time);
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
		if (!ring_buffer_write(&rb, (uint8_t)buf)) {
			/* ERROR HANDLING */
		}
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