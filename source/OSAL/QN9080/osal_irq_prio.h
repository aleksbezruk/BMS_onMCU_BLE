/**
 * @file osal_irq_prio.h
 * @brief OSAL IRQ priority configuration
 * @details This file contains the configuration for OSAL IRQ priorities, including priority levels
 *          and other IRQ-related settings.
 * @note QN9080 specific implementation details.
 * @version 0.1.0
 */

#ifndef OSAL_IRQ_PRIO_H
#define OSAL_IRQ_PRIO_H

#include "FreeRTOSConfig.h"

/*! \brief OSAL IRQ priority levels
 *  \details This enum defines the priority levels for OSAL IRQs.
 */
typedef enum {
    /** 
     * Kernel unaware IRQ priority.
     * This priority is used for interrupts that do not interact with the FreeRTOS kernel.
     * It is set to 0, which is the highest priority.
     */
    OSAL_IRQ_KERNEL_UNAWARE_PRIO = 0,
    /** 
     * Kernel aware IRQ priority.
     * This priority is maximum priority value used for interrupts that interact with the FreeRTOS kernel.
     * It is set to the maximum syscall interrupt priority defined in FreeRTOSConfig.h.
     */
    OSAL_IRQ_MAX_SYSCALL_PRIO = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,
    /** 
     * UART IRQ priority.
     * This priority is used for UART (QSPY) interrupts.
     * It is set to the maximum syscall interrupt priority plus one.
     */
    OSAL_IRQ_UART_PRIO = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1,
    /** 
     * Kernel IRQ priority.
     * This priority is used for interrupts that require the FreeRTOS kernel.
     * It is set to the lowest priority value defined in FreeRTOSConfig.h.
     * This priority is used for the kernel's own interrupts, such as PendSV and SysTick.
     */
    OSAL_IRQ_KERNEL_PRIO = configLIBRARY_LOWEST_INTERRUPT_PRIORITY,
} OSAL_IRQ_Priority_t;

#endif // OSAL_IRQ_PRIO_H