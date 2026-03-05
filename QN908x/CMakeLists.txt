
cmake_minimum_required(VERSION 3.15.3)

# Export compile commands for IDE integration (generates compile_commands.json)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optional: print out extra messages to see what is going on. Comment it to have less verbose messages
set(CMAKE_VERBOSE_MAKEFILE ON)

# Path to toolchain file. This one has to be before 'project()' below
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/arm-none-eabi-gcc.cmake)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER_WORKS 1)

# =============================
# Build configuration selection
# =============================
option(BUILD_CONFIG "Choose build configuration: Debug or Test" "Debug")

if(BUILD_CONFIG STREQUAL "Debug")
    set(PROJECT_NAME BMS_QN9080_Debug)
    set(CONFIG_DEFINE "-DDEBUG_BUILD")
elseif(BUILD_CONFIG STREQUAL "Test")
    set(PROJECT_NAME BMS_QN9080_Test)
    set(CONFIG_DEFINE "-DTEST_BUILD")
else()
    message(FATAL_ERROR "Unknown BUILD_CONFIG: ${BUILD_CONFIG}. Use Debug or Test.")
endif()

set(EXECUTABLE ${PROJECT_NAME}.elf)
set(LINKER_FILE ${CMAKE_SOURCE_DIR}/startup/qn908x.ld)
project(${PROJECT_NAME})

