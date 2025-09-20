/**
 * @file  hal.c
 *
 * @brief Implementation of generic HAL functions like init board/hardware, generic utils
 *
 * @note QN9080
 * 
 * @version 0.6.0
 */

#include "fsl_power.h"
#include "clock_config.h"
#include "system_QN908XC.h"
#include "pin_mux.h"
#include "fsl_rtc.h"
#include "fsl_calibration.h"
#include "fsl_syscon.h"
#include "fsl_rf.h"
#include "fsl_flash.h"

#include "GPIO_Adapter.h"
#include "gpio_pins.h"

#include "hal.h"
#include "hal_gpio.h"
#include "hal_led.h"

#include "Flash_Adapter.h"

// NVDS (Non-Volatile Data Storage) for BLE ROM functions
#include "nvds.h"

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

    /** Set RF Rx Mode */
    RF_ConfigRxMode(SYSCON, gBleRfRxMode);

    /** Initialize the FLASH driver via FLASH Adapter */
    NV_Init();

    /** Initialize NVDS (Non-Volatile Data Storage) for BLE ROM functions */
    // NVDS initialization must be done after NV_Init() which sets up gFlashConfig
    // This enables ROM BLE functions like nvds_get, nvds_put, etc.
    extern flash_config_t gFlashConfig;  // Declared in Flash_Adapter.c
    
    // Temporarily set flash block base to FSL_FEATURE_FLASH_BASE_ADDR for NVDS operations
    uint32_t originalBlockBase = gFlashConfig.blockBase;
    gFlashConfig.blockBase = FSL_FEATURE_FLASH_BASE_ADDR;
    
    // Initialize NVDS with ROM function
    uint8_t nvds_status = nvds_init(
        (uint8_t *)CFG_NVDS_ADDRESS,        // NVDS space in FLASH (0x2107F000)
        CFG_NVDS_SIZE,                      // Size: 2KB
        (uint8_t *)CFG_NVDS_BACKUP_ADDRESS, // Backup space (0x2107E800)
        NULL,                               // Use default ROM API
        &gFlashConfig                       // Flash configuration
    );
    
    // Restore original flash block base for normal operations
    gFlashConfig.blockBase = originalBlockBase;
    
    // NVDS_FAIL (status=1) is normal for fresh devices - continue execution like NXP examples
    // Only log the status without failing - ROM BLE functions are now accessible regardless
    (void)nvds_status; // Suppress unused variable warning

    /** Set crystal load capacitance */
    // if (!_HAL_isDebuggerConnected()) {
    //     _HAL_setCrystalLoadCap();
    // }
    _HAL_setCrystalLoadCap();

    /** System calibration */
    // if (!_HAL_isDebuggerConnected()) {
    //     CALIB_SystemCalib();
    // }
    CALIB_SystemCalib();

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

// ==================================
// Faults handling functions
// ==================================
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} fault_stack_frame_t;
/**
 * @brief HardFault handler function
 * 
 * @details This function is called when a HardFault exception occurs.
 *          It logs the fault information and enters an infinite loop.
 *          It also blinks the red LED rapidly to indicate a hard fault.
 *
 * @param[in] stack_frame Pointer to the stack frame at the time of the fault
 * 
 * @retval None
 */
void HardFault_Handler_C(uint32_t *stack_frame) {
    volatile fault_stack_frame_t *frame = (fault_stack_frame_t *)stack_frame;
    volatile uint32_t fault_pc = frame->pc;
    volatile uint32_t fault_lr = frame->lr;
    volatile uint32_t cfsr = SCB->CFSR;  // Configurable Fault Status Register
    volatile uint32_t hfsr = SCB->HFSR;  // HardFault Status Register
    volatile uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address Register
    volatile uint32_t bfar = SCB->BFAR;   // BusFault Address Register

    // Log fault information via QSPY
    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("HardFault: PC=");
        QS_U32(0, fault_pc);
        QS_STR(" LR=");
        QS_U32(0, fault_lr);
        QS_STR(" CFSR=");
        QS_U32(0, cfsr);
        QS_STR(" HFSR=");
        QS_U32(0, hfsr);
        QS_STR(" MMFAR=");
        QS_U32(0, mmfar);
        QS_STR(" BFAR=");
        QS_U32(0, bfar);
    QS_END()
    QS_FLUSH(); // Flush QSPY output

    // Blink red LED rapidly to indicate hard fault
    HAL_LED_init_red();
    for(int i = 0; i < 10; i++) {
        HAL_LED_red_toggle();
        for(volatile int j = 0; j < 100000; j++); // Simple delay
    }

    // Set breakpoint here to examine variables in debugger
    __asm("BKPT #0");

    // Infinite loop to prevent return
    while(1) {
        HAL_LED_red_toggle();
        for(volatile int j = 0; j < 10000; j++);
    }
}

/* [] END OF FILE */
