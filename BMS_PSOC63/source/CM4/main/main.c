/******************************************************************************
* @file  main.c
*
* @brief This is the source code for CM4 in the the Dual CPU Application.
*
* @version 0.1.0
*/

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "BSP.h"
#include "qspyHelper.h"
#if defined(Q_UTEST)
#include <stdio.h>
#include <gcov.h>
#endif // Q_UTEST
#include "ADC.h"
#include "BLE.h"

// RTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"

/*******************************/
/*** Functions prototypes */
/******************************/
static void mainTask_(cy_thread_arg_t arg);
#if defined(Q_UTEST)
void __gcov_dump(void); /* internal gcov function to write data */
#endif //Q_UTEST
void vApplicationIdleHook(void);

/*******************************/
/*** Definitions */
/******************************/
#define MAIN_TASK_STACK_SIZE 400U   // bytes, aligned to 8 bytes

/*******************************/
/*** Data */
/******************************/
static cy_thread_t mainTaskHandle_;
/** 
 *  In stack words because stack pointer should be aligned to 
 *  8 bytes per the RTOS requirements.
 */
static uint64_t mainTaskStack_[MAIN_TASK_STACK_SIZE/8U];

/*******************************/
/*** Code */
/******************************/
#if defined(Q_UTEST)
extern void initialise_monitor_handles(void);
static void initSemihosting(void)
{
    initialise_monitor_handles();
}
#endif //Q_UTEST

int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS) {
        CY_ASSERT(0);
    }

    /** Enable global interrupts */
    __enable_irq();

#if defined(Q_UTEST)
    initSemihosting();
    printf("Semihosting started\n");
#endif //Q_UTEST

    /** Init LEDs */
    BSP_init_led_green();
    BSP_init_led_red();

    /** Init QSPY */
    QS_onStartup(NULL);

    /** Init QSPY dictionary & filters */
    QS_addUsrRecToDic(MAIN);
    QS_addUsrRecToDic(ADC);
    QS_addUsrRecToDic(BLE_TRACE);
#if defined(Q_UTEST)
    QS_addUsrRecToDic(UTEST);
    QS_addUsrRecToDic(BSP);
#endif //Q_UTEST
    QS_initGlbFilters();

    /** dictionaries... */
    QS_FUN_DICTIONARY(&mainTask_);
#if defined(Q_UTEST)
    QS_FUN_DICTIONARY(__gcov_dump);
    BSP_initUTdic();
#endif //Q_UTEST

    /** Create main task */
    result = cy_rtos_thread_create(
        &mainTaskHandle_, 
        mainTask_,
        "mainTask", 
        mainTaskStack_,         // should be aligned to 8 bytes
        MAIN_TASK_STACK_SIZE,   // in bytes
        CY_RTOS_PRIORITY_HIGH,  // prio
        NULL                    // no args
    );
    if (result != CY_RSLT_SUCCESS) {
        CY_ASSERT(0);
    }

    /** Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /** Never reach the point unless error conditions */
    CY_ASSERT(0);
    while(1) {}
}


/**
 * @fn mainTask_
 * @brief Main task/dispatcher
 * 
 * @param[in] arg the argument passed from the thread create call 
 *            to the entry function
 * @retval None
 */
static void mainTask_(cy_thread_arg_t arg)
{
    (void) arg;
    /** Init ADC peripheral & create ADC task */
    ADC_status_t adcStatus = ADC_init();
    if (adcStatus != ADC_STATUS_OK) {
        CY_ASSERT(0);
    }

    /** Init BLE peripheral & create BLE tasks */
    BLE_status_t bleStatus = BLE_init();
    if (bleStatus != BLE_STATUS_OK) {
        CY_ASSERT(0);
    }

    while(1) {
        (void)cy_rtos_delay_milliseconds(1000U);    // the API always returns SUCCESS becaause of hardcode

        BSP_led_green_toggle();

#if !defined(Q_UTEST)
        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Running main task");
        QS_END()
#endif //Q_UTEST
    }
}

/**
 * @fn vApplicationIdleHook
 * @brief Callback for Idle task
 * 
 * @details In case of main build it performs actions on Idle condition.
 *          In case of test/Q_UTEST build it BEHAVES additionally as part 
 *          of Q_UTEST framework by parsing incoming messages on QSPY.
 * 
 * @param None
 * @retval None
 */
void vApplicationIdleHook(void)
{
     /** Do job on Idle */
     QS_onIdle();
}

/* [] END OF FILE */