enable_language(C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Optional: issue a message to be sure it uses the correct toolchain file.
message(STATUS "CMAKE_TOOLCHAIN_FILE is: ${CMAKE_TOOLCHAIN_FILE}")

# List of source files per build configuration
if(BUILD_CONFIG STREQUAL "Debug")
    set(SRC_FILES
        ../source/main/main.c
        ../source/ADC/ADC.c
        startup/startup_qn908x.S
        CMSIS/system_QN908XC.c
        drivers/fsl_adc.c
        drivers/fsl_aes.c
        drivers/fsl_calibration.c
        drivers/fsl_clock.c
        drivers/fsl_common.c
        drivers/fsl_ctimer.c
        drivers/fsl_flash.c
        drivers/fsl_flexcomm.c
        drivers/fsl_gpio.c
        drivers/fsl_i2c.c
        drivers/fsl_inputmux.c
        drivers/fsl_power.c
        drivers/fsl_reset.c
        drivers/fsl_rf.c
        drivers/fsl_rng.c
        drivers/fsl_rtc.c
        drivers/fsl_spi.c
        drivers/fsl_usart.c
        drivers/fsl_wdt.c
        board/clock_config.c
        board/gpio_pins.c
        board/pin_mux.c
        utilities/fsl_debug_console.c
        framework/LED/Source/LED.c
        framework/GPIO/GPIO_Adapter.c
        freertos/croutine.c
        freertos/event_groups.c
        freertos/fsl_tickless_systick.c
        freertos/heap_4.c
        freertos/list.c
        freertos/port.c
        freertos/queue.c
        freertos/tasks.c
        freertos/timers.c
        framework/OSAbstraction/Source/fsl_os_abstraction_free_rtos.c
        framework/Lists/GenericList.c
        framework/Panic/Source/Panic.c
        framework/NVM/Source/NV_Flash.c
        framework/Flash/Internal/Flash_Adapter.c
        framework/FunctionLib/FunctionLib.c
        framework/MemManager/Source/MemManager.c
        framework/TimersManager/Source/TimersManager.c
        framework/TimersManager/Source/TMR_Adapter.c
        framework/SecLib/SecLib.c
        framework/RNG/Source/RNG.c
        framework/Messaging/Source/Messaging.c
        ../source/HAL/QN9080/hal/hal.c
        ../source/HAL/QN9080/hal_led/hal_led.c
        ../source/HAL/QN9080/hal_gpio/hal_gpio.c
        ../source/HAL/QN9080/hal_uart/hal_uart.c
        ../source/HAL/QN9080/hal_adc/hal_adc.c
        ../source/HAL/QN9080/hal_time/hal_time.c
        ../source/HAL/QN9080/hal_ble/hal_ble.c
        ../source/qspy/qs_64bit.c
        ../source/qspy/qs_fp.c
        ../source/qspy/qs_rx.c
        ../source/qspy/qs.c
        ../source/qspy/qstamp.c
        ../source/qspyHelper/qspyHelper.c
        bluetooth/controller/interface/controller_config.c
        bluetooth/host/config/ble_globals.c
        bluetooth/hci_transport/source/hcit_serial_interface.c
        BLE_qn9080/ble_controller_task.c
        BLE_qn9080/ble_host_tasks.c
        BLE_qn9080/gatt_db/gatt_database.c
        BLE_qn9080/gatt_uuid128_defs.c
        ../source/BLE/BLE.c
        ../source/BLE/BatteryService.c
        ../source/BLE/AIOService.c
    )
elseif(BUILD_CONFIG STREQUAL "Test")
    set(SRC_FILES
        test_main.c
        startup/startup_qn908x.S
        CMSIS/system_QN908XC.c
        drivers/fsl_adc.c
        drivers/fsl_aes.c
        drivers/fsl_calibration.c
        drivers/fsl_clock.c
        drivers/fsl_common.c
        drivers/fsl_ctimer.c
        drivers/fsl_flash.c
        drivers/fsl_flexcomm.c
        drivers/fsl_gpio.c
        drivers/fsl_i2c.c
        drivers/fsl_inputmux.c
        drivers/fsl_power.c
        drivers/fsl_reset.c
        drivers/fsl_rf.c
        drivers/fsl_rng.c
        drivers/fsl_rtc.c
        drivers/fsl_spi.c
        drivers/fsl_usart.c
        drivers/fsl_wdt.c
        board/clock_config.c
        board/gpio_pins.c
        board/pin_mux.c
        utilities/fsl_debug_console.c
        framework/LED/Source/LED.c
        framework/GPIO/GPIO_Adapter.c
        freertos/croutine.c
        freertos/event_groups.c
        freertos/fsl_tickless_systick.c
        freertos/heap_4.c
        freertos/list.c
        freertos/port.c
        freertos/queue.c
        freertos/tasks.c
        freertos/timers.c
        framework/OSAbstraction/Source/fsl_os_abstraction_free_rtos.c
        framework/Lists/GenericList.c
        framework/Panic/Source/Panic.c
        framework/MemManager/Source/MemManager.c
        framework/FunctionLib/FunctionLib.c
        framework/SecLib/SecLib.c
        framework/RNG/Source/RNG.c
        framework/Messaging/Source/Messaging.c
        framework/NVM/Source/NV_Flash.c
        framework/Flash/Internal/Flash_Adapter.c
        framework/TimersManager/Source/TimersManager.c
        framework/TimersManager/Source/TMR_Adapter.c
        ../source/HAL/QN9080/hal/hal.c
        ../source/HAL/QN9080/hal_led/hal_led.c
        ../source/HAL/QN9080/hal_gpio/hal_gpio.c
        ../source/HAL/QN9080/hal_uart/hal_uart.c
        ../source/HAL/QN9080/hal_adc/hal_adc.c
        ../source/HAL/QN9080/hal_time/hal_time.c
        ../source/qspy/qs_64bit.c
        ../source/qspy/qs_fp.c
        ../source/qspy/qs_rx.c
        ../source/qspy/qs.c
        ../source/qspy/qstamp.c
        ../source/qspyHelper/qspyHelper.c
        BLE_qn9080/BLE_qn9080.c
        bluetooth/controller/interface/controller_config.c
        bluetooth/host/config/ble_globals.c
        bluetooth/hci_transport/source/hcit_serial_interface.c
        BLE_qn9080/ble_controller_task.c
        BLE_qn9080/ble_host_tasks.c
        BLE_qn9080/gatt_db/gatt_database.c
        BLE_qn9080/gatt_uuid128_defs.c
        # Add or remove files specific to Test config here
    )
else()
    message(FATAL_ERROR "Unknown BUILD_CONFIG: ${BUILD_CONFIG}. Use Debug or Test.")
endif()

# Build the executable based on the source files
add_executable(${EXECUTABLE} ${SRC_FILES})

# List of compiler defines, prefix with -D compiler option
target_compile_definitions(${EXECUTABLE} PRIVATE
        -D__STARTUP_CLEAR_BSS
        -D__MCUXPRESSO
        -D__USE_CMSIS
        -D__NEWLIB__
        -DCPU_QN9080C
        -DCPU_QN9080C_cm4
        -DDEBUG
        -DCPU_QN908X=1
        -DFSL_RTOS_FREE_RTOS
        -DCFG_BLE_PRJ=1
        -DDEBUG_BUILD_CONFIG=1

        ${CONFIG_DEFINE}
        )

# List of includ directories
target_include_directories(${EXECUTABLE} PRIVATE
        .
        startup
        CMSIS
        drivers
        framework/common
        framework/FunctionLib
        board
        utilities
        framework/RNG/Interface
        framework/GPIO
        framework/LED/Interface
        framework/TimersManager/Interface
        ../source/HAL/QN9080/hal
        ../source/HAL/QN9080/hal_led
        ../source/HAL/QN9080/hal_gpio
        ../source/HAL/QN9080/hal_uart
        ../source/HAL/QN9080/hal_adc
        ../source/HAL/QN9080/hal_time
        ../source/HAL/QN9080/hal_nvm
        ../source/HAL/QN9080/hal_ble
        ../source/main
        ../source/qspy
        ../source/qspyHelper
        ../source/qutestHelper
        ../source/OSAL/QN9080
        ../source/main
        ../source/ADC
        freertos
        framework/OSAbstraction/Interface
        framework/Lists
        framework/Panic/Interface
        framework/MemManager/Interface
        framework/FunctionLib
        framework/SecLib
        framework/Messaging/Interface
        framework/NVM/Interface
        framework/Flash/Internal
        framework/SerialManager/Interface
        framework/ModuleInfo
        bluetooth/controller/interface
        bluetooth/hci_transport/interface
        bluetooth/host/interface
        BLE_qn9080
        BLE_qn9080/gatt_db
        BLE_qn9080/gatt_db/macros
        ../source/BLE
        )

# Compiler options
target_compile_options(${EXECUTABLE} PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard

        -fdata-sections
        -ffunction-sections
        -ffreestanding
        -fno-builtin
        -fno-common
        -fmerge-constants
        -fstack-usage

        -Wall
        -O0
        -g3
        -gdwarf-4

        -include app_preinclude.h
        )

# Linker options
# __ram_vector_table__=1 tells to Linker file to allocate memory for Vectors table in RAM
target_link_options(${EXECUTABLE} PRIVATE
        -T${LINKER_FILE}
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -specs=nano.specs
        -Wl,-Map=${PROJECT_NAME}.map,--cref
        -Wl,--gc-sections
        -Wl,--sort-section=alignment
        -Wl,--print-memory-usage
        -Wl,--defsym=__ram_vector_table__=1
        -Wl,--defsym=gUseNVMLink_d=1
        -Wl,--defsym=gEraseNVMLink_d=1
        "-Wl,--just-symbols=${CMAKE_SOURCE_DIR}/board/fw_symbols_gcc.h"
        )

# Add the crypto and BLE libraries after the executable target
target_link_libraries(${EXECUTABLE} PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/lib_crypto_m4.a
        ${CMAKE_SOURCE_DIR}/libs/lib_ble_QN908XC_controller.a
        ${CMAKE_SOURCE_DIR}/libs/lib_ble_5-0_host_peripheral_cm4.a
        c
        m
        nosys
        gcc
        )

# Optional: Print executable size as part of the post build process
add_custom_command(TARGET ${EXECUTABLE}
        POST_BUILD
        COMMAND ${CMAKE_SIZE_UTIL} ${EXECUTABLE})

# Optional: Create hex, bin and S-Record files after the build
add_custom_command(TARGET ${EXECUTABLE}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O srec --srec-len=64 ${EXECUTABLE} ${PROJECT_NAME}.s19
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin)