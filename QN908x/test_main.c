#include "pin_mux.h"
#include "clock_config.h"
#include "system_QN908XC.h"
#include "fsl_rtc.h"
#include "fsl_power.h"
#include "LED.h"
#include "GPIO_Adapter.h"

#define gDCDC_Mode 1
/* Specifies the number of physical LEDs on the target board */
// #define gLEDsOnTargetBoardCnt_c 3

extern gpioOutputPinConfig_t ledPins[];

void hardware_init(void)
{
    POWER_EnableDCDC(gDCDC_Mode);

#if gBleUseHSClock2MbpsPhy_c
    BOARD_BootClockHSRUN();
#else
    BOARD_InitBootClocks();
#endif

    /* Update SystemCoreClock if default clock value (16MHz) has changed */
    SystemCoreClockUpdate();

    BOARD_InitPins();

#if (defined(BOARD_XTAL1_CLK_HZ) && (BOARD_XTAL1_CLK_HZ != CLK_XTAL_32KHZ))
    RCO32K_InitSwCalib(RCO32K_CALIBRATION_INTERVAL);         /* Enable periodic 32k RCO calibration */
#else
    RTC_Calibration(RTC, kRTC_BackwardCalibration, 0x6000);  /* by default RTC is counting 32000 ticks per seconds, adjust that! */
#endif
}

static void _initLed(void)
{
    BOARD_InitLEDs();
    GpioOutputPinInit(ledPins, gLEDsOnTargetBoardCnt_c);
}

static void _toggleGreen(void)
{
    Led2Toggle();
}

int main(void)
{
    unsigned int i = 0;

    hardware_init();
    _initLed();

    while(1) {
        i++;
        if (i == 1000000) {
            _toggleGreen();
            i = 0;
        }
    }
    return 0;
}


/* End of FILE */