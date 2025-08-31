/**
 * @file osal_task_config.h
 * @brief OSAL task configuration
 * @details This file contains the configuration for OSAL tasks, including task priorities,
 *          stack sizes, and other task-related settings.
 */
#ifndef OSAL_TASK_CONFIG_H
#define OSAL_TASK_CONFIG_H

#include "cyabs_rtos_impl.h"

// =======================
// Task configuration
// =======================
/*! OSAL task priority enumeration */
typedef enum {
    OSAL_IDLE_TASK_PRIORITY = 0u, /**< Minimum priority for Idle task. Reserved by OSAL (FreeRTOS) */
    OSAL_ADC_TASK_PRIORITY = CY_RTOS_PRIORITY_LOW,    /**< Low priority for ADC task */
    OSAL_MAIN_TASK_PRIORITY = CY_RTOS_PRIORITY_BELOWNORMAL,  /**< BelowNormal priority for Main task */
    OSAL_BLE_TASK_PRIORITY = CY_RTOS_PRIORITY_NORMAL,  /**< Normal priority for BLE task */
    OSAL_TIMER_TASK_PRIORITY = CY_RTOS_PRIORITY_ABOVENORMAL, /**< AboveNormal priority for Timer task */
    OSAL_BLE_HOST_TASK_PRIORITY = CY_RTOS_PRIORITY_HIGH, /**< High priority for BLE Host task. Reserved by BLE stack */
    OSAL_BLE_LL_TASK_PRIORITY = CY_RTOS_PRIORITY_REALTIME, /**< RealTime priority for BLE Link Layer task. Reserved by BLE stack */
} OSAL_TaskPriority_t;

#endif /* OSAL_TASK_CONFIG_H */

/* [] END OF FILE */
