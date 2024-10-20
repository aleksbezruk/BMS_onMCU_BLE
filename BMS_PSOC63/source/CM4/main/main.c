/**
 * @file  main.c
 *
 * @brief Implementa main() function entry and main Task's business logic.
 *
 * @version 0.1.0
 */

#include <string.h>

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
#include "bms_events.h"

// RTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"

///////////////////
// Functions prototypes
///////////////////
static void mainTask_(cy_thread_arg_t arg);
static void parseQueueItem_(Main_queue_data_t* queueItem);
static void handleAdcEvt_(Evt_adc_data_t* evt);
#if defined(Q_UTEST)
/** internal gcov library function to write data */
void __gcov_dump(void);
#endif //Q_UTEST
void vApplicationIdleHook(void);

///////////////////
// Definitions
///////////////////
#define MAIN_TASK_STACK_SIZE 560U   /**< size in bytes, aligned to 8 bytes */
#define MAIN_QUEUE_SIZE 2U

///////////////////
// Private data
///////////////////
static cy_thread_t mainTaskHandle_;
/** 
 *  In stack words because stack pointer should be aligned to 
 *  8 bytes boundary per the RTOS requirements.
 */
static uint64_t mainTaskStack_[MAIN_TASK_STACK_SIZE/8U];
static StaticQueue_t staticQueueHandle;
static cy_queue_t mainTaskQueueHandle;
static Main_queue_data_t mainQueueSto[MAIN_QUEUE_SIZE];

///////////////////
// Code
///////////////////
#if defined(Q_UTEST)
extern void initialise_monitor_handles(void);
static void initSemihosting(void)
{
    initialise_monitor_handles();
}
#endif //Q_UTEST

/**
 * @brief main() function
 * 
 * @param None
 * 
 * @retval Never returns
 *
 */
int main(void)
{
    cy_rslt_t result;

    /** Initialize the device and board peripherals */
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
 * @brief Main task's handler
 * 
 * @param[in] arg the argument passed from the thread create call to the entry function
 * 
 * @retval None
 */
static void mainTask_(cy_thread_arg_t arg)
{
    (void) arg;
    cy_rslt_t result;
    Main_queue_data_t queueItem;

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

    /** Init timeout timers 
     * @todo timeout timers definition
     */

    /** Create an event Queue */
    mainTaskQueueHandle = xQueueCreateStatic(MAIN_QUEUE_SIZE, 
                                     sizeof(Main_queue_data_t),
                                     (uint8_t *) mainQueueSto,
                                     &staticQueueHandle);
    if (mainTaskQueueHandle == NULL) {
        CY_ASSERT(0);
    }

    /** 
     * Main Task loop :
     *  1. Waits for event about measurements results & cmds requested via BLE
     *  2. Executes controling of switch based on meas results
     *  3. Controls operations timeout (ADC timeout, BLE timeout etc.)
     *  4. Handles errors
     */
    while(1) {
        /** Wait for event */
        result = cy_rtos_queue_get(&mainTaskQueueHandle, 
                                   &queueItem,
                                   CY_RTOS_NEVER_TIMEOUT);
        if (result != CY_RSLT_SUCCESS) {
            CY_ASSERT(0);
        }

        /** Errors handling 
         *
         * @todo Handling errors detected by another tasks (ADC, BLE etc.)
         */

        /** Restart timeout timers 
         * 
         * @todo Tineout timers handling
         */

        /** Process events: 
         *      1. Switches control 
         *      2. BLE cmds from Client/User
         */
        parseQueueItem_(&queueItem);

        /** Toggle LED for debug purposes */
        BSP_led_green_toggle();

        /** Print debug message that indicates Running Main Task */
#if !defined(Q_UTEST)
        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Running main task");
        QS_END()
#endif //Q_UTEST
    }
}

/**
 * @brief Parses a queue item.
 * 
 * @param[in] queueItem Queue item to parse
 * 
 * @retval None
 */
static void parseQueueItem_(Main_queue_data_t* queueItem)
{
    switch (queueItem->evtType)
    {
    case EVT_ADC:
        handleAdcEvt_(&queueItem->adcData);
        break;
    case EVT_BLE:
        /** @todo Implement handling BLE evts */
        break;
    
    default:
        CY_ASSERT(0);
        break;
    }
}

/**
 * @brief Handles an ADC event
 * 
 * @param[in] evt Pointer to incoming event
 * 
 * @retval None. 
 */
static void handleAdcEvt_(Evt_adc_data_t* evt)
{
    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("Banks volt: ");
        QS_I16(0, evt->bank1_mv);
        QS_I16(0, evt->bank2_mv);
        QS_I16(0, evt->bank3_mv);
        QS_I16(0, evt->bank4_mv);
    QS_END()
}

/**
 * @brief Callback for the Idle task
 * 
 * @details In case of main build it performs actions on Idle condition.
 *          In case of test/Q_UTEST build it BEHAVES additionally as part 
 *          of Q_UTEST framework by parsing incoming messages on QSPY.
 * 
 * @param None
 * 
 * @retval None
 */
void vApplicationIdleHook(void)
{
     /** Do job on Idle */
     QS_onIdle();
}

/////////////////////////
/// Queue functions/APIs
/////////////////////////
/** 
 * @brief Posts an event into Main tasks event queue
 * 
 * @param[in] evt Event to post
 * 
 * @retval None
 * 
 */
void MAIN_post_evt(Evt_adc_data_t* evt)
{
    Main_queue_data_t queueItem;

    queueItem.evtType = EVT_ADC;
    memcpy((uint8_t*) &queueItem.adcData, (uint8_t*) evt, sizeof(Evt_adc_data_t));

    cy_rslt_t result = cy_rtos_queue_put(&mainTaskQueueHandle,
                                         &queueItem,
                                         CY_RTOS_NEVER_TIMEOUT);
    if (result != CY_RSLT_SUCCESS) {
        CY_ASSERT(0);
    }
}

//////////////////////////////////
/// Switches control functions
//////////////////////////////////
/**
 * @todo Implement 
 */

//////////////////////////////////
/// Banks balancing functions
//////////////////////////////////
/**
 * @todo Implement 
 */

/* [] END OF FILE */
