
#include "bootloader.h"
#include "bootloader_cmds.h"
#include "crc.h"
#include "gpio.h"
#include "main.h"
#include "memory.h"
#include "stdarg.h"
#include "stdio.h"
#include "stm32l4xx_hal.h"
#include "string.h"
#include "usart.h"
#include "flash.h"
#include "msg_printer.h"
#include <stdint.h>

uint8_t bootloader_receive_buffer[BOOTLOADER_RECEIVE_BUFFER_SIZE];
uint8_t bootloader_version[3] = { MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION };

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

void bootloader_uart_read(void)
{
	printmsg("bootloader_uart_read\r\n");
	uint8_t receive_length = 0;

	while (1) {
		memset(bootloader_receive_buffer, 0,
		       BOOTLOADER_RECEIVE_BUFFER_SIZE);
		HAL_UART_Receive(&huart2, bootloader_receive_buffer, 1,
				 HAL_MAX_DELAY);
		receive_length = bootloader_receive_buffer[0];
		if (HAL_OK == HAL_UART_Receive(&huart2,
					       &bootloader_receive_buffer[1],
					       receive_length, 2000)) {
			CommandID cmd_id = bootloader_receive_buffer[1];
			uint8_t valid_cmd = 0;
			for (int i = 0; i < cmd_table_size; i++) {
				if (cmd_table[i]->id == cmd_id) {
					cmd_table[i]->handler(
						cmd_table[i],
						bootloader_receive_buffer);
					valid_cmd = 1;
					break;
				}
			}
			if (!valid_cmd) {
				char msg[] = "Invalid command\n";
				HAL_UART_Transmit(&huart2, (uint8_t *)msg,
						  sizeof(msg), HAL_MAX_DELAY);
			}
		}
	}
}

void bootloader_decide(void)
{
	if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
		bootloader_uart_read();
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
		bootloader_toggle_led();
	}
}

void bootloader_toggle_led(void)
{
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}
