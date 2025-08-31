/**
 * @file  LP.c
 *
 * @brief Implements Low Power modes and tickless mode of the BMS/MCU.
 *
 * @details   ## **Details**
 *            ### 1.FreeRTOS tickless functionality, General Info: <br>
 *                1.1 mtb_shared/freertos/release-v10.5.002/Source/portable/COMPONENT_CM4/TOOLCHAIN_GCC_ARM/port.c ; <br>
 *                1.2 #define configUSE_TICKLESS_IDLE ; <br>
 *                    a. configUSE_TICKLESS_IDLE=1 : The Built-in FreeRTOS Tickless Idle functionality is enabled: <br>
 *                      - vPortSuppressTicksAndSleep() ->  <br>
 *                          - mtb_shared/freertos/release-v10.5.002/Source/portable/COMPONENT_CM4/TOOLCHAIN_GCC_ARM/portmacro.h; <br>
 *                          - mtb_shared/freertos/release-v10.5.002/Source/portable/COMPONENT_CM4/TOOLCHAIN_GCC_ARM/port.c; <br>
 *                          - 'WFI' -> sleep <br>
 *                    b. configUSE_TICKLESS_IDLE=2 : User defined tickless idle functionality : <br>
 *                      - mtb_shared/abstraction-rtos/release-v1.8.2/source/COMPONENT_FREERTOS/cyabs_freertos_helpers.c -> <br> 
 *                          portSUPPRESS_TICKS_AND_SLEEP() macro -> vApplicationSleep() ; <br>
 *                      - #define DEEPSLEEP_ENABLE ; <br>
 *                1.3 mtb_shared/freertos/release-v10.5.002/Source/tasks.c ; <br>
 *                1.4 tasks.c -> prvGetExpectedIdleTime() ; <br>
 *                1.5 tasks.c -> vTaskStepTick() ; <br>
 *                1.6 tasks.c -> eTaskConfirmSleepModeStatus() ; <br>
 *                1.7 # configEXPECTED_IDLE_TIME_BEFORE_SLEEP ; <br>
 *                1.8 configPRE_SLEEP_PROCESSING macros ; <br>
 *                1.9 configPOST_SLEEP_PROCESSING nacros ; <br>
 * 
 *              ### 2.PSOC63 & Cypress SDK specific requirements/dependencies for LP functionality: <br>
 *                      2.1 LP for CMO+ <br>
 *                          - Makefile: DISABLE_COMPONENTS=CM0P_SLEEP ; <br>
 *                          - Makefile: COMPONENT_CM0P_BLESS - This image starts BLE controller on CM0+ core, <br>
 *                                       starts CM4 core at CY_CORTEX_M4_APPL_ADDR and puts CM0+ core into a deep sleep mode. <br>
 *                                       It's alredy implemented, CMO+ core additional setting not needed. <br>
 *                      2.2 LP for SDK & BLE stack  <br>
 *                          - #define CY_CFG_PWR_SYS_IDLE_MODE (set in MTB Generated sources): <br>
 *                              #define CY_CFG_PWR_MODE_LP 0x01UL               <br>
 *                                  #define CY_CFG_PWR_MODE_ULP 0x02UL          <br>
 *                                  #define CY_CFG_PWR_MODE_ACTIVE 0x04UL       <br>
 *                                  #define CY_CFG_PWR_MODE_SLEEP 0x08UL        <br>
 *                                  #define CY_CFG_PWR_MODE_DEEPSLEEP 0x10UL    <br>
 *                          - #define CY_CFG_PWR_MODE_SLEEP - looks like setup CPU core Sleep mode ( see cyhal_syspm_sleep() ); <br>
 *                          - #define CY_CFG_PWR_MODE_DEEPSLEEP: <br>
 *                              - Available Core regulator for PSOC63: <br>
 *                                  - Normal Current LDO ; <br>
 *                                  - Minimum Current LDO ; <br>
 *                                  - Normal Current Buck ; <br>
 *                                  - Minimum Current Buck ; <br>
 *                              - System Active power mode : <br>
 *                                  - LP - is the default operating mode of the device after reset and provides maximum system performance. ; <br>
 *                                  - ULP Ultra Low Power (ULP) mode is identical to LP mode with a performance tradeoff made to achieve lower <br>
 *                                    system current. This tradeoff lowers the core operating voltage, which then requires reduced operating clock
 *                                    frequency and limited high-frequency clock sources. ; <br>
 *                              - CY_CFG_PWR_DEEPSLEEP_LATENCY / CY_CFG_PWR_DEEPSLEEP_LATENCY-> set DPSLP latency ; <br>
 *                              - CYHAL_SYSPM_RSLT_DEEPSLEEP_LOCKED -> option to lock/unlock DPSLP at runtime ; <br>
 *                              - cyhal_syspm_tickless_deepsleep() -> _cyhal_syspm_deepsleep_internal() -> Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT) ; <br>
 *                              - wake up sources: BLE, LP timers, GPIO ; <br>
 *                              - UART operation in DPSLP: <br>
 *                                  - retention Enabled; <br>
 *                                  - wake-up isn't available, so RX line should be configred for Input with IRQ on falling edge (START bit) ; <br>
 *                              - SAR ADC is Off in DPSLP, so needed logic to de-init ADC before DPSLP & re-init after wake-up/before measurements <br>
 *                              - register callbacks before / after transition to DPDSLP: <br> 
 *                                  - see \ref Cy_SysPm_CpuEnterDeepSleep() ; <br> 
 *                                  - see \ref CY_SYSPM_BEFORE_TRANSITION, CY_SYSPM_AFTER_TRANSITION, cy_en_syspm_callback_mode_t ; <br>
 *                                  - see \ref Cy_SysPm_RegisterCallback() ; <br>
 *                          - #define DEEPSLEEP_ENABLE -> Local 'CY_driver' definition for the vApplicationSleep() callback ; <br>
 * 
 *                ### 3. Define BMS system parameters that affects power consumption
 *                    3.1 Advertising interval - 3 seconds
 *                    3.2 ADC measurements duration - 5 measurements to avreage result
 * 
 * @version 0.5.0
 */

#include <stdbool.h>

// Cypress includes
#include "cy_pdl.h"
#include "cyhal.h"
#include "cyhal_syspm.h"
#include "cycfg.h"

// BMS includes
#include "LP.h"
#include "qspyHelper.h"
#include "ADC.h"

// RTOS includes
#include "OSAL.h"

// HAL
#include "hal.h"

// =========================
// Functions prototype
// =========================
static cy_en_syspm_status_t _checkReadyDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode);
static cy_en_syspm_status_t _beforeDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode);
static cy_en_syspm_status_t _afterDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode);
static void blockSleepTimerCallback_(OSAL_TimerArg_t arg);
void LP_enterSleep(TickType_t xExpectedIdleTime);

uint32_t cyabs_rtos_get_deepsleep_latency(void);

// =========================
// Defines
// =========================
#define pdTICKS_TO_MS(xTicks)    ( ( ( TickType_t ) ( xTicks ) * 1000u ) / configTICK_RATE_HZ )

// #define ENABLE_DEBUG_PRINTS  // uncomment to enable debug prints

// =========================
// Private data
// =========================
static volatile LP_modes_t _mode;

static cy_stc_syspm_callback_params_t _beforeDeepSleepCallbackParams;
static cy_stc_syspm_callback_t _pmBeforeDeepSlpCallback = {
    .callback = _beforeDeepSleepCallback,
    .type = CY_SYSPM_DEEPSLEEP,
    .skipMode = CY_SYSPM_SKIP_CHECK_READY | CY_SYSPM_SKIP_CHECK_FAIL | CY_SYSPM_SKIP_AFTER_TRANSITION, // only before transition
    .callbackParams = &_beforeDeepSleepCallbackParams,
    .prevItm = NULL,    // for CY driver internal usage
    .nextItm = NULL,    // for CY driver intranal usage
    .order = 0U
};

static cy_stc_syspm_callback_params_t _afterDeepSleepCallbackParams;
static cy_stc_syspm_callback_t _pmAfterDeepSlpCallback = {
    .callback = _afterDeepSleepCallback,
    .type = CY_SYSPM_DEEPSLEEP,
    .skipMode = CY_SYSPM_SKIP_CHECK_READY | CY_SYSPM_SKIP_CHECK_FAIL | CY_SYSPM_SKIP_BEFORE_TRANSITION, // only after transition
    .callbackParams = &_afterDeepSleepCallbackParams,
    .prevItm = NULL,    // for CY driver intranal usage
    .nextItm = NULL,    // for CY driver intranal usage
    .order = 0U
};

static cy_stc_syspm_callback_params_t _checkReadyDeepSleepCallbackParams;
static cy_stc_syspm_callback_t _pmCheckReadyDeepSlpCallback = {
    .callback = _checkReadyDeepSleepCallback,
    .type = CY_SYSPM_DEEPSLEEP,
    .skipMode = CY_SYSPM_SKIP_AFTER_TRANSITION | CY_SYSPM_SKIP_CHECK_FAIL | CY_SYSPM_SKIP_BEFORE_TRANSITION, // only CY_SYSPM_SKIP_CHECK_READY
    .callbackParams = &_checkReadyDeepSleepCallbackParams,
    .prevItm = NULL,    // for CY driver intranal usage
    .nextItm = NULL,    // for CY driver intranal usage
    .order = 0U
};

// QSPY RX line config for wakeup
static const cy_stc_gpio_pin_config_t QSPY_wakeup_config =
{
    .outVal = 1,
    .driveMode = CY_GPIO_DM_PULLUP,     /**< Resistive Pull-Up. Input buffer on */
    .hsiom = HSIOM_SEL_GPIO,
    .intEdge = CY_GPIO_INTR_FALLING,    /**< Catch START bit */
    .intMask = 1UL,                     /**< Enable Wakeup interrupt */
    .vtrip = CY_GPIO_VTRIP_CMOS,
    .slewRate = CY_GPIO_SLEW_FAST,
    .driveSel = CY_GPIO_DRIVE_1_2,
    .vregEn = 0UL,
    .ibufMode = 0UL,
    .vtripSel = 0UL,
    .vrefSel = 0UL,
    .vohSel = 0UL,
};

/** Block Sleep flag */
static volatile bool _blockSleep;

/** QSPY Wakeup flag */
static volatile bool _qspyWakeup;

/** Block Sleep Timer */
OSAL_TIMER_DEFINE(blockSleepTimer);

/** Low Power Timer */
static cyhal_lptimer_t* _lptimer = NULL;

// =========================
// Code
// =========================
/**
 * @brief Initialize Low Power modes and tickless mode
 * @details This function initializes the Low Power modes and tickless mode functionality.
 *          It registers the necessary callbacks for deep sleep transitions and initializes the block sleep timer.
 * @param None
 * @retval \ref LP_status_t. Status of the initialization: LP_INIT_STATUS_OK if successful, LP_INIT_STATUS_FAIL otherwise.
 */
LP_status_t LP_init(void)
{
    LP_status_t status = LP_INIT_STATUS_OK;
    OSAL_Status_t osal_status = OSAL_SUCCESS;

    if (!Cy_SysPm_RegisterCallback(&_pmBeforeDeepSlpCallback)) {
        status = LP_INIT_STATUS_FAIL;
    }

    if (status == LP_INIT_STATUS_OK) {
        if (!Cy_SysPm_RegisterCallback(&_pmAfterDeepSlpCallback)) {
            status = LP_INIT_STATUS_FAIL;
        }
    }

    if (status == LP_INIT_STATUS_OK) {
        if (!Cy_SysPm_RegisterCallback(&_pmCheckReadyDeepSlpCallback)) {
            status = LP_INIT_STATUS_FAIL;
        }
    }

    OSAL_TIMER_CREATE(
        OSAL_TIMER_GET_HANDLE(blockSleepTimer),
        OSAL_TIMER_TYPE_ONE_SHOT, // One-shot timer for block sleep
        blockSleepTimerCallback_,
        0U, // arg
        osal_status
    );
    if (osal_status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    return status;
}

/**
 * @brief Check if the system is ready for deep sleep
 * @details This function checks if the system is ready for deep sleep by verifying the status of the peripherals.
 *          It returns CY_SYSPM_SUCCESS if the system is ready, otherwise it returns CY_SYSPM_CANCELED.
 * @param callbackParams Pointer to the callback parameters (unused in this implementation)
 * @param mode The mode of the callback (unused in this implementation)
 */
static cy_en_syspm_status_t _checkReadyDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode)
{
    (void) callbackParams;

    LP_periph_ready_t periphStatus = LP_getPeriphStatus();
    if ((_blockSleep == true) || (periphStatus != LP_PERIPH_READY)) {
        return CY_SYSPM_CANCELED;
    } else {
        return CY_SYSPM_SUCCESS;
    }
}

