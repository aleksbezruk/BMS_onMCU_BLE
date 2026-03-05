/**
 * @file OSAL.h
 * 
 * @brief Operating System Abstraction layer
 * 
 * @details This file provides the OSAL interface for the QN9080 platform.
 *          It includes necessary headers and defines the OSAL functions.
 *          The OSAL layer provides a consistent interface for task management,
 *          queue management, and other OS-related functionalities.
 * 
 * @note This OSAL implementation uses xmacroes to define tasks and queues,
 *       allowing for a more flexible and maintainable codebase. Also usage of xmacroes
 *       avoids function calls overhead and allows for better optimization by the compiler 
 *       and reduce stack usage.
 * 
 * @note NXP QN9080 OSAL implementation.
 * 
 * @version 0.7.0
 */

#ifndef OSAL_H
#define OSAL_H

// =======================
// Includes
// =======================
// NXP OSA
#include "fsl_os_abstraction.h"
#include "fsl_os_abstraction_free_rtos.h"

// FreeRTOS
#include "FreeRTOS.h"

#include "osal_task_config.h"

// =======================
// Defines
// =======================
/*! OSAL status enumeration */
typedef enum {
    OSAL_SUCCESS,
    OSAL_FAILURE,
    OSAL_TIMEOUT
} OSAL_Status_t;

// =======================
// Task functionality
// =======================
/*! OSAL task argument */
typedef osaTaskParam_t OSAL_arg_t;

/*! OSAL thread function */
typedef void (*OSAL_ThreadFunc_t)(OSAL_arg_t arg);

/*! OSAL task define
 * @brief OSAL task define
 * @details This macro defines a task in the OSAL layer.
 *
 * @param[in] taskHandle Name of the task handle to be defined.
 */
#define OSAL_TASK_DEFINE(taskHandle) \
    static osaTaskId_t OSAL_##taskHandle = NULL

/*!
 * @brief OSAL task get handle
 * @details This macro retrieves the handle of a task defined in the OSAL layer.
 *
 * @param[in] taskHandle Name of the task handle to retrieve.
 * @return Pass through for QN9080 (nothing returned). 
 *         Used for consistency with other OSAL implementations.
 */
#define OSAL_TASK_GET_HANDLE(taskHandle) OSAL_##taskHandle

/*! 
 * @brief OSAL task creation
 *
 * @details This macro creates a task in the OSAL layer.
 * 
 * @param[in,out] pTaskHandle pointer to Handle for the created task
 * @param[in] taskFunc Function to be executed by the task. Not used in this implementation.
 *            Memory for the task function is allocated by the FreeRTOS itself in heap.
 * @param[in] taskName Name of the task
 * @param[in] pStack Pointer to the stack memory for the task
 * @param[in] stackSize Stack size for the task
 * @param[in] priority Priority of the task
 * @param[in] arg Argument to be passed to the task
 * @param[out] status Status of the task creation
 */
#define OSAL_TASK_CREATE(pTaskHandle, taskFunc, taskName, pStack, stackSize, priority, arg, status) \
    OSA_TASK_DEFINE(taskFunc, priority, 1, stackSize, false); \
    pTaskHandle = OSA_TaskCreate(OSA_TASK(taskFunc), (osaTaskParam_t)arg); \
    if (pTaskHandle == NULL) { \
        status = OSAL_FAILURE; \
    } else { \
        status = OSAL_SUCCESS; \
    }
/*! 
 * @brief OSAL task delay
 * @details This macro delays the execution of the current task for a specified number of milliseconds.
 * @param[in] ms Number of milliseconds to delay
 */
#define OSAL_TASK_DELAY(ms) \
    OSA_TimeDelay(ms)

// =======================
// Queue functionality
// =======================
/*! OSAL queue never timeout definition */
#define OSAL_QUEUE_TIMEOUT_NEVER osaWaitForever_c

