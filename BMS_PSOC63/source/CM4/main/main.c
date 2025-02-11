/**
 * @file  main.c
 *
 * @brief Implementation main() function entry and main Task's business logic.
 *
 * @version 0.4.0
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
#include "MAIN.h"
#include "LP.h"

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
static void handleSystemEvt_(Evt_sys_data_t* evt);

static void MAIN_SM_handleSysEvt(Evt_sys_data_t* evt);
static void MAIN_SM_charge_setBal(Evt_sys_data_t* evt);
static void MAIN_SM_print_onStateChange(void);
static void MAIN_SM_handleAdcEvt(Evt_adc_data_t* evt);

#if defined(Q_UTEST)
/** internal gcov library function to write data */
void __gcov_dump(void);
#endif //Q_UTEST
void vApplicationIdleHook(void);

static void MAIN_initDischargeSw(void);
static void MAIN_initChargeSw(void);
static void MAIN_initBalancerSw(void);
static void MAIN_setDischargeSw(MAIN_dischargeSw_state_t state);
static void MAIN_setChargeSw(MAIN_chargeSw_state_t state);
static void MAIN_enableBalancerSw(uint8_t balBanksEnMask);
static void MAIN_disableBalancerSw(uint8_t balBanksDisMask);

static void ble_update_vbat_(Evt_adc_data_t* adcEvt);

static void led_blink_alive_(void);
static void blinkTimerCallback_(cy_timer_callback_arg_t arg);

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

static BMS_state_t bmsState_ = BMS_STATE_IDLE;
static uint8_t swState_;

static cy_timer_t blinkTimer;
static volatile uint8_t ledBlinkCntr_;

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
    QS_addUsrRecToDic(BLE_BAS);
    QS_addUsrRecToDic(BLE_AIOS);
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

    /** Set default state (OFF) for discharge control switch */
    MAIN_initDischargeSw();
    MAIN_setDischargeSw(MAIN_BMS_DISCHARGE_OFF);

    /** Set default state (OFF) for charge control switch */
    MAIN_initChargeSw();
    MAIN_setChargeSw(MAIN_BMS_CHARGE_OFF);

    /** Set default state (OFF) for balancing switches */
    MAIN_initBalancerSw();
    MAIN_disableBalancerSw(MAIN_BMS_ALL_BANKS);

    /** Init LED blink timer */
    result = cy_rtos_timer_init(
        &blinkTimer,
        CY_TIMER_TYPE_ONCE,
        blinkTimerCallback_,
        0U // arg
    );

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
    /** temporary solution for LPM debug purposes, @todo: fix */
     QS_FLUSH();

    /** Enter Sleep mode */
    LP_periph_ready_t periphStatus = LP_getPeriphStatus();
    if (periphStatus == LP_PERIPH_READY) {
        LP_enterSleep();
    }
}

/////////////////////////
/// Queue functions/APIs
/////////////////////////
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
    CY_ASSERT((evt != NULL) && (eventType < EVT_TYPE_MAX));

    Main_queue_data_t queueItem;

    queueItem.evtType = eventType;
    memcpy((uint8_t*) &queueItem.evtData, (uint8_t*) evt, sizeof(Main_evt_t));

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
 * @brief Init MCU pin for discharge switch
 * 
 * @param None
 * 
 * @retval None
 * 
 */
static void MAIN_initDischargeSw(void)
{
    Cy_GPIO_Pin_FastInit(
        BMS_DISCHARGE_PORT,
        BMS_DISCHARGE_PIN,
        CY_GPIO_DM_STRONG_IN_OFF,
        BMS_DISCHARGE_OFF, 
        HSIOM_SEL_GPIO
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
    Cy_GPIO_Pin_FastInit(
        BMS_CHARGE_PORT,
        BMS_CHARGE_PIN,
        CY_GPIO_DM_STRONG_IN_OFF,
        BMS_CHARGE_OFF, 
        HSIOM_SEL_GPIO
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
static void MAIN_setDischargeSw(MAIN_dischargeSw_state_t state)
{
    Switch_state_t* sw_state = (Switch_state_t *) &swState_;

    switch(state) {
        case MAIN_BMS_DISCHARGE_OFF:
        {
            Cy_GPIO_Write(
                BMS_DISCHARGE_PORT,
                BMS_DISCHARGE_PIN,
                BMS_DISCHARGE_OFF
            );
            sw_state->setDischState = 0;
            break;
        }
        case MAIN_BMS_DISCHARGE_ON:
        {
            Cy_GPIO_Write(
                BMS_DISCHARGE_PORT,
                BMS_DISCHARGE_PIN,
                BMS_DISCHARGE_ON
            );
            sw_state->setDischState = 1;
            break;
        }
        default:
            CY_ASSERT(0);
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
static void MAIN_setChargeSw(MAIN_chargeSw_state_t state)
{
    Switch_state_t* sw_state = (Switch_state_t *) &swState_;

    switch(state) {
        case MAIN_BMS_CHARGE_OFF:
        {
            Cy_GPIO_Write(
                BMS_CHARGE_PORT,
                BMS_CHARGE_PIN,
                BMS_CHARGE_OFF
            );
            sw_state->setChargeState = 0;
            break;
        }
        case MAIN_BMS_CHARGE_ON:
        {
            Cy_GPIO_Write(
                BMS_CHARGE_PORT,
                BMS_CHARGE_PIN,
                BMS_CHARGE_ON
            );
            sw_state->setChargeState = 1;
            break;
        }
        default:
            CY_ASSERT(0);
            break;
    }
}

//////////////////////////////////
/// Banks balancing functions
//////////////////////////////////
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
    Cy_GPIO_Pin_FastInit(
        BMS_BAL_BANK1_PORT,
        BMS_BAL_BANK1_PIN,
        CY_GPIO_DM_STRONG_IN_OFF,
        BMS_BAL_BANK1_OFF, 
        HSIOM_SEL_GPIO
    );

    /** Bank2 pin init */
    Cy_GPIO_Pin_FastInit(
        BMS_BAL_BANK2_PORT,
        BMS_BAL_BANK2_PIN,
        CY_GPIO_DM_STRONG_IN_OFF,
        BMS_BAL_BANK2_OFF, 
        HSIOM_SEL_GPIO
    );

    /** Bank3 pin init */
    Cy_GPIO_Pin_FastInit(
        BMS_BAL_BANK3_PORT,
        BMS_BAL_BANK3_PIN,
        CY_GPIO_DM_STRONG_IN_OFF,
        BMS_BAL_BANK3_OFF, 
        HSIOM_SEL_GPIO
    );

    /** Bank4 pin init */
    Cy_GPIO_Pin_FastInit(
        BMS_BAL_BANK4_PORT,
        BMS_BAL_BANK4_PIN,
        CY_GPIO_DM_STRONG_IN_OFF,
        BMS_BAL_BANK4_OFF, 
        HSIOM_SEL_GPIO
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
    CY_ASSERT((balBanksEnMask > 0) && (balBanksEnMask <= MAIN_BMS_ALL_BANKS));

    Switch_state_t* sw_state = (Switch_state_t *) &swState_;

    if (balBanksEnMask & MAIN_BMS_BANK1_MASK) {
        Cy_GPIO_Write(
            BMS_BAL_BANK1_PORT,
            BMS_BAL_BANK1_PIN,
            BMS_BAL_BANK1_ON
        );
        sw_state->setBank1Balancer = 1;
    }
    if (balBanksEnMask & MAIN_BMS_BANK2_MASK) {
        Cy_GPIO_Write(
            BMS_BAL_BANK2_PORT,
            BMS_BAL_BANK2_PIN,
            BMS_BAL_BANK2_ON
        );
        sw_state->setBank2Balancer = 1;
    }
    if (balBanksEnMask & MAIN_BMS_BANK3_MASK) {
        Cy_GPIO_Write(
            BMS_BAL_BANK3_PORT,
            BMS_BAL_BANK3_PIN,
            BMS_BAL_BANK3_ON
        );
        sw_state->setBank3Balancer = 1;
    }
    if (balBanksEnMask & MAIN_BMS_BANK4_MASK) {
        Cy_GPIO_Write(
            BMS_BAL_BANK4_PORT,
            BMS_BAL_BANK4_PIN,
            BMS_BAL_BANK4_ON
        );
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
    CY_ASSERT((balBanksDisMask > 0) && (balBanksDisMask <= MAIN_BMS_ALL_BANKS));

    Switch_state_t* sw_state = (Switch_state_t *) &swState_;

    if (balBanksDisMask & MAIN_BMS_BANK1_MASK) {
        Cy_GPIO_Write(
            BMS_BAL_BANK1_PORT,
            BMS_BAL_BANK1_PIN,
            BMS_BAL_BANK1_OFF
        );
        sw_state->setBank1Balancer = 0;
    }
    if (balBanksDisMask & MAIN_BMS_BANK2_MASK) {
        Cy_GPIO_Write(
            BMS_BAL_BANK2_PORT,
            BMS_BAL_BANK2_PIN,
            BMS_BAL_BANK2_OFF
        );
        sw_state->setBank2Balancer = 0;
    }
    if (balBanksDisMask & MAIN_BMS_BANK3_MASK) {
        Cy_GPIO_Write(
            BMS_BAL_BANK3_PORT,
            BMS_BAL_BANK3_PIN,
            BMS_BAL_BANK3_OFF
        );
        sw_state->setBank3Balancer = 0;
    }
    if (balBanksDisMask & MAIN_BMS_BANK4_MASK) {
        Cy_GPIO_Write(
            BMS_BAL_BANK4_PORT,
            BMS_BAL_BANK4_PIN,
            BMS_BAL_BANK4_OFF
        );
        sw_state->setBank4Balancer = 0;
    }
}

/**
 * @todo Implement banks balancing algorithm based on banks' voltage
 */

//////////////////////////////////
/// State machine functions
//////////////////////////////////
static void MAIN_SM_handleSysEvt(Evt_sys_data_t* evt)
{
    switch (bmsState_)
    {
        case BMS_STATE_IDLE:
        {
            if (evt->setDischState == 1U) {
                MAIN_setDischargeSw(MAIN_BMS_DISCHARGE_ON);
                bmsState_ = BMS_STATE_DISCHARGE;
                MAIN_SM_print_onStateChange();
            } else if (evt->setChargeState == 1U) {
                MAIN_setChargeSw(MAIN_BMS_CHARGE_ON);
                bmsState_ = BMS_STATE_CHARGE;
                MAIN_SM_print_onStateChange();
            }
            break;
        }

        case BMS_STATE_DISCHARGE:
        {
            if (evt->setDischState == 0U) {
                MAIN_setDischargeSw(MAIN_BMS_DISCHARGE_OFF);
                bmsState_ = BMS_STATE_IDLE;
                MAIN_SM_print_onStateChange();
            }
            break;
        }

        case BMS_STATE_CHARGE:
        {
            if (evt->setChargeState == 0U) {
                MAIN_setChargeSw(MAIN_BMS_CHARGE_OFF);
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
            CY_ASSERT(0);
            break;
        }

        case BMS_STATE_SHELF:
        {
             /** @todo Implement */
             CY_ASSERT(0);
            break;
        }

        default:
            CY_ASSERT(0);
    }
}

static void MAIN_SM_charge_setBal(Evt_sys_data_t* evt)
{
    if (evt->setBank1Balancer == 1U) {
        MAIN_enableBalancerSw(MAIN_BMS_BANK1_MASK);
    } else {
        MAIN_disableBalancerSw(MAIN_BMS_BANK1_MASK);
    }
    if (evt->setBank2Balancer == 1U) {
        MAIN_enableBalancerSw(MAIN_BMS_BANK2_MASK);
    } else {
        MAIN_disableBalancerSw(MAIN_BMS_BANK2_MASK);
    }
    if (evt->setBank3Balancer == 1U) {
        MAIN_enableBalancerSw(MAIN_BMS_BANK3_MASK);
    } else {
        MAIN_disableBalancerSw(MAIN_BMS_BANK3_MASK);
    }
    if (evt->setBank4Balancer == 1U) {
        MAIN_enableBalancerSw(MAIN_BMS_BANK4_MASK);
    } else {
        MAIN_disableBalancerSw(MAIN_BMS_BANK4_MASK);
    }
}

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
            LP_setMode(LP_SLEEP_MODE);
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
            LP_setMode(LP_SLEEP_MODE);
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
            LP_setMode(LP_SLEEP_MODE);
            break;
        }

        case BMS_STATE_ERROR:
        {
            /** @todo Implement */
            CY_ASSERT(0);
            break;
        }

        case BMS_STATE_SHELF:
        {
             /** @todo Implement */
             CY_ASSERT(0);
            break;
        }

        default:
            CY_ASSERT(0);
    }
}

static void MAIN_SM_print_onStateChange(void)
{
    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("BMS state: ");
        QS_U8(0, bmsState_);
    QS_END()
}

//////////////////////////////////
/// BLE commands
//////////////////////////////////
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

//////////////////////////////////
/// Spare, debug functions
//////////////////////////////////
/**
 * @attention Should be non-blocking call
 *            to avoid starvation of other threads especiaaly BLE stack
 */
static void led_blink_alive_(void)
{
    ledBlinkCntr_ = 0;
    // start timer
    cy_rslt_t result = cy_rtos_timer_start(&blinkTimer, 100U); // 100 ms
    if (result != CY_RSLT_SUCCESS) {
        CY_ASSERT(0);
    }
}

/**
 * @note 3 "on-off" blinks
 */
static void blinkTimerCallback_(cy_timer_callback_arg_t arg)
{
    if (ledBlinkCntr_%2 == 0) {
        BSP_led_green_on();
    } else {
        BSP_led_green_off();
    }

    ledBlinkCntr_++;
    if (ledBlinkCntr_ < 6U) {
        // re-start timer to continue blink
        cy_rslt_t result = cy_rtos_timer_start(&blinkTimer, 100U); // 100 ms
        if (result != CY_RSLT_SUCCESS) {
            CY_ASSERT(0);
        }
    }
}

/* [] END OF FILE */
