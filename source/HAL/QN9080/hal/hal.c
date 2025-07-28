/**
 * @file  hal.c
 *
 * @brief Implementation of generic HAL functions like init board/hardware, generic utils
 *
 * @note QN9080
 * 
 * @version 0.5.0
 */

#include "fsl_power.h"
#include "clock_config.h"
#include "system_QN908XC.h"
#include "pin_mux.h"
#include "fsl_rtc.h"
#include "fsl_calibration.h"
#include "fsl_syscon.h"
#include "GPIO_Adapter.h"
#include "gpio_pins.h"

#include "hal.h"
#include "hal_gpio.h"

/* =========================
 * Defines
 * ========================= */

/* =========================
 * Functions prototype
 * ========================= */
static void _HAL_initLed(void);
static void _HAL_setCrystalLoadCap(void);
static inline bool _HAL_isDebuggerConnected(void);

/* =========================
 * Private data
 * ========================= */

/* =========================
 * Code
 * ========================= */
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
 *         Other values from \ref HAL_status_t may be returned in the future to indicate error conditions.
 * 
 */
HAL_status_t HAL_init_hardware(void)
{
    /** Init DC-DC mode */
    POWER_EnableDCDC(gDCDC_Mode);

    /** Set crystal load capacitance */
    if (!_HAL_isDebuggerConnected()) {
        _HAL_setCrystalLoadCap();
    }

    /** System calibration */
    if (!_HAL_isDebuggerConnected()) {
        CALIB_SystemCalib();
    }

    /** Relocate VTOR and copy Vector Table to RAM */
    extern uint32_t __VECTOR_TABLE[];
    extern uint32_t __VECTOR_RAM[];
    extern uint32_t __RAM_VECTOR_TABLE_SIZE_BYTES[];
    uint32_t __RAM_VECTOR_TABLE_SIZE = (uint32_t)(__RAM_VECTOR_TABLE_SIZE_BYTES);
    for (uint32_t n = 0; n < ((uint32_t)__RAM_VECTOR_TABLE_SIZE) / sizeof(uint32_t); n++) {
        __VECTOR_RAM[n] = __VECTOR_TABLE[n];
    }
    SCB->VTOR = (uint32_t)__VECTOR_RAM;

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

    /** Init UART, ADC pins */
    HAL_GPIO_init_pin(
        HAL_GPIO_UART_TX_PORT, 
        HAL_GPIO_UART_TX_PIN,
        HAL_GPIO_UART_TX,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_HIGH,
        1u  // initialState N/A for not GPIO pins (UART, SPI, etc.)
    );
    HAL_GPIO_init_pin(
        HAL_GPIO_UART_RX_PORT,
        HAL_GPIO_UART_RX_PIN,
        HAL_GPIO_UART_RX,
        HAL_GPIO_PULL_UP,
        HAL_GPIO_DRIVE_NONE,
        1u  // initialState N/A for Inputs
    );

    HAL_GPIO_init_pin(
        HAL_GPIO_ADC1_PORT,
        HAL_GPIO_ADC1_PIN,
        HAL_GPIO_ANALOG_INPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_NONE,
        1u  // initialState N/A for Inputs
    );
    HAL_GPIO_init_pin(
        HAL_GPIO_ADC2_PORT,
        HAL_GPIO_ADC2_PIN,
        HAL_GPIO_ANALOG_INPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_NONE,
        1u  // initialState N/A for Inputs
    );
    HAL_GPIO_init_pin(
        HAL_GPIO_ADC3_PORT,
        HAL_GPIO_ADC3_PIN,
        HAL_GPIO_ANALOG_INPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_NONE,
        1u  // initialState N/A for Inputs
    );
    HAL_GPIO_init_pin(
        HAL_GPIO_ADC4_PORT,
        HAL_GPIO_ADC4_PIN,
        HAL_GPIO_ANALOG_INPUT,
        HAL_GPIO_PULL_DISABLED,
        HAL_GPIO_DRIVE_NONE,
        1u  // initialState N/A for Inputs
    );

    /** Calibrate RTC */
#if (defined(BOARD_XTAL1_CLK_HZ) && (BOARD_XTAL1_CLK_HZ != CLK_XTAL_32KHZ))
    RCO32K_InitSwCalib(RCO32K_CALIBRATION_INTERVAL);         /* Enable periodic 32k RCO calibration */
#else
    RTC_Calibration(RTC, kRTC_BackwardCalibration, 0x6000);  /* by default RTC is counting 32000 ticks per seconds, adjust that! */
#endif

    /** Init LED */
    _HAL_initLed();

    return HAL_STATUS_OK;
}