/*! 
 * @brief OSAL create queue handle
 * @details This macro defines a queue handle in the OSAL layer.
 *          It allocates memory for a static queue handle and a queue structure.
 * @note The queue handle is a static variable of type `osaMsgQId_t`.
 * 
 * @param[in] queueHandle Name of the queue handle to be defined.
 * @note The queue handle is used to manage the queue in the OSAL layer.
 *       It is a static variable that can be used across the OSAL layer.
 *       The queue structure is used to hold the queue items and their metadata.
 *       This macro should be used to define a queue before it is created.
 *       It is typically used in the OSAL layer to create queues for inter-task communication.
 *       The queue handle can be used to create, send, receive, and delete items in the queue.
 *       The queue structure holds the necessary information for the queue operations.
 *
 * @note This macro should be used in conjunction with `OSAL_QUEUE_CREATE` to create the queue.
 *
 * @return None
 */
#define OSAL_QUEUE_DEFINE(queueHandle) \
    static osaMsgQId_t OSAL_##queueHandle = NULL

/*! OSAL get queue handle
 * @details This macro retrieves the handle of a queue defined in the OSAL layer.
 * @note The handle is a pointer to the static variable defined by `OSAL_QUEUE_DEFINE`.
 * @param[in] queueHandle Name of the queue handle to retrieve.
 * @return Pointer to the queue handle.
 */
#define OSAL_QUEUE_GET_HANDLE(queueHandle) \
    (osaMsgQId_t)OSAL_##queueHandle

/*!
 * @brief OSAL queue get static handle
 * @details This macro retrieves the static handle of a queue defined in the OSAL layer.
 * @note The static handle is a pointer to the static variable defined by `OSAL_QUEUE_DEFINE`.
 * @param[in] queueHandle Name of the queue handle to retrieve.
 *
 * @attention Static queue isn't implemented in this OSAL layer for QN9080.
 *
 * @return Pointer to the static queue handle.
 */
#define OSAL_QUEUE_GET_STATIC_HANDLE(queueHandle) (void) 0  // RFU, placeholder for future implementation

/*! 
 * @brief OSAL queue create
 *
 * @details This macro creates a queue in the OSAL layer.
 *
 * @param[in,out] queueHandle Handle for the created queue
 * @param[in] queueName Name of the queue. For now it is not used, but can be useful for debugging.
 * @param[in] queueLength Length of the queue (number of items it can hold)
 * @param[in] queueItemSize Size of each item in the queue
 * @param[in] queueStorage Storage for the queue items
 * @param[out] status Status of the queue creation
 */
#define OSAL_QUEUE_CREATE(queueHandle, queueName, queueLength, queueItemSize, queueStorage, status) \
    OSAL_##queueHandle = OSA_MsgQ_CopyOfData_Create(queueLength, queueItemSize); \
    if (OSAL_##queueHandle == NULL) { \
        status = OSAL_FAILURE; \
    } else { \
        status = OSAL_SUCCESS; \
    }

/*! 
 * @brief OSAL queue put
 * @details This macro puts an item into a queue in the OSAL layer.
 *
 * @param[in] queueHandle Handle of the queue
 * @param[in] pItem Pointer to the item to be put into the queue
 * @param[in] timeout Timeout for the queue operation (typically waits forever.
 *            Isn't used in this implementation, but can be useful for future extensions)
 * @param[out] status Status of the queue operation
 */
#define OSAL_QUEUE_PUT(queueHandle, pItem, timeout, status) \
    if (queueHandle != NULL) { \
        status = OSA_MsgQPut(queueHandle, pItem); \
    } else { \
        status = OSAL_FAILURE; \
    }

/*!
 * @brief OSAL queue get
 * @details This macro gets an item from a queue in the OSAL layer.
 *
 * @param[in] queueHandle Handle of the queue
 * @param[out] pItem Pointer to the item to be retrieved from the queue
 * @param[in] timeout Timeout for the queue operation (typically waits forever)
 * @param[out] status Status of the queue operation
 */
#define OSAL_QUEUE_GET(queueHandle, pItem, timeout, status) \
    if (queueHandle != NULL) { \
        status = OSA_MsgQGet(queueHandle, pItem, timeout); \
    } else { \
        status = OSAL_FAILURE; \
    }