/**
 * @brief Prepare the system for deep sleep
 * @details This function prepares the system for deep sleep by de-initializing the ADC,
 *          enabling the wake-up on QSPY command, and configuring the GPIO pin for QSPY RX line.
 *          It also enables the NVIC IRQ for the GPIO interrupt.
 * @param callbackParams Pointer to the callback parameters (unused in this implementation)
 * @param mode The mode of the callback (unused in this implementation)
 * @retval CY_SYSPM_SUCCESS if the preparation is successful, otherwise an error code.
 * @note This function is called before entering deep sleep mode.
 *       It de-initializes the ADC to save power and configures the GPIO pin for
 *       QSPY RX line to wake up the system on a command.
 *       It also enables the NVIC IRQ for the GPIO interrupt to handle wake-up events.
 *       The QSPY RX line is configured to use a pull-up resistor and to trigger
 *       an interrupt on a falling edge, which corresponds to the START bit of the QSPY command.
 *       The interrupt is masked to ensure that it does not interfere with the deep sleep
 *       operation, and the NVIC IRQ is enabled to allow the system to wake up on
 *       a QSPY command.
 *       After the system wakes up, the QSPY RX line is reconfigured to enable
 *       the UART receiver back, and the GPIO interrupt is disabled to prevent further
 *       interrupts until the next QSPY command.
 *       The function also starts a blocking timer if a QSPY command is received,
 *       which prevents the system from entering deep sleep until the timer expires.
 *       This is useful for ensuring that the system does not enter deep sleep mode
 *       while there are pending QSPY commands to be processed.
 */
static cy_en_syspm_status_t _beforeDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode)
{
    (void) callbackParams;

    /** De-init ADC */
    ADC_setState(false);
    ADC_deinit();

    /** Enable wake-up on QSPY command */
    _qspyWakeup = false;
    Cy_GPIO_Pin_Init(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN, &QSPY_wakeup_config);
    Cy_GPIO_ClearInterrupt(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN);
    /** Enable NVIC IRQ, \ref IRQn_Type in cy8c6347bzi_bld53.h */
    NVIC_SetPriority(ioss_interrupts_gpio_5_IRQn, 2U);
    NVIC_EnableIRQ(ioss_interrupts_gpio_5_IRQn);

    return CY_SYSPM_SUCCESS;
}

/**
 * @brief Handle the system after deep sleep
 * @details This function handles the system after deep sleep by reconfiguring the QSPY RX line,
 *          disabling the GPIO interrupt, and starting a blocking timer if a QSPY command was received.
 *          It also checks if the wake-up was due to a QSPY command and sets the appropriate flags.
 * @param callbackParams Pointer to the callback parameters (unused in this implementation)
 * @param mode The mode of the callback (unused in this implementation)
 * @retval CY_SYSPM_SUCCESS if the handling is successful, otherwise an error code.
 * @note This function is called after exiting deep sleep mode.
 *       It checks if the wake-up was due to a QSPY command and sets the `_qspyWakeup` flag accordingly.
 *       It then reconfigures the QSPY RX line to enable the UART receiver back and disables the GPIO interrupt
 *       to prevent further interrupts until the next QSPY command.
 *       If a QSPY command was received, it starts a blocking timer to prevent the system from entering deep sleep
 *       until the timer expires. This is useful for ensuring that the system does not enter deep sleep mode
 *       while there are pending QSPY commands to be processed.
 */
