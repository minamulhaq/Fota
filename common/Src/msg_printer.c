
#include <stdarg.h>
#include "usart.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

/* void printmsg(char *format, ...)
{
    char str[80];
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
    va_end(args);
}
 */