// =======================
// Timers functionality
// =======================
/*!
 * @brief Timer type enumeration
 * @details This enumeration defines the types of timers available in the OSAL layer.
 *          It includes one-shot and periodic timers. 
 */
typedef enum {
    OSAL_TIMER_TYPE_ONE_SHOT,
    OSAL_TIMER_TYPE_PERIODIC
} OSAL_TimerType_t;

/*!
 * @brief OSAL timer argument type
 * @details This type is used to pass arguments to the timer callback function.
 *          It is defined as a pointer to `TimerHandle_t`.
 */
typedef  TimerHandle_t OSAL_TimerArg_t;

/*!
 * @brief OSAL timer callback function type
 * @details This type defines the signature of the timer callback function.
 *          It takes a pointer to an argument of type `OSAL_TimerArg_t`.
 *          The callback function is called when the timer expires.
 */
typedef void (*OSAL_TimerCallback_t)(OSAL_TimerArg_t arg);

/*!
 * @brief OSAL timer define
 * @details This macro defines a timer handle in the OSAL layer.
 *          It allocates memory for a static timer handle and a timer structure.
 * @note The timer handle is a static variable of type `TimerHandle_t` 
 *       and the timer structure is of type `TimerHandle_t`.
 *
 * @param[in] timerHandle Name of the timer handle to be defined.
 */
#define OSAL_TIMER_DEFINE(timerHandle)  \
    static TimerHandle_t OSAL_##timerHandle = NULL

/*! 
 * @brief OSAL timer get handle
 * @details This macro retrieves the handle of a timer defined in the OSAL layer.
 * @note The handle is a pointer to the static variable defined by `OSAL_TIMER_DEFINE`.
 * @param[in] timerHandle Name of the timer handle to retrieve.
 * @return Pointer to the timer handle.
 */
#define OSAL_TIMER_GET_HANDLE(timerHandle) \
    OSAL_##timerHandle

/*!
 * @brief OSAL timer create
 * @details This macro creates a timer in the OSAL layer.
 *
 * @param[in,out] timerHandle Handle for the created timer
 * @param[in] timerCallback Callback function to be called when the timer expires
 * @param[in] timerType Type of the timer (one-shot or periodic)
 * @param[in] arg Argument to be passed to the timer callback function
 * @param[out] status Status of the timer creation
 */
#define OSAL_TIMER_CREATE(timerHandle, timerType, timerCallback, arg, status, ...) \
    TickType_t ticks = pdMS_TO_TICKS(__VA_ARGS__); /* Additional parameter for timer period */ \
    timerHandle = xTimerCreate( \
        #timerHandle, \
        ticks, \
        (timerType == OSAL_TIMER_TYPE_PERIODIC) ? pdTRUE : pdFALSE, \
        (void *)arg, \
        timerCallback   \
    ); \
    if (timerHandle == NULL) { \
        status = OSAL_FAILURE; \
    } else { \
        status = OSAL_SUCCESS; \
    }

/*!
 * @brief OSAL timer start
 * @details This macro starts a timer in the OSAL layer.
 *
 * @param[in] timerHandle Handle of the timer to be started
 * @param[in] timerInterval Interval for the timer in milliseconds.
 *            The parameter isn't used in this implementation, but can be useful for future extensions.
 * @note The timer will start counting down from the specified interval.
 *       If the timer is a periodic timer, it will restart automatically after each expiration.
 *       If the timer is a one-shot timer, it will stop after the first expiration.
 * @param[out] status Status of the timer start operation
 */
#define OSAL_TIMER_START(timerHandle, timerInterval, status) \
    if (timerHandle != NULL) { \
        if (xTimerStart(timerHandle, 0U) == pdPASS) { \
            status = OSAL_SUCCESS; \
        } else { \
            status = OSAL_FAILURE; \
        } \
    } else { \
        status = OSAL_FAILURE; \
    }

#endif // OSAL_H

/* [] END OF FILE */
