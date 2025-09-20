/**
 * @file hal_time.c
 * @brief HAL time management functions for QN9080.
 * @note QN9080 specific implementation details.
 * @version 0.6.0
 */

#include "hal.h"
#include "hal_time.h"

#include "fsl_common.h"

#ifndef BMS_DISABLE_RTOS
// OSA
#include "fsl_os_abstraction.h"
#include "fsl_os_abstraction_free_rtos.h"

#include "osal_irq_prio.h"
#endif

// ===================
// Defines
// ===================
#define TICK_COUNT_MAX    (0xFFFFFFFFU) // Helper macro for rollover calculation

/** Debug on QSPY port */
#define HAL_TIME_DEBUG_EN   (false)

// ===================
// Private data
// ===================
static volatile uint32_t _current_time_ms = 0;
#ifndef BMS_DISABLE_RTOS
static volatile uint32_t _current_ticks = 0;
static volatile uint32_t _previous_ticks = 0;
#endif // BMS_DISABLE_RTOS

// ===================
// Code
// ===================
/**
 * @brief Initialize the HAL time management.
 * @details This function configures the SysTick timer to generate interrupts at a rate of 1ms.
 *          It sets the SysTick interrupt priority to the kernel priority defined in OSAL.
 * @param None
 * @return None
 * @note It maybe redundant to use the function since FreeRTOS already initializes the SysTick timer.
 *       However, this function is provided for completeness and to ensure the SysTick is configured
 *       correctly for the HAL time management.
 * @note This function must be called after the FreeRTOS scheduler is started. 
 */
void hal_time_init(void) 
{
    /** Configure SysTick for 1ms ticks */
    (void)SysTick_Config(SystemCoreClock / HAL_TICKS_PER_SEC);
    NVIC_SetPriority(SysTick_IRQn, OSAL_IRQ_KERNEL_PRIO);
}

/**
 * @brief Get the current time in milliseconds since device boot.
 * 
 * @details This value is derived from the FreeRTOS tick count and may not be monotonic if the tick rate changes during runtime.
 * 
 * @return Current time in milliseconds since boot, based on FreeRTOS tick count.
 */
uint32_t hal_time_get(void) 
{
    return _current_time_ms;
}

/**
 * @brief Delay for a specified number of milliseconds.
 * 
 * @param ms Delay duration in milliseconds.
 * 
 * @details This function uses FreeRTOS's vTaskDelay to suspend the task for the specified
 * duration. This function must be called from a FreeRTOS task context, not from an ISR or before the scheduler starts.
 * Note: This implementation assumes that the system clock is running and SysTick is configured correctly.
 * If FreeRTOS is not used, the delay will be a busy-wait loop, which is not power-efficient and should be avoided in production code.
 * Consider using a hardware timer or low-power sleep modes for delays in non-RTOS environments.
 * In a FreeRTOS environment, this function will yield the CPU to other tasks during the delay period,
 * allowing other tasks to run while waiting and improving system responsiveness.
 * Ensure that the FreeRTOS scheduler is running before calling this function.
 * If the scheduler is not running, this function will block indefinitely.
 */
void hal_time_delay(uint32_t ms) 
{
#ifndef BMS_DISABLE_RTOS
    /* Delay for the specified milliseconds using FreeRTOS */
    OSA_TimeDelay(ms);
#else
    /* If FreeRTOS is not used, implement a busy-wait loop for delay */
    uint32_t start_time = hal_time_get();
    while ((hal_time_get() - start_time) < ms) {
        // Busy-wait loop
        __asm volatile ("nop"); // Optional: insert NOP to reduce power consumption
    }
#endif // BMS_DISABLE_RTOS
}

//==================
// SysTick Handler
//==================
#ifndef BMS_DISABLE_RTOS
/**
 * @brief FreeRTOS weak callback: Tick hook handler.
 * 
 * This function overrides the FreeRTOS weak callback `vApplicationTickHook`.
 * It is called by the FreeRTOS kernel on every tick interrupt (SysTick).
 * To enable this hook, ensure `configUSE_TICK_HOOK` is set to 1 in FreeRTOSConfig.h.
 * This implementation updates the millisecond time counter for the HAL.
 * 
 * Note: FreeRTOS does not provide the tick count as a parameter to vApplicationTickHook,
 * so xTaskGetTickCount() is used here. If a parameter becomes available in future FreeRTOS
 * versions, use it directly to avoid the function call overhead.
 */
void vApplicationTickHook(void)
{
    // Get the current tick count from FreeRTOS
    _current_ticks = OSA_TimeGetMsec();

    // Update time & check for tick overflow and handle it
    if (_current_ticks >= _previous_ticks) {
        _current_time_ms += (_current_ticks - _previous_ticks);
    } else {
        // Rollover occurred: add the remaining ticks before rollover plus the new ticks
        _current_time_ms += (TICK_COUNT_MAX - _previous_ticks + 1U) + _current_ticks;
    }

    // Update the previous ticks for the next call
    _previous_ticks = _current_ticks;
}
#else
void SysTick_Handler(void)
{
    // Increment the millisecond counter
    _current_time_ms++;
    // If FreeRTOS is not used, we can implement a simple SysTick handler
    // that just increments the millisecond counter.
}
#endif // BMS_DISABLE_RTOS
