/**
 * @file  hal.c
 *
 * @brief Implementation of generic HAL functions like init board/hardware, generic utils
 *
 * @note QN9080
 * 
 * @version 0.5.0
 */

#include "hal.h"
#include "fsl_power.h"
#include "clock_config.h"
#include "system_QN908XC.h"
#include "pin_mux.h"
#include "fsl_rtc.h"
#include "GPIO_Adapter.h"
#include "gpio_pins.h"

///////////////////////
// Defines
///////////////////////

///////////////////////
// Functions prototype
///////////////////////
static void _initLed(void);

///////////////////////
// Private data
///////////////////////

///////////////////////
// Code
///////////////////////
/**
 * @brief Generic API to init underlying hardware. Purpose is
 *        to configure all hardware (power, clocks, peripheral) to be ready for
 *        further BMS operations. 
 * 
 * @note Configures peripherals pins as well: UART, ADC pins.
 * 
 * @details In general it may use low level BSP provided by
 *          particular Vendor if such exist and (or) use autogenerate vendor tool(s)
 *          that allow to configure hardware using convenient GUI. Using such COTS tools allow
 *          us to speed up development/prototyping process.
 * 
 * @param   None
 * 
 * @retval See \ref HAL_status_t. HAL_STATUS_OK - if success.
 * 
 */
HAL_status_t HAL_init_hardware(void)
{
    /** Init DC-DC mode */
    POWER_EnableDCDC(gDCDC_Mode);

    /** Init clocks at boot-up */
#if gBleUseHSClock2MbpsPhy_c
    BOARD_BootClockHSRUN();
#else
    BOARD_InitBootClocks();
#endif

    /** Update SystemCoreClock if default clock value (16MHz) has changed */
    SystemCoreClockUpdate();

    /** Init board pins */
    BOARD_InitPins();

    /** @todo: Init UART, ADC pins */

    /** Calibrate RTC */
#if (defined(BOARD_XTAL1_CLK_HZ) && (BOARD_XTAL1_CLK_HZ != CLK_XTAL_32KHZ))
    RCO32K_InitSwCalib(RCO32K_CALIBRATION_INTERVAL);         /* Enable periodic 32k RCO calibration */
#else
    RTC_Calibration(RTC, kRTC_BackwardCalibration, 0x6000);  /* by default RTC is counting 32000 ticks per seconds, adjust that! */
#endif

    /** Init LED */
    _initLed();

    return HAL_STATUS_OK;
}

///////////////////////
// Spare, private functions
///////////////////////
static void _initLed(void)
{
    BOARD_InitLEDs();
    GpioOutputPinInit(ledPins, gLEDsOnTargetBoardCnt_c);
}

/* [] END OF FILE */
