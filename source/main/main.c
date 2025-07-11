/**
 * @file  main.c
 *
 * @brief Implementation main() function entry and main Task's business logic.
 *
 * @version 0.4.0
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// HAL
#include "hal.h"
#include "hal_led.h"
#include "hal_gpio.h"
#include "hal_time.h"

// BMS Application includes
#include "BSP.h"
#include "qspyHelper.h"

#include "ADC.h"
#include "BLE.h"
#include "MAIN.h"
#include "LP.h"

// Q_UTEST / Unit tests includes
#if defined(Q_UTEST)
#include <stdio.h>
#include <gcov.h>
#endif // Q_UTEST

// RTOS includes
#include "OSAL.h"

// ========================
// Functions prototype
// ========================
static void mainTask_(OSAL_arg_t arg);
static void parseQueueItem_(Main_queue_data_t* queueItem);
static void handleAdcEvt_(Evt_adc_data_t* evt);
static void handleSystemEvt_(Evt_sys_data_t* evt);

static void MAIN_SM_handleSysEvt(Evt_sys_data_t* evt);
static void MAIN_SM_charge_setBal(Evt_sys_data_t* evt);
static void MAIN_SM_print_onStateChange(void);
static void MAIN_SM_handleAdcEvt(Evt_adc_data_t* evt);

static void MAIN_initDischargeSw(void);
static void MAIN_initChargeSw(void);
static void MAIN_initBalancerSw(void);
static void MAIN_setDischargeSw(HAL_dischargeSw_state_t state);
static void MAIN_setChargeSw(HAL_chargeSw_state_t state);
static void MAIN_enableBalancerSw(uint8_t balBanksEnMask);
static void MAIN_disableBalancerSw(uint8_t balBanksDisMask);

static void ble_update_vbat_(Evt_adc_data_t* adcEvt);

static void led_blink_alive_(void);
static void blinkTimerCallback_(OSAL_TimerArg_t arg);

// Q_UTEST / Unit tests specific functions
#if defined(Q_UTEST)
/** internal gcov library function to write data */
void __gcov_dump(void);
#endif //Q_UTEST

// FreeRTOS hooks
void vApplicationIdleHook(void);

// ================
// Definitions
// ================
#define MAIN_TASK_STACK_SIZE 1024U   /**< size in bytes, aligned to 8 bytes */
#define MAIN_QUEUE_SIZE 5U

// ================
// Private data
// ================
OSAL_TASK_DEFINE(mainTask);

/**
 * @brief Main task stack
 *
 * @note Defined in stack words because stack pointer should be aligned to
 *       8 bytes boundary per the RTOS requirements.
 * This is required by the ARM Procedure Call Standard (AAPCS) and the Cortex-M4 hardware. 
 * The ARM Cortex-M4 expects the stack pointer to be 8-byte aligned at all times, 
 * especially on exception entry/exit and for correct operation of floating-point instructions (if enabled).
 * FreeRTOS also assumes this alignment when creating tasks and managing context switches. 
 * Misalignment can lead to hard faults or unpredictable behavior.
 *
 * So, always ensure your stack memory and stack pointer are 8-byte aligned for Cortex-M4 with FreeRTOS.
 */
static uint64_t mainTaskStack_[MAIN_TASK_STACK_SIZE/8U];

/**
 * @brief Queue handle for the main task
 */
OSAL_QUEUE_DEFINE(mainTaskQueueHandle);
static Main_queue_data_t mainQueueSto[MAIN_QUEUE_SIZE];

/**
 * @brief State of the BMS and switches
 */
static BMS_state_t bmsState_ = BMS_STATE_IDLE;
static uint8_t swState_;

/**
 * @brief Timer for LED blinking
 */
OSAL_TIMER_DEFINE(blinkTimer);
static volatile uint8_t ledBlinkCntr_;

// =======================
// Code
// =======================
/*! Q_UTEST / Unit tests specific functions */
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
    /** Initialize the device and board peripherals */
    HAL_status_t initStatus = HAL_init_hardware();
    if (initStatus != HAL_STATUS_OK) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    /** Enable global interrupts */
    __enable_irq();

    /** Init semihosting for Q_UTEST */
#if defined(Q_UTEST)
    initSemihosting();
    printf("Semihosting started\n");
