cmake_minimum_required(VERSION 3.15.3)

# Optional: print out extra messages to see what is going on. Comment it to have less verbose messages
set(CMAKE_VERBOSE_MAKEFILE ON)

# Path to toolchain file. This one has to be before 'project()' below
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/arm-none-eabi-gcc.cmake)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER_WORKS 1)

# Setup project, output and linker file
project(BMS_QN908x)
set(EXECUTABLE ${PROJECT_NAME}.elf)
set(LINKER_FILE ${CMAKE_SOURCE_DIR}/startup/qn908x.ld)

enable_language(C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Optional: issue a message to be sure it uses the correct toolchain file.
message(STATUS "CMAKE_TOOLCHAIN_FILE is: ${CMAKE_TOOLCHAIN_FILE}")

# List of source files
set(SRC_FILES
        test_main.c
        startup/startup_qn908x.S
        CMSIS/system_QN908XC.c
        drivers/fsl_adc.c
        #drivers/fsl_aes.c
        drivers/fsl_calibration.c
        drivers/fsl_clock.c
        drivers/fsl_common.c
        drivers/fsl_ctimer.c
        drivers/fsl_flash.c
        drivers/fsl_flexcomm.c
        drivers/fsl_gpio.c
        #drivers/fsl_i2c_freertos.c
        drivers/fsl_i2c.c
        drivers/fsl_inputmux.c
        drivers/fsl_power.c
        drivers/fsl_reset.c
        drivers/fsl_rf.c
        drivers/fsl_rng.c
        drivers/fsl_rtc.c
        #drivers/fsl_spi_freertos.c
        drivers/fsl_spi.c
        #drivers/fsl_usart_freertos.c
        drivers/fsl_usart.c
        drivers/fsl_wdt.c
        #framework/common/FwkInit.c
        #framework/FunctionLib/FunctionLib.c
        #board/board.c
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
        )

# Build the executable based on the source files
add_executable(${EXECUTABLE} ${SRC_FILES})

# List of compiler defines, prefix with -D compiler option
target_compile_definitions(${EXECUTABLE} PRIVATE
        -D__STARTUP_CLEAR_BSS
        -D__ATOLLIC__
        -DCPU_QN9080C
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
        ../source/main
        ../source/qspy
        ../source/qspyHelper
        ../source/qutestHelper
        ../source/OSAL/QN9080
        freertos
        framework/OSAbstraction/Interface
        framework/Lists
        framework/Panic/Interface
        )

# Compiler options
target_compile_options(${EXECUTABLE} PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard

        -fdata-sections
        -ffunction-sections

        -Wall
        -O0
        -g3

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
        -lc
        -lm
        -lnosys
        -Wl,-Map=${PROJECT_NAME}.map,--cref
        -Wl,--gc-sections
        -Xlinker -print-memory-usage -Xlinker
        -Wl,--defsym=__ram_vector_table__=1
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