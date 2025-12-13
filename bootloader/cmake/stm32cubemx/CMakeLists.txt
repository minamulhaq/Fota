cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms
	USE_HAL_DRIVER
	STM32L476xx
    $<$<CONFIG:Debug>:DEBUG>
)
set(DIR_COMMON ${CMAKE_SOURCE_DIR}/../common)
set(DIR_COMMON_INC ${CMAKE_SOURCE_DIR}/../common/Inc)
set(DIR_COMMON_SRC ${CMAKE_SOURCE_DIR}/../common/Src)


set(DIR_DRIVERS ${CMAKE_SOURCE_DIR}/../Drivers)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${DIR_COMMON_INC}
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Inc
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Inc/Legacy
    ${DIR_DRIVERS}/CMSIS/Device/ST/STM32L4xx/Include
    ${DIR_DRIVERS}/CMSIS/Include
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${CMAKE_SOURCE_DIR}/Core/Src/main.c
    ${CMAKE_SOURCE_DIR}/Core/Src/gpio.c
    ${CMAKE_SOURCE_DIR}/Core/Src/crc.c
    ${CMAKE_SOURCE_DIR}/Core/Src/usart.c
    ${CMAKE_SOURCE_DIR}/Core/Src/tim.c
    ${CMAKE_SOURCE_DIR}/Core/Src/stm32l4xx_it.c
    ${CMAKE_SOURCE_DIR}/Core/Src/stm32l4xx_hal_msp.c
    ${CMAKE_SOURCE_DIR}/Core/Src/sysmem.c
    ${CMAKE_SOURCE_DIR}/Core/Src/bootloader.c
    ${CMAKE_SOURCE_DIR}/Core/Src/packet_controller.c
    ${CMAKE_SOURCE_DIR}/Core/Src/bootloader_fsm.c
    ${CMAKE_SOURCE_DIR}/Core/Src/sm_common.c
    ${CMAKE_SOURCE_DIR}/Core/Src/comms.c
    ${CMAKE_SOURCE_DIR}/Core/Src/bootloader_cmds.c
    ${CMAKE_SOURCE_DIR}/Core/Src/syscalls.c
    ${CMAKE_SOURCE_DIR}/startup_stm32l476xx.s


    ${DIR_COMMON_SRC}/msg_printer.c
    ${DIR_COMMON_SRC}/is_ringbuffer.c
    ${DIR_COMMON_SRC}/fota_api.c
)

# STM32 HAL/LL Drivers
set(STM32_Drivers_Src
    ${CMAKE_SOURCE_DIR}/Core/Src/system_stm32l4xx.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_crc.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_crc_ex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_tim.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc_ex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ramfunc.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_gpio.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c_ex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_dma.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_dma_ex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr_ex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_cortex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_exti.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart_ex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart_ex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_tim_ex.c
    ${DIR_DRIVERS}/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr.c
)

# Drivers Midllewares



# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS
    STM32_Drivers

)
# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)


# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