#endif //Q_UTEST

    /** Init LEDs */
    HAL_LED_init_green();
    HAL_LED_init_red();

    /** Init QSPY */
    QS_onStartup(NULL);

    /** Init QSPY dictionary & filters */
    QS_addUsrRecToDic(MAIN);
    QS_addUsrRecToDic(ADC_RCD);
    QS_addUsrRecToDic(BLE_TRACE);
    QS_addUsrRecToDic(BLE_BAS);
    QS_addUsrRecToDic(BLE_AIOS);
    QS_addUsrRecToDic(HAL);
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

    /** Set Low Power mode at start up  */
    LP_setMode(LP_DISABLED_MODE);

    /** Create main task */
    OSAL_Status_t status = OSAL_SUCCESS;
    OSAL_TASK_CREATE(
        OSAL_TASK_GET_HANDLE(mainTask),
        mainTask_,
        "mainTask",
        mainTaskStack_,         // should be aligned to 8 bytes
        MAIN_TASK_STACK_SIZE,   // in bytes
        OSAL_MAIN_TASK_PRIORITY,  // prio
        NULL,                    // no args
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    /** Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /** Never reach the point unless error conditions */
    HAL_ASSERT(0, __FILE__, __LINE__);
    while(true) {}
}

/**
 * @brief Main task's handler
 * 
 * @param[in] arg the argument passed from the thread create call to the entry function
 * 
 * @retval None
 */
static void mainTask_(OSAL_arg_t arg)
{
    (void) arg;
    OSAL_Status_t status = OSAL_SUCCESS;
    Main_queue_data_t queueItem;

    /** Start hardware timer for ticks count */
    hal_time_init();

    /** Init ADC peripheral & create ADC task */
    ADC_status_t adcStatus = ADC_init();
    if (adcStatus != ADC_STATUS_OK) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    /** Init BLE peripheral & create BLE tasks */
    BLE_status_t bleStatus = BLE_init();
    if (bleStatus != BLE_STATUS_OK) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    /** Init Low Power modes */
    LP_status_t lpStatus = LP_init();
    if (lpStatus != LP_INIT_STATUS_OK) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    /** Set default state (OFF) for discharge control switch */
    MAIN_initDischargeSw();
    MAIN_setDischargeSw(HAL_BMS_DISCHARGE_OFF);

    /** Set default state (OFF) for charge control switch */
    MAIN_initChargeSw();
    MAIN_setChargeSw(HAL_BMS_CHARGE_OFF);

    /** Set default state (OFF) for balancing switches */
    MAIN_initBalancerSw();
    MAIN_disableBalancerSw(HAL_BMS_ALL_BANKS);

    /** Init LED blink timer */
    OSAL_TIMER_CREATE(
        OSAL_TIMER_GET_HANDLE(blinkTimer),
        OSAL_TIMER_TYPE_ONE_SHOT, // One-shot timer for LED blinking
        blinkTimerCallback_,
        0U, // arg
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    /** Init timeout timers 
     * @todo timeout timers definition
     */

    /** Create an event Queue */
    OSAL_QUEUE_CREATE(
        mainTaskQueueHandle,    // handle name
        "mainTaskQueue",
        MAIN_QUEUE_SIZE,
        sizeof(Main_queue_data_t),
        mainQueueSto,
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__);
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
        OSAL_QUEUE_GET(
            OSAL_QUEUE_GET_HANDLE(mainTaskQueueHandle),
            &queueItem,
            OSAL_QUEUE_TIMEOUT_NEVER, // wait forever
            status
        );
        if (status != OSAL_SUCCESS) {
            HAL_ASSERT(0, __FILE__, __LINE__);
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

        /** Blink LED for alive indication */
        led_blink_alive_();

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
            handleAdcEvt_(&queueItem->evtData.adcData);
            break;

        case EVT_SYSTEM:
            handleSystemEvt_(&queueItem->evtData.sysEvtData);
            break;
        
        default:
            HAL_ASSERT(0, __FILE__, __LINE__);
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
    /**
     * Cells balancing & charge/discharge switches control
     */
    MAIN_SM_handleAdcEvt(evt);

    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("Banks volt: ");
        QS_I16(0, evt->bank1_mv);
        QS_I16(0, evt->bank2_mv);
        QS_I16(0, evt->bank3_mv);
        QS_I16(0, evt->bank4_mv);
        QS_I16(0, evt->full_mv);
    QS_END()
}

/**
 * @brief Handles an System event
 * 
 * @param[in] evt Pointer to incoming event
 * 
 * @retval None
 */
static void handleSystemEvt_(Evt_sys_data_t* evt)
{
    /** Process an event in Main task state machine */
    MAIN_SM_handleSysEvt(evt);

    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("Sys evt, set switches state: ");
        QS_U8(0, swState_);
    QS_END()

    /** Notify BLE task about switches' state */
    Ble_evt_t btEvt;
    btEvt.sysData.swStates = swState_;
    BLE_post_evt(&btEvt, EVT_SYSTEM);
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
     /** Handle QSPY communication */
     QS_onIdle();
}

// ========================
// Queue functions/APIs
// ========================
/** 
 * @brief Posts an event into Main task event queue
 * 
 * @param[in] evt Event to post
 * 
 * @param[in] eventType Event type to post
 * 
 * @retval None
 * 
 */
void MAIN_post_evt(Main_evt_t* evt, Evt_types_t eventType)
{
    HAL_ASSERT((evt != NULL) && (eventType < EVT_TYPE_MAX), __FILE__, __LINE__);

    OSAL_Status_t status = OSAL_SUCCESS;
    Main_queue_data_t queueItem;

    queueItem.evtType = eventType;
    memcpy((uint8_t*) &queueItem.evtData, (uint8_t*) evt, sizeof(Main_evt_t));

    OSAL_QUEUE_PUT(
        OSAL_QUEUE_GET_HANDLE(mainTaskQueueHandle),
        &queueItem,
        OSAL_QUEUE_TIMEOUT_NEVER,
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }
}

// ===============================
// Switches control functions
// ===============================
/** 
 * @brief Init MCU pin for discharge switch
 * 
 * @param None
 * 
 * @retval None
 * 
 */
static void MAIN_initDischargeSw(void)
{
    HAL_GPIO_init_pin(
        HAL_DISCHARGE_PORT,
        HAL_DISCHARGE_PIN,
        HAL_GPIO_DIGITAL_OUTPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_HIGH,
        HAL_DISCHARGE_OFF
    );
}

/** 
 * @brief Init MCU pin for charge switch
 * 
 * @param None
 * 
 * @retval None
 * 
 */
static void MAIN_initChargeSw(void)
{
    HAL_GPIO_init_pin(
        HAL_CHARGE_PORT,
        HAL_CHARGE_PIN,
        HAL_GPIO_DIGITAL_OUTPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_HIGH,
        HAL_CHARGE_OFF
    );
}

/** 
 * @brief Sets discharge switch state
 * 
 * @param[in] state : ON / OFF
 * 
 * @retval None
 * 
 */
static void MAIN_setDischargeSw(HAL_dischargeSw_state_t state)
{
    Switch_state_t* sw_state = (Switch_state_t *) &swState_;

    switch(state) {
        case HAL_BMS_DISCHARGE_OFF:
        {
            HAL_GPIO_set_pin(HAL_DISCHARGE_PORT, HAL_DISCHARGE_PIN, HAL_DISCHARGE_OFF);
            sw_state->setDischState = 0;
            break;
        }
        case HAL_BMS_DISCHARGE_ON:
        {
            HAL_GPIO_set_pin(HAL_DISCHARGE_PORT, HAL_DISCHARGE_PIN, HAL_DISCHARGE_ON);
            sw_state->setDischState = 1;
            break;
        }
        default:
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
    }
}

/** 
 * @brief Sets charge switch state
 * 
 * @param[in] state : ON / OFF
 * 
 * @retval None
 * 
 */
static void MAIN_setChargeSw(HAL_chargeSw_state_t state)
{
    Switch_state_t* sw_state = (Switch_state_t *) &swState_;

    switch(state) {
        case HAL_BMS_CHARGE_OFF:
        {
            HAL_GPIO_set_pin(HAL_CHARGE_PORT, HAL_CHARGE_PIN, HAL_CHARGE_OFF);
            sw_state->setChargeState = 0;
            break;
        }
        case HAL_BMS_CHARGE_ON:
        {
            HAL_GPIO_set_pin(HAL_CHARGE_PORT, HAL_CHARGE_PIN, HAL_CHARGE_ON);
            sw_state->setChargeState = 1;
            break;
        }
        default:
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
    }
}

// ==================================
// Banks balancing functions
// ==================================
/** 
 * @brief Init MCU pins for balancer circuits
 * 
 * @param None
 * 
 * @note 4 banks is used in the BMS
 * 
 * @retval None
 * 
 */
static void MAIN_initBalancerSw(void)
{
    /** Bank1 pin init */
    HAL_GPIO_init_pin(
        HAL_BAL_BANK1_PORT,
        HAL_BAL_BANK1_PIN,
        HAL_GPIO_DIGITAL_OUTPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_HIGH,
        HAL_BAL_BANK1_OFF
    );

    /** Bank2 pin init */
    HAL_GPIO_init_pin(
        HAL_BAL_BANK2_PORT,
        HAL_BAL_BANK2_PIN,
        HAL_GPIO_DIGITAL_OUTPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_HIGH,
        HAL_BAL_BANK2_OFF
    );

    /** Bank3 pin init */
    HAL_GPIO_init_pin(
        HAL_BAL_BANK3_PORT,
        HAL_BAL_BANK3_PIN,
        HAL_GPIO_DIGITAL_OUTPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_HIGH,
        HAL_BAL_BANK3_OFF
    );

    /** Bank4 pin init */
    HAL_GPIO_init_pin(
        HAL_BAL_BANK4_PORT,
        HAL_BAL_BANK4_PIN,
        HAL_GPIO_DIGITAL_OUTPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_HIGH,
        HAL_BAL_BANK4_OFF
    );
}

/** 
 * @brief Enable balancer banks by mask
 * 
 * @param[in] balBanksEnMask  mask that defines Banks to enable
 * 
 * @retval None
 * 
 */
static void MAIN_enableBalancerSw(uint8_t balBanksEnMask)
{
    HAL_ASSERT((balBanksEnMask > 0) && (balBanksEnMask <= HAL_BMS_ALL_BANKS), __FILE__, __LINE__);

    Switch_state_t* sw_state = (Switch_state_t *) &swState_;

    if (balBanksEnMask & HAL_BMS_BANK1_MASK) {
        HAL_GPIO_set_pin(HAL_BAL_BANK1_PORT, HAL_BAL_BANK1_PIN, HAL_BAL_BANK1_ON);
        sw_state->setBank1Balancer = 1;
    }
    if (balBanksEnMask & HAL_BMS_BANK2_MASK) {
        HAL_GPIO_set_pin(HAL_BAL_BANK2_PORT, HAL_BAL_BANK2_PIN, HAL_BAL_BANK2_ON);
        sw_state->setBank2Balancer = 1;
    }
    if (balBanksEnMask & HAL_BMS_BANK3_MASK) {
        HAL_GPIO_set_pin(HAL_BAL_BANK3_PORT, HAL_BAL_BANK3_PIN, HAL_BAL_BANK3_ON);
        sw_state->setBank3Balancer = 1;
    }
    if (balBanksEnMask & HAL_BMS_BANK4_MASK) {
        HAL_GPIO_set_pin(HAL_BAL_BANK4_PORT, HAL_BAL_BANK4_PIN, HAL_BAL_BANK4_ON);
        sw_state->setBank4Balancer = 1;
    }
}

/** 
 * @brief Disnable balancer banks by mask
 * 
 * @param[in] balBanksDisMask  mask that defines Banks to disable
 * 
 * @retval None
 * 
 */
static void MAIN_disableBalancerSw(uint8_t balBanksDisMask)
{
    HAL_ASSERT((balBanksDisMask > 0) && (balBanksDisMask <= HAL_BMS_ALL_BANKS), __FILE__, __LINE__);

    Switch_state_t* sw_state = (Switch_state_t *) &swState_;

    if (balBanksDisMask & HAL_BMS_BANK1_MASK) {
        HAL_GPIO_set_pin(HAL_BAL_BANK1_PORT, HAL_BAL_BANK1_PIN, HAL_BAL_BANK1_OFF);
        sw_state->setBank1Balancer = 0;
    }
    if (balBanksDisMask & HAL_BMS_BANK2_MASK) {
        HAL_GPIO_set_pin(HAL_BAL_BANK2_PORT, HAL_BAL_BANK2_PIN, HAL_BAL_BANK2_OFF);
        sw_state->setBank2Balancer = 0;
    }
    if (balBanksDisMask & HAL_BMS_BANK3_MASK) {
        HAL_GPIO_set_pin(HAL_BAL_BANK3_PORT, HAL_BAL_BANK3_PIN, HAL_BAL_BANK3_OFF);
        sw_state->setBank3Balancer = 0;
    }
    if (balBanksDisMask & HAL_BMS_BANK4_MASK) {
        HAL_GPIO_set_pin(HAL_BAL_BANK4_PORT, HAL_BAL_BANK4_PIN, HAL_BAL_BANK4_OFF);
        sw_state->setBank4Balancer = 0;
    }
}

/**
 * @todo Implement banks balancing algorithm based on banks' voltage
 */

// ===========================
// State machine functions
// ===========================
/**
 * @brief Handles system events in the BMS state machine
 * 
 * @param[in] evt Pointer to incoming event
 * 
 * @retval None
 */
static void MAIN_SM_handleSysEvt(Evt_sys_data_t* evt)
{
    switch (bmsState_)
    {
        case BMS_STATE_IDLE:
        {
            if (evt->setDischState == 1U) {
                MAIN_setDischargeSw(HAL_BMS_DISCHARGE_ON);
                bmsState_ = BMS_STATE_DISCHARGE;
                MAIN_SM_print_onStateChange();
            } else if (evt->setChargeState == 1U) {
                MAIN_setChargeSw(HAL_BMS_CHARGE_ON);
                bmsState_ = BMS_STATE_CHARGE;
                MAIN_SM_print_onStateChange();
            }
            break;
        }

        case BMS_STATE_DISCHARGE:
        {
            if (evt->setDischState == 0U) {
                MAIN_setDischargeSw(HAL_BMS_DISCHARGE_OFF);
                bmsState_ = BMS_STATE_IDLE;
                MAIN_SM_print_onStateChange();
            }
            break;
        }

        case BMS_STATE_CHARGE:
        {
            if (evt->setChargeState == 0U) {
                MAIN_setChargeSw(HAL_BMS_CHARGE_OFF);
                bmsState_ = BMS_STATE_IDLE;
                MAIN_SM_print_onStateChange();
            } else {
                MAIN_SM_charge_setBal(evt);
            }
            break;
        }

        case BMS_STATE_ERROR:
        {
            /** @todo Implement */
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
        }

        case BMS_STATE_SHELF:
        {
             /** @todo Implement */
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
        }

        default:
        {
            HAL_ASSERT(0, __FILE__, __LINE__);
        }
    }
}

/**
 * @brief Sets balancing switches based on the event data
 * 
 * @param[in] evt Pointer to incoming event
 * 
 * @retval None
 */
static void MAIN_SM_charge_setBal(Evt_sys_data_t* evt)
{
    if (evt->setBank1Balancer == 1U) {
        MAIN_enableBalancerSw(HAL_BMS_BANK1_MASK);
    } else {
        MAIN_disableBalancerSw(HAL_BMS_BANK1_MASK);
    }
    if (evt->setBank2Balancer == 1U) {
        MAIN_enableBalancerSw(HAL_BMS_BANK2_MASK);
    } else {
        MAIN_disableBalancerSw(HAL_BMS_BANK2_MASK);
    }
    if (evt->setBank3Balancer == 1U) {
        MAIN_enableBalancerSw(HAL_BMS_BANK3_MASK);
    } else {
        MAIN_disableBalancerSw(HAL_BMS_BANK3_MASK);
    }
    if (evt->setBank4Balancer == 1U) {
        MAIN_enableBalancerSw(HAL_BMS_BANK4_MASK);
    } else {
        MAIN_disableBalancerSw(HAL_BMS_BANK4_MASK);
    }
}

/**
 * @brief Handles ADC events in the BMS state machine
 * 
 * @param[in] evt Pointer to incoming ADC event
 * 
 * @retval None
 */
static void MAIN_SM_handleAdcEvt(Evt_adc_data_t* evt)
{
    /** Handling specific to BMS state */
    switch (bmsState_)
    {
        case BMS_STATE_IDLE:
        {
            /** @todo Check full VBAT: 
             * 1. if VBAT < min: save log to flash, transit to error state
             * 2. if VBAT > max: save log to flash
             * 4. Send VBAT to BLE task
             */
            ble_update_vbat_(evt);
            LP_setMode(LP_DEEP_SLEEP_MODE);
            break;
        }

        case BMS_STATE_DISCHARGE:
        {
            if (evt->full_mv < ADC_BMS_FULL_VBAT_MIN) {
                Evt_sys_data_t evt = {0};
                evt.setDischState = 0;
                MAIN_post_evt((Main_evt_t*) &evt, EVT_SYSTEM);
            }
            ble_update_vbat_(evt);
            LP_setMode(LP_DEEP_SLEEP_MODE);
            break;
        }

        case BMS_STATE_CHARGE:
        {
            if (evt->full_mv > ADC_BMS_FULL_VBAT_MAX) {
                Evt_sys_data_t evt = {0};
                evt.setChargeState = 0;
                MAIN_post_evt((Main_evt_t*) &evt, EVT_SYSTEM);
            }
            ble_update_vbat_(evt);
            LP_setMode(LP_DEEP_SLEEP_MODE);
            break;
        }

        case BMS_STATE_ERROR:
        {
            /** @todo Implement */
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
        }

        case BMS_STATE_SHELF:
        {
             /** @todo Implement */
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
        }

        default:
        {
            HAL_ASSERT(0, __FILE__, __LINE__);
        }
    }
}

/**
 * @brief Prints BMS state on state change
 * 
 * @retval None
 */
static void MAIN_SM_print_onStateChange(void)
{
    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("BMS state: ");
        QS_U8(0, bmsState_);
    QS_END()
}

// ===========================
/// BLE commands
// ===========================
/**
 * @brief Updates battery voltage level via BLE
 * 
 * @param[in] adcEvt Pointer to ADC event data
 * 
 * @retval None
 */
static void ble_update_vbat_(Evt_adc_data_t* adcEvt)
{
    Ble_evt_t btEvt;

    btEvt.vbat.batLvlPercent = ADC_BMS_CALC_PERCENT(adcEvt->full_mv);
    btEvt.vbat.adcData.bank1_mv = adcEvt->bank1_mv;
    btEvt.vbat.adcData.bank2_mv = adcEvt->bank2_mv;
    btEvt.vbat.adcData.bank3_mv = adcEvt->bank3_mv;
    btEvt.vbat.adcData.bank4_mv = adcEvt->bank4_mv;
    btEvt.vbat.adcData.full_mv = adcEvt->full_mv;

    BLE_post_evt(&btEvt, EVT_BLE_VBAT);
}

// ==========================
/// Spare, debug functions
// ==========================
/**
 * @brief Blinks the alive LED
 * @details This function starts a timer that will blink the green LED
 * @param None
 * @retval None
 * 
 * @attention Should be non-blocking call
 *            to avoid starvation of other threads especiaaly BLE stack
 */
static void led_blink_alive_(void)
{
    ledBlinkCntr_ = 0;
    // start timer
    OSAL_Status_t status = OSAL_SUCCESS;
    OSAL_TIMER_START(
        OSAL_TIMER_GET_HANDLE(blinkTimer),
        100U, // 100 ms
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }
}

/**
 * @brief Callback for the LED blink timer
 * @details This function is called by the OSAL timer when it expires.
 *          It toggles the green LED state to create a blink effect.
 *
 * @param[in] arg Timer argument (unused)
 * @retval None
 * @note 3 "on-off" blinks
 */
static void blinkTimerCallback_(OSAL_TimerArg_t arg)
{
    (void) arg; // unused argument

    // Blink green LED
    // 3 blinks, 100 ms each
    // 100 ms on, 100 ms off
    if (ledBlinkCntr_%2 == 0) {
        HAL_LED_green_on();
    } else {
        HAL_LED_green_off();
    }

    ledBlinkCntr_++;
    if (ledBlinkCntr_ < 6U) {
        // re-start timer to continue blink
        OSAL_Status_t status = OSAL_SUCCESS;
        OSAL_TIMER_START(
            OSAL_TIMER_GET_HANDLE(blinkTimer),
            100U, // 100 ms
            status
        );
        if (status != OSAL_SUCCESS) {
            HAL_ASSERT(0, __FILE__, __LINE__);
        }
    }
}

/* [] END OF FILE */