static cy_en_syspm_status_t _afterDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode)
{
    (void) callbackParams;

    /** Check if wakeup is due to QSPY */
    if (Cy_GPIO_GetInterruptStatusMasked(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN) == 1U) {
        _qspyWakeup = true;
#if defined(ENABLE_DEBUG_PRINTS)
        QS_BEGIN_ID(HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("Wakeup on QSPY");
        QS_END()
        QS_FLUSH();
#endif
    }

    /** Reconfigure QSPY RX line to enable UART receiver back, Disable GPIO IRQ
     * \ref CYBSP_UART_RX_PIN, CYBSP_UART_RX_config
     */
    Cy_GPIO_Pin_Init(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN, &CYBSP_UART_RX_config);
    Cy_GPIO_SetInterruptMask(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN, 0U);
    Cy_GPIO_ClearInterrupt(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN);
    NVIC_DisableIRQ(ioss_interrupts_gpio_5_IRQn);
    NVIC_ClearPendingIRQ(ioss_interrupts_gpio_5_IRQn);

    return CY_SYSPM_SUCCESS;
}

/**
 * @brief Interrupt handler for GPIO 5 (QSPY RX line)
 * @details This function handles the GPIO interrupt for the QSPY RX line.
 *          It checks if the interrupt was triggered by a QSPY command and sets the `_qspyWakeup` flag accordingly.
 *          It also clears the interrupt status for the QSPY RX line.
 * @note This function is called when a falling edge is detected on the QSPY RX line,
 *       which corresponds to the START bit of a QSPY command.
 */
void ioss_interrupts_gpio_5_IRQHandler(void)
{
    /** Check if wakeup is due to QSPY */
    if (Cy_GPIO_GetInterruptStatusMasked(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN) == 1U) {
        Cy_GPIO_ClearInterrupt(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN);
        _qspyWakeup = true;
    }
}

/**
 * @brief Callback for the block sleep timer
 * @details This function is called when the block sleep timer expires.
 *          It sets the `_blockSleep` flag to false, allowing the system to enter low power mode.
 * @param[in] arg Timer argument (unused)
 * @retval None
 * @note This function is used to unblock the system from entering low power mode after a certain period.
 */
static void blockSleepTimerCallback_(cy_timer_callback_arg_t arg)
{
    _blockSleep = false;

#if defined(ENABLE_DEBUG_PRINTS)
    QS_BEGIN_ID(HAL, 0 /*prio/ID for local Filters*/)
        QS_STR("SLP block end");
    QS_END()
    QS_FLUSH();
#endif
}

/**
 * @brief Get BMS peripherals readiness status for Low Power mode
 * 
 * @param None
 * 
 * @retval see @LP_periph_ready_t
 */
LP_periph_ready_t LP_getPeriphStatus(void)
{
    LP_periph_ready_t readiness = LP_PERIPH_READY;

    /** Is QSPY RX buffer empty ? */
    QSPY_rx_status_t qspy_rx_status = QS_get_rxStatus();
    if (qspy_rx_status == QSPY_RX_NOT_EMPTY) {
        readiness = LP_PERIPH_NOT_READY;
    }

    /** Is QSPY TX buffer empty ? */
    QSPY_tx_status_t qspy_tx_status = QS_get_txStatus();
    if (qspy_tx_status == QSPY_TX_NOT_EMPTY) {
        readiness = LP_PERIPH_NOT_READY;
    }

    return readiness;
}

/**
 * @brief Set the Low Power mode for the BMS system
 * 
 * @param[in] mode - Low Power mode to set, see @LP_modes_t
 * 
 * @retval None
 */
void LP_setMode(LP_modes_t mode)
{
    HAL_ASSERT((mode >= LP_DISABLED_MODE) && (mode <= LP_SHELF_MODE), __FILE__, __LINE__);
    _mode = mode;
}

/**
 * @brief Enter low power sleep or deep sleep mode for a specified duration
 * 
 * @param[in] xExpectedIdleTime - expected time to sleep [RTOS ticks]
 * 
 * @retval None
 * @details This function puts the system into sleep or deep sleep mode based on the current low power mode setting,
 *          for the duration specified by xExpectedIdleTime.
 */
void LP_enterSleep(TickType_t xExpectedIdleTime)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    uint32_t actual_sleep_ms = 0;
    uint32_t sleep_ms;

    switch (_mode)
    {
        /** LP_DISABLED_MODE */
        case LP_DISABLED_MODE:
        {
            // do nothing
            return;
        }

        /** LP_SLEEP_MODE */
        case LP_SLEEP_MODE:
        {
            sleep_ms = pdTICKS_TO_MS(xExpectedIdleTime);
            uint32_t sleep_latency =
            #if defined (CY_CFG_PWR_SLEEP_LATENCY)
            CY_CFG_PWR_SLEEP_LATENCY +
            #endif
            0;
            if (sleep_ms > sleep_latency) {
                result = cyhal_syspm_tickless_sleep(
                    _lptimer,
                    (sleep_ms - sleep_latency),
                    &actual_sleep_ms
                );
            } else {
                result = CY_RTOS_TIMEOUT;
            }
            break;
        }

        /** LP_DEEP_SLEEP_MODE */
        case LP_DEEP_SLEEP_MODE:
        {
            sleep_ms = pdTICKS_TO_MS(xExpectedIdleTime);
            // Adjust the deep-sleep time by the sleep/wake latency if set.
            #if defined(CY_CFG_PWR_DEEPSLEEP_LATENCY) || \
            defined(CY_CFG_PWR_DEEPSLEEP_RAM_LATENCY)
            uint32_t deep_sleep_latency = cyabs_rtos_get_deepsleep_latency();
            if (sleep_ms > deep_sleep_latency) {
                result = cyhal_syspm_tickless_deepsleep(
                    _lptimer,
                    (sleep_ms - deep_sleep_latency),
                    &actual_sleep_ms
                );
            } else {
                result = CY_RTOS_TIMEOUT;
            }
            #else \
            // defined(CY_CFG_PWR_DEEPSLEEP_LATENCY) ||
            // defined(CY_CFG_PWR_DEEPSLEEP_RAM_LATENCY)
            result = cyhal_syspm_tickless_deepsleep(_lptimer, sleep_ms, &actual_sleep_ms);
            #endif
            break;
        }

        default:
        {
            HAL_ASSERT(0, __FILE__, __LINE__);  // unexpected behavior
        }
    }

    /** Notify RTOS about wakeup from Sleep */
    if (result == CY_RSLT_SUCCESS) {
        // If you hit this assert, the latency time (CY_CFG_PWR_DEEPSLEEP_LATENCY) should
        // be increased. This can be set though the Device Configurator, or by manually
        // defining the variable in cybsp.h for the TARGET platform.
        HAL_ASSERT(actual_sleep_ms <= pdTICKS_TO_MS(xExpectedIdleTime), __FILE__, __LINE__);
        vTaskStepTick(convert_ms_to_ticks(actual_sleep_ms));

        /** Start DPSLP blocking timer if QSPY cmd is received */
        if (_qspyWakeup) {
            _blockSleep = true;
            OSAL_Status_t status = OSAL_SUCCESS;
            OSAL_TIMER_START(
                OSAL_TIMER_GET_HANDLE(blockSleepTimer),
                2000U, // 2000 ms
                status
            );
            if (status != OSAL_SUCCESS) {
                HAL_ASSERT(0, __FILE__, __LINE__);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
// vApplicationSleep
//
/** User defined tickless idle sleep function.
 *
 * Provides a implementation for portSUPPRESS_TICKS_AND_SLEEP macro that allows
 * the device to attempt to deep-sleep for the idle time the kernel expects before
 * the next task is ready. This function disables the system timer and enables low power
 * timer that can operate in deep-sleep mode to wake the device from deep-sleep after
 * expected idle time has elapsed.
 *
 * @param[in] xExpectedIdleTime     Total number of tick periods before
 *                                  a task is due to be moved into the Ready state.
 */
//--------------------------------------------------------------------------------------------------
void vApplicationSleep(TickType_t xExpectedIdleTime)
{
    static cyhal_lptimer_t timer;
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (NULL == _lptimer) {
        result = cyhal_lptimer_init(&timer);
        if (result == CY_RSLT_SUCCESS) {
            _lptimer = &timer;
        } else {
            HAL_ASSERT(0, __FILE__, __LINE__);
        }
    }

    if (NULL != _lptimer) {
        /** Disable interrupts so that nothing can change the status of the RTOS while
         * we try to go to sleep or deep-sleep.
         */
        uint32_t status = cyhal_system_critical_section_enter();
        eSleepModeStatus sleep_status = eTaskConfirmSleepModeStatus();

        if (sleep_status != eAbortSleep) {
            // By default, the device will deep-sleep in the idle task unless if the device
            // configurator overrides the behaviour to sleep in the System->Power->RTOS->System
            // Idle Power Mode setting
            LP_enterSleep(xExpectedIdleTime);
        }

        cyhal_system_critical_section_exit(status);
    } else {
        HAL_ASSERT(0, __FILE__, __LINE__);   // timer should be allocated
    }
}

/* [] END OF FILE */
