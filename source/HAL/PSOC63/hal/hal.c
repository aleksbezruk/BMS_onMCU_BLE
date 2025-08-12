/**
 * @file  hal.c
 * @brief Implementation of generic HAL functions like init board/hardware, generic utils
 * @note PSOC63
 * @version 0.5.0
 */

#include "hal.h"
#include "hal_led.h"
#include "cybsp.h"
#include "cy_pdl.h"


// =======================
// Code
// =======================
/**
 * @brief Generic API to init underlying hardware. Purpose is
 *        to configure all hardware (power, clocks, peripheral) to be ready for
 *        further BMS operations.
 * 
 * @note Configures peripherals pins as well: UART, ADC pins.
 * 
 * @details In general it may use low level BSP provided by
 *          particular Vendor if such exist and (or) use autogenerate vendor tool(s)
 *          that allow to configure hardware using convenient GUI. Using such COTS tools allow
 *          us to speed up development/prototyping process.
 * 
 * @param   None
 * 
 * @retval See \ref HAL_status_t. HAL_STATUS_OK - if success.
 */
HAL_status_t HAL_init_hardware(void)
{
    HAL_status_t status = HAL_STATUS_UNKNOWN;

    cy_rslt_t result = cybsp_init();
    if (result == CY_RSLT_SUCCESS) {
        status = HAL_STATUS_OK;
    } else {
        status = HAL_STATUS_FAIL;
    }

    return status;
}

// ==================================
// Faults handling functions
// ==================================
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} fault_stack_frame_t;

/**
 * @brief HardFault handler function
 * 
 * @details This function is called when a HardFault exception occurs.
 *          It logs the fault information and enters an infinite loop.
 *          It also blinks the red LED rapidly to indicate a hard fault.
 *
 * @param[in] stack_frame Pointer to the stack frame at the time of the fault
 * 
 * @retval None
 */
void HardFault_Handler_C(uint32_t *stack_frame) {
    volatile fault_stack_frame_t *frame = (fault_stack_frame_t *)stack_frame;
    volatile uint32_t fault_pc = frame->pc;
    volatile uint32_t fault_lr = frame->lr;
    volatile uint32_t cfsr = SCB->CFSR;  // Configurable Fault Status Register
    volatile uint32_t hfsr = SCB->HFSR;  // HardFault Status Register
    volatile uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address Register
    volatile uint32_t bfar = SCB->BFAR;   // BusFault Address Register

    // Log fault information via QSPY
    QS_BEGIN_ID(HAL, 0 /*prio/ID for local Filters*/)
        QS_STR("HardFault: PC=");
        QS_U32(0, fault_pc);
        QS_STR(" LR=");
        QS_U32(0, fault_lr);
        QS_STR(" CFSR=");
        QS_U32(0, cfsr);
        QS_STR(" HFSR=");
        QS_U32(0, hfsr);
        QS_STR(" MMFAR=");
        QS_U32(0, mmfar);
        QS_STR(" BFAR=");
        QS_U32(0, bfar);
    QS_END()
    QS_FLUSH(); // Flush QSPY output

    // Blink red LED rapidly to indicate hard fault
    HAL_LED_init_red();
    for(int i = 0; i < 10; i++) {
        HAL_LED_red_toggle();
        for(volatile int j = 0; j < 100000; j++); // Simple delay
    }

    // Set breakpoint here to examine variables in debugger
    __asm("BKPT #0");

    // Infinite loop to prevent return
    while(1) {
        HAL_LED_red_toggle();
        for(volatile int j = 0; j < 10000; j++);
    }
}

/* [] END OF FILE */
