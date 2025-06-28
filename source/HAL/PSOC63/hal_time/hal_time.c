/**
 * @file hal_time.c
 * @brief HAL time management functions for PSoC63.
 * @note PSOC63 specific implementation details.
 * @version 0.5.0
 */

#include "hal_time.h"
#include "hal.h"

#include "cy_pdl.h"
#include "cyhal.h"

#include "FreeRTOS.h"
#include "task.h"

// ===================
// Defines
// ===================
#define SYSTICK_INTERRUPT_PRIORITY    (1U)
#define TICK_COUNT_MAX    (0xFFFFFFFFU) // Helper macro for rollover calculation

// ===================
// Private data
// ===================
static volatile uint32_t _current_time_ms = 0;
static volatile uint32_t _current_ticks = 0;
static volatile uint32_t _previous_ticks = 0;

// ===================
// Code
// ===================
void hal_time_init(void) 
{
    /** Configure SysTick for 1ms ticks */
    (void)SysTick_Config(SystemCoreClock / HAL_TICKS_PER_SEC);
    NVIC_SetPriority(SysTick_IRQn, SYSTICK_INTERRUPT_PRIORITY);
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
 */
void hal_time_delay(uint32_t ms) 
{
    /* Delay for the specified milliseconds using FreeRTOS */
    vTaskDelay(pdMS_TO_TICKS(ms));
}

//==================
// SysTick Handler
//==================
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
    _current_ticks = xTaskGetTickCount();

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
