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
 * @version 0.4.0
 */

#include <stdbool.h>

#include "cy_pdl.h"
#include "cyhal.h"
#include "cyhal_syspm.h"
#include "cybsp.h"

#include "LP.h"
#include "qspyHelper.h"
#include "ADC.h"

#include "cycfg.h"

// RTOS includes
#include "FreeRTOS.h"
#include "cyabs_rtos.h"

///////////////////////
// Functions prototype
///////////////////////
static cy_en_syspm_status_t _checkReadyDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode);
static cy_en_syspm_status_t _beforeDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode);
static cy_en_syspm_status_t _afterDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode);
static void blockSleepTimerCallback_(cy_timer_callback_arg_t arg);
void LP_enterSleep(TickType_t xExpectedIdleTime);

uint32_t cyabs_rtos_get_deepsleep_latency(void);

///////////////////////
// Defines
///////////////////////
#define pdTICKS_TO_MS(xTicks)    ( ( ( TickType_t ) ( xTicks ) * 1000u ) / configTICK_RATE_HZ )

///////////////////////
// Private data
///////////////////////
static volatile LP_modes_t _mode;

static cy_stc_syspm_callback_params_t _beforeDeepSleepCallbackParams;
static cy_stc_syspm_callback_t _pmBeforeDeepSlpCallback = {
    .callback = _beforeDeepSleepCallback,
    .type = CY_SYSPM_DEEPSLEEP,
    .skipMode = CY_SYSPM_SKIP_CHECK_READY | CY_SYSPM_SKIP_CHECK_FAIL | CY_SYSPM_SKIP_AFTER_TRANSITION, // only before transition
    .callbackParams = &_beforeDeepSleepCallbackParams,
    .prevItm = NULL,    // for CY driver intranal usage
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

static volatile bool _blockSleep;
static volatile bool _qspyWakeup;
static cy_timer_t _blockSleepTimer;
static cyhal_lptimer_t* _lptimer = NULL;

///////////////////////
// Code
///////////////////////

LP_status_t LP_init(void)
{
    LP_status_t status = LP_INIT_STATUS_OK;
    cy_rslt_t result;

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

    result = cy_rtos_timer_init(
        &_blockSleepTimer,
        CY_TIMER_TYPE_ONCE,
        blockSleepTimerCallback_,
        0U // arg
    );

    if (result != CY_RSLT_SUCCESS) {
        status = LP_INIT_STATUS_FAIL;
    }

    return status;
}

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

static cy_en_syspm_status_t _afterDeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams, cy_en_syspm_callback_mode_t mode)
{
    (void) callbackParams;

    /** Check if wakeup is due to QSPY */
    if (Cy_GPIO_GetInterruptStatusMasked(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN) == 1U) {
        _qspyWakeup = true;
    }

    /** Reconfigure QSPY RX line to enable UART receiver back, Disable GPIO IRQ
     * \ref CYBSP_UART_RX_PIN, CYBSP_UART_RX_config
     */
    Cy_GPIO_Pin_Init(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN, &CYBSP_UART_RX_config);
    Cy_GPIO_SetInterruptMask(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN, 0U);
    Cy_GPIO_ClearInterrupt(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN);
    NVIC_DisableIRQ(ioss_interrupts_gpio_5_IRQn);
    NVIC_ClearPendingIRQ(ioss_interrupts_gpio_5_IRQn);

    /** Start DPSLP blocking timer if QSPY cmd is received */
    if (_qspyWakeup) {
        _blockSleep = true;
        cy_rslt_t result = cy_rtos_timer_start(&_blockSleepTimer, 100U); // 100 ms
        if (result != CY_RSLT_SUCCESS) {
            CY_ASSERT(0);
        }
    }

    return CY_SYSPM_SUCCESS;
}

void ioss_interrupts_gpio_5_IRQHandler(void)
{
    /** Check if wakeup is due to QSPY */
    if (Cy_GPIO_GetInterruptStatusMasked(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN) == 1U) {
        Cy_GPIO_ClearInterrupt(CYBSP_UART_RX_PORT, CYBSP_UART_RX_PIN);
        _qspyWakeup = true;
    }
}

static void blockSleepTimerCallback_(cy_timer_callback_arg_t arg)
{
    _blockSleep = false;
}

/**
 * @brief Get BMS peripherals rediness status for Low Power mode
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
 * @brief Get BMS peripherals rediness status for Low Power mode
 * 
 * @param[in] mode - Low Power mode to set, see @LP_modes_t
 * 
 * @retval see @LP_periph_ready_t
 */
void LP_setMode(LP_modes_t mode)
{
    CY_ASSERT((mode >= LP_DISABLED_MODE) && (mode <= LP_SHELF_MODE));
    _mode = mode;
}

/**
 * @brief Get BMS peripherals rediness status for Low Power mode
 * 
 * @param[in] xExpectedIdleTime - expected time to sleep [RTOS ticks]
 * 
 * @retval None
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
            CY_ASSERT(0);   // unexpected behavior
        }
    }

    /** Notify RTOS about wakeup from Sleep */
    if (result == CY_RSLT_SUCCESS) {
        // If you hit this assert, the latency time (CY_CFG_PWR_DEEPSLEEP_LATENCY) should
        // be increased. This can be set though the Device Configurator, or by manually
        // defining the variable in cybsp.h for the TARGET platform.
        CY_ASSERT(actual_sleep_ms <= pdTICKS_TO_MS(xExpectedIdleTime));
        vTaskStepTick(convert_ms_to_ticks(actual_sleep_ms));
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
            CY_ASSERT(false);
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
        CY_ASSERT(false);   // timer should be allocated
    }
}

/* [] END OF FILE */
