
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

uint8_t bootloader_receive_buffer[BOOTLOADER_RECEIVE_BUFFER_SIZE];
uint8_t bootloader_version[3] = {MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION};

void printmsg(char *format, ...) {
    char str[80];
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
    va_end(args);
}

void bootloader_jump_to_user_app(void) {
    // Function pointer to the reset handler of the user application
    void (*app_reset_hander)(void);
    printmsg("bootloader_jump_to_user_app\r\n");

    /*
     * 1. Configure the MSP by reading the value from the base address of the application
     */
    uint32_t msp_value = *(volatile uint32_t *)FLASH_SECTOR2_BASE_ADDRESS;
    printmsg("MSP value: %#x\r\n", msp_value);

    __set_MSP(msp_value);

    /*
     * 2. Now fetch the reset handler address of the user application
     * from the location FLASH_SECTOR2_BASE_ADDRESS + 4
     */
    uint32_t reset_handler_address = *(volatile uint32_t *)(FLASH_SECTOR2_BASE_ADDRESS + 4);
    app_reset_hander = (void (*)(void))reset_handler_address;

    /*
     * 3. Jump to application reset handler
     */
    printmsg("Jumping to user application\r\n");
    app_reset_hander();
}
void bootloader_uart_read(void) {
    printmsg("bootloader_uart_read\r\n");
    uint8_t receive_length = 0;

    while (1) {
        memset(bootloader_receive_buffer, 0, BOOTLOADER_RECEIVE_BUFFER_SIZE);
        HAL_UART_Receive(&huart2, bootloader_receive_buffer, 1, HAL_MAX_DELAY);
        receive_length = bootloader_receive_buffer[0];
        if (HAL_OK == HAL_UART_Receive(&huart2, &bootloader_receive_buffer[1], receive_length, 2000)) {
            CommandID cmd_id = bootloader_receive_buffer[1];
            uint8_t valid_cmd = 0;
            for (int i = 0; i < cmd_table_size; i++) {
                if (cmd_table[i]->id == cmd_id) {
                    cmd_table[i]->handler(cmd_table[i], bootloader_receive_buffer);
                    valid_cmd = 1;
                    break;
                }
            }
            if (!valid_cmd) {
                char msg[] = "Invalid command\n";
                HAL_UART_Transmit(&huart2, (uint8_t *)msg, sizeof(msg), HAL_MAX_DELAY);
            }
        }
    }
}

void bootloader_decide(void) {
    if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
        bootloader_uart_read();
    } else {
        bootloader_jump_to_user_app();
    }
}

CRC_VERIFICATION bootloader_verify_crc(uint8_t *pData, uint32_t len, uint32_t crc_host) {
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

void bootloader_packet_setup(void) {
}
void bootloader_packet_update(void) {
}
bool bootloader_is_packet_available(void) {
    return false;
}
void bootloader_read(command_packet_t *packet) {
}
void bootloader_write(command_packet_t *packet) {
}
