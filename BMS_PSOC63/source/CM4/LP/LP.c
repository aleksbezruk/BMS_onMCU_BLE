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
 *                          portSUPPRESS_TICKS_AND_SLEEP() macro ; <br>
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
 *                      2.2 @todo LP for SDK & BLE stack  <br>
 *                          - #define CY_CFG_PWR_SYS_IDLE_MODE (set in MTB Generated sources): <br>
 *                              #define CY_CFG_PWR_MODE_LP 0x01UL               <br>
 *                                  #define CY_CFG_PWR_MODE_ULP 0x02UL          <br>
 *                                  #define CY_CFG_PWR_MODE_ACTIVE 0x04UL       <br>
 *                                  #define CY_CFG_PWR_MODE_SLEEP 0x08UL        <br>
 *                                  #define CY_CFG_PWR_MODE_DEEPSLEEP 0x10UL    <br>
 *                          - #define CY_CFG_PWR_MODE_SLEEP - looks like setup CPU core Sleep mode ( see cyhal_syspm_sleep() ); <br>
 *                          - #define CY_CFG_PWR_MODE_DEEPSLEEP: <br>
 *                              - Core regulator : <br>
 *                                  - @todo Normal Current LDO ??? ; <br>
 *                                  - @todo Minimum Current LDO ??? ; <br>
 *                                  - @todo Normal Current Buck ??? ; <br>
 *                                  - @todo Minimum Current Buck ??? ; <br>
 *                              - System Active power mode : <br>
 *                                  - LP - is the default operating mode of the device after reset and provides maximum system performance. ; <br>
 *                                  - @todo ULP Ultra Low Power (ULP) mode is identical to LP mode with a performance tradeoff made to achieve lower <br>
 *                                    system current. This tradeoff lowers the core operating voltage, which then requires reduced operating clock
 *                                    frequency and limited high-frequency clock sources. ; <br>
 *                              - CY_CFG_PWR_DEEPSLEEP_LATENCY / CY_CFG_PWR_DEEPSLEEP_LATENCY-> ??? ; <br>
 *                              - @todo CYHAL_SYSPM_RSLT_DEEPSLEEP_LOCKED -> ??? ; <br>
 *                              - cyhal_syspm_tickless_deepsleep() -> _cyhal_syspm_deepsleep_internal() -> Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT) ; <br>
 *                              - @todo wake up sources & mechanism -> ??? ; <br>
 *                              - @todo SCB operation in DPSLP ???
 *                          - #define DEEPSLEEP_ENABLE -> Local 'CY_driver' definition for the vApplicationSleep() callback ; <br>
 * 
 *                ### 3. @todo Define BMS system parameters that affects power consumption
 *                    3.1 @todo Advertising interval ???
 *                    3.2 @todo ADC measurements duration ???
 * 
 * @version 0.4.0
 */

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

#include "LP.h"
#include "qspyHelper.h"

///////////////////////
// Private data
///////////////////////
static volatile LP_modes_t _mode;

///////////////////////
// Code
///////////////////////
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

    QSPY_rx_status_t qspy_rx_status = QS_get_rxStatus();
    if (qspy_rx_status == QSPY_RX_NOT_EMPTY) {
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
 * @param None
 * 
 * @retval None
 */
void LP_enterSleep(void)
{
    switch (_mode)
    {
        case LP_DISABLED_MODE:
        {
            // do nothing
            break;
        }

        case LP_SLEEP_MODE:
        {
            /** @todo: implement */
            __asm("nop");
            break;
        }

        case LP_DEEP_SLEEP_MODE:
        {
            /** @todo: implement */
            __asm("nop");
            break;
        }

        default:
        {
            CY_ASSERT(0);   // unexpected behavior
        }
    }
}

/* [] END OF FILE */