// ==================================
// Spare, private functions
// ==================================
/**
 * @brief Init LEDs
 * 
 * @note This function is called from HAL_init_hardware() to initialize LEDs.
 * 
 * @param None
 * 
 * @retval None
 */
static void _HAL_initLed(void)
{
    BOARD_InitLEDs();
    GpioOutputPinInit(ledPins, gLEDsOnTargetBoardCnt_c);
}

/**
 * @brief Set crystal load capacitance.
 *
 * @details See reference Guide "QN908x Crystals Load Capacitance Calibration"
 * @note This function is used to set the load capacitance for the crystal oscillator.
 *        The load capacitance is used to match the crystal oscillator to the load capacitance of the circuit.
 *        The load capacitance is specified in the reference guide and is typically set to
 *        a value between 0 and 3, where 0 is the lowest load capacitance and 3 is the highest load capacitance.
 *        The load capacitance is used to match the crystal oscillator to the load capacitance of the circuit.
 * This function sets the load capacitance for the crystal oscillator.
 * The value is limited to a maximum of 3.
 *
 * @note 32MHz crystal: ANA_CTRL0[XTAL_LOAD_CAP] - register-controlled load cap of the XTAL in
 *       normal mode. XTAL load cap =5 pF + 0.35 pF * XTAL_LOAD_CAP + 5 pF * XTAL_EXTRA_CAP
 *       8pF should be set for 32MHz crystal => XTAL_LOAD_CAP = 8
 * 
 * @note 32kHz crystal: ANA_CTRL0[XTAL32K_LOAD_CAP] - load cap selection of XTAL32K; XTAL32K
 *       load cap = 3.6 pF + 0.4 pF * XTAL32K_LOAD_CAP + 6.4 pF * XTAL32K_EXTRA_CAP
 *       7pF should be set for 32kHz crystal => XTAL32K_LOAD_CAP = 7
 *
 * @param None
 *
 * @retval None
 */
static void _HAL_setCrystalLoadCap(void)
{
    /** 32 MHz crystal */
    SYSCON_SetLoadCap(SYSCON, 1, 8U); // 1 = 16/32MHz, loadCap = 8
    /** 32 kHz crystal */
    SYSCON_SetLoadCap(SYSCON, 0, 7U); // 0 = 32kHz, loadCap = 7
}

/**
 * @brief Checks if a debugger is connected to the MCU.
 *
 * @details This function checks the Debug Halting Control and Status Register (DHCSR)
 *          to determine if a debugger is connected. The C_DEBUGEN bit in DHCSR is set
 *          when a debugger is connected and has enabled debug mode.
 *
 * @note This is useful to skip crystal calibration and system calibration when debugging,
 *       as these operations can interfere with the debugger's ability to maintain connection
 *       and properly halt/step through code.
 *
 * @param None
 *
 * @retval true if debugger is connected, false otherwise
 */
static inline bool _HAL_isDebuggerConnected(void)
{
    /* Check the Debug Halting Control and Status Register (DHCSR) */
    /* C_DEBUGEN bit (bit 0) is set when debugger is connected */
    return (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0;
}

/* [] END OF FILE */
