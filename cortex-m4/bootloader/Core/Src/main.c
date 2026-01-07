/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "bootloader.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

UART_HandleTypeDef *fota_uart = &huart2;

uint8_t dmadata[MAX_PAYLOAD_SIZE * 2];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void bl_send_bytes(const uint8_t *data, const uint32_t size)
{
	HAL_UART_Transmit(fota_uart, data, size, 100);
}

void bl_deinit(void)
{
	__disable_irq();
	for (int i = 0; i < 8; i++) // Clear all NVIC pending registers
		NVIC->ICPR[i] = 0xFFFFFFFF;
	__set_PRIMASK(0); // Re-enable interrupts if needed
	HAL_UART_AbortReceive(fota_uart);
}
void setup_cb(void)
{
	// 1. Abort any existing activity to reset the State Machine
	HAL_UART_AbortReceive(fota_uart);

	// 2. Clear all Interrupt Flags (Specifically IDLE and Overrun)
	// Writing to ICR (Interrupt Flag Clear Register)
	__HAL_UART_CLEAR_FLAG(fota_uart, UART_CLEAR_IDLEF | UART_CLEAR_OREF |
						 UART_CLEAR_NEF |
						 UART_CLEAR_FEF);

	// 3. Flush the RDR (Receive Data Register)
	// Reading the register multiple times ensures the hardware FIFO is empty
	volatile uint32_t dummy_read;
	while (__HAL_UART_GET_FLAG(fota_uart, UART_FLAG_RXNE)) {
		dummy_read = fota_uart->Instance->RDR;
		(void)dummy_read; // Prevent compiler optimization
	}

	// 4. Start DMA Reception
	// DO NOT manually call __HAL_UART_ENABLE_IT(&fota_uart, UART_IT_IDLE);
	// HAL_UARTEx_ReceiveToIdle_DMA already enables the required interrupts.
	if (HAL_UARTEx_ReceiveToIdle_DMA(fota_uart, dmadata, sizeof(dmadata)) !=
	    HAL_OK) {
		Error_Handler();
	}

	// Optional: Disable Half-Transfer interrupt to avoid double-triggering
	__HAL_DMA_DISABLE_IT(fota_uart->hdmarx, DMA_IT_HT);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	MX_CRC_Init();
	MX_TIM6_Init();
	MX_TIM7_Init();
	/* USER CODE BEGIN 2 */

	setup_cb();

	bl_handle_t bl_handle = {

		.serrif = { .wb = bl_send_bytes },
		.deinit = bl_deinit
	};
	bootloader_setup(&bl_handle);
	bootloader_decide();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
  */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) !=
	    HAL_OK) {
		Error_Handler();
	}

	/** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 10;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
  */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
				      RCC_CLOCKTYPE_SYSCLK |
				      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) !=
	    HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == fota_uart->Instance) {
		uint32_t error_code = HAL_UART_GetError(huart);

		// Framing Error (FE) Detected
		if (error_code & HAL_UART_ERROR_FE) {
			// A framing error often means the baud rate is slightly off
			// or the line was disconnected/glitched.

			// 1. Clear the FE flag by writing to ICR
			__HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF);

			// 2. Read the data register to flush the 'bad' byte
			volatile uint32_t dummy = huart->Instance->RDR;
			(void)dummy;

			// 3. Re-sync the DMA
			setup_cb();
		}

		// Handle Overrun (ORE) - common if ESP32 sends while STM32 is in ISR
		if (error_code & HAL_UART_ERROR_ORE) {
			__HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF);
			setup_cb();
		}
	}
}

/**
 * @brief  Reception Event Callback (Handles IDLE and Complete events)
 * @param  huart: UART handle
 * @param  Size: Number of bytes actually received

 
 "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x " ((uint8_t*)dmadata)[0] ((uint8_t*)dmadata)[1] ((uint8_t*)dmadata)[2] ((uint8_t*)dmadata)[3] ((uint8_t*)dmadata)[4] ((uint8_t*)dmadata)[5] ((uint8_t*)dmadata)[6] ((uint8_t*)dmadata)[7] ((uint8_t*)dmadata)[8] ((uint8_t*)dmadata)[9] ((uint8_t*)dmadata)[10] ((uint8_t*)dmadata)[11] ((uint8_t*)dmadata)[12] ((uint8_t*)dmadata)[13] ((uint8_t*)dmadata)[14] ((uint8_t*)dmadata)[14]
 */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	(void)Size;
	if (huart->Instance == fota_uart->Instance) {
		uint16_t received = sizeof(dmadata) -
				    __HAL_DMA_GET_COUNTER(fota_uart->hdmarx);
		for (size_t i = 0; i < received; i++) {
			bootloader_byte_received(dmadata[i]);
		}
		setup_cb();
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
	(void)file;
	(void)line;
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */