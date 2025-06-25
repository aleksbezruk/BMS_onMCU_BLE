#include <stdbool.h>
#include <stdint.h>

// FSL
#include "fsl_common.h"

// HAL
#include "hal.h"
#include "hal_led.h"
#include "hal_gpio.h"
#include "hal_adc.h"
// QSPY
#include "qspyHelper.h"
#include "qs.h"

// LED tests
volatile bool test_green_led;
volatile bool test_red_led;
volatile uint8_t led_test_num;

// LED tests
#define LED_NUM_TESTS   3U
#define SMALL_DELAY 100000u
#define BIG_DELAY (2 * SMALL_DELAY)
#define GREEN_ON_INTERVAL SMALL_DELAY
#define GREEN_OFF_INTERVAL (2 * SMALL_DELAY)
#define GREEN_TOGGLE_INTERVAL (3 * SMALL_DELAY)
#define RED_ON_INTERVAL SMALL_DELAY
#define RED_OFF_INTERVAL (2 * SMALL_DELAY)
#define RED_TOGGLE_INTERVAL (3 * SMALL_DELAY)

// GPIO test
volatile bool hal_gpio_test_en;
volatile bool hal_gpio_test_disch_en;
volatile bool hal_gpio_test_chrg_en;
volatile bool hal_gpio_test_bal1;
volatile bool hal_gpio_test_bal2;
volatile bool hal_gpio_test_bal3;
volatile bool hal_gpio_test_bal4;

// UART test
// TBD

// ADC test
volatile bool hal_adc_test_en = true;
volatile uint8_t adc_test_num;
#define ADC_NUM_TESTS   3U
/**
 * Bank1
 * R1 = 1.2 MOhm, R2 = 1.2 MOhm
 */
#define ADC_BANK1_CONV_RATIO    2.0f
/**
 * Bank2
 * R1 = 1.2 MOhm, R2 = 390 kOhm
 */
#define ADC_BANK2_CONV_RATIO    4.07692308f
/**
 * Bank3
 * R1 = 2.4 MOhm, R2 = 390 kOhm
 */
#define ADC_BANK3_CONV_RATIO    7.153843402f
/**
 * Bank4
 * R1 = 2.4 MOhm, R2 = 390 kOhm
 */
#define ADC_BANK4_CONV_RATIO    7.153843402f
/**
 * mV
 */
#define ADC_BANK_VOLT_CALC(v_neg, v_pos)    (int32_t) ( v_pos - v_neg )
/**
 * Convert ADC value to mV using conversion ratio
 * @note val is in [mV]
 */
#define ADC__TEST_CONV_BY_RATIO(val, ratio)   (int32_t) ( (float) val * ratio )

/** ADC offset voltage in mV */
#define ADC_OFFSET 60u

// Test completion flag
volatile bool isTestsCompleted = false;
// QSPY packet transmission flag
volatile bool isTxPacket = true;

///////////////
// Function Prototypes
///////////////
static void TEST_led(void);
static void TEST_gpio(void);
static void TEST_adc(void);
static void _IdleTask(void);

//////////////////////////////////
/// Tests code ...
//////////////////////////////////

#define NOP() __asm("nop")

static void OS_initTimer_(void)
{
    (void)SysTick_Config(SystemCoreClock / HAL_TICKS_PER_SEC);
    NVIC_SetPriority(SysTick_IRQn, 1U);
}

int main(void)
{
    HAL_status_t hwStatus = HAL_init_hardware();
    if (hwStatus != HAL_STATUS_OK) {
        HAL_ASSERT(0);
    }

    HAL_LED_init_green();
    HAL_LED_init_red();

    /** Init QSPY */
    QS_onStartup(NULL);

    /** Init QSPY dictionary & filters */
    QS_addUsrRecToDic(MAIN);
    QS_addUsrRecToDic(ADC_RCD);
    QS_initGlbFilters();

    /** dictionaries... */
    QS_FUN_DICTIONARY(&_IdleTask);

    /** Start OS timer */
    OS_initTimer_();

    test_green_led = true;
    while(1) {
        if (!isTestsCompleted) {
            /** LED tests */
            TEST_led();

            /** GPIO tests */
            TEST_gpio();

            /** ADC tests */
            TEST_adc();

            /** All tests completed */
            isTestsCompleted = true;
        } else {
            if (isTxPacket) {
                // Send only once, after all tests are completed
                isTxPacket = false;
                // Send a message to QSPY indicating that all tests are completed
                QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
                    QS_STR("Tests completed successfully");
                QS_END()
                QS_FLUSH(); // Flush QSPY output
            }

            _IdleTask();
        }
    }

    return 0;
}


//////////////////////////////
/// Tests implementation
//////////////////////////////

/**
 * @brief Test LED functionality.
 * 
 * This function toggles the green and red LEDs in a specific pattern.
 * It will turn on the green LED, then turn it off, toggle it, and then
 * switch to the red LED, repeating the process until all tests are completed.
 */
static void TEST_led(void)
{
    unsigned int i = 0;

    while (led_test_num < LED_NUM_TESTS) {
        i++;
        // Green
        if (test_green_led) {
            if (i == GREEN_ON_INTERVAL) {
                HAL_LED_green_on();
            } else if (i == GREEN_OFF_INTERVAL) {
                HAL_LED_green_off();
            } else if (i == GREEN_TOGGLE_INTERVAL) {
                HAL_LED_green_toggle();
                for (i = 0; i < BIG_DELAY; i++) ;
                HAL_LED_green_toggle();
                test_green_led = false;
                test_red_led = true;
                i = 0;
            } else {
                // nothing todo, continue test
                _IdleTask();
            }
        }
        // Red
        if (test_red_led) {
            if (i == RED_ON_INTERVAL) {
                HAL_LED_red_On();
            } else if (i == RED_OFF_INTERVAL) {
                HAL_LED_red_Off();
            } else if (i == RED_TOGGLE_INTERVAL) {
                HAL_LED_red_toggle();
                for (i = 0; i < BIG_DELAY; i++) ;
                HAL_LED_red_toggle();
                test_green_led = true;
                test_red_led = false;

                i = 0;
                led_test_num++;
            } else {
                // nothing todo, continue test
                _IdleTask();
            }
        }
    }

    // All tests completed, turn off both LEDs
    HAL_LED_green_off();
    HAL_LED_red_Off();

    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("LED tests completed successfully");
    QS_END()

    QS_FLUSH(); // Flush QSPY output
}

/**
 * @note Semimanual test. Need to connect debug probe, Enable GPIO test & Enable appropriate test.
 */
static void TEST_gpio(void)
{
    if (hal_gpio_test_en) {
        /** Init pins */
        HAL_GPIO_init_pin(
            HAL_DISCHARGE_PORT,
            HAL_DISCHARGE_PIN,
            HAL_GPIO_DIGITAL_OUTPUT,
            HAL_GPIO_PULL_DISABLED,
            HAL_GPIO_DRIVE_HIGH,
            HAL_DISCHARGE_OFF
        );

        HAL_GPIO_init_pin(
            HAL_CHARGE_PORT,
            HAL_CHARGE_PIN,
            HAL_GPIO_DIGITAL_OUTPUT,
            HAL_GPIO_PULL_DISABLED,
            HAL_GPIO_DRIVE_HIGH,
            HAL_CHARGE_OFF
        );

        HAL_GPIO_init_pin(
            HAL_BAL_BANK1_PORT,
            HAL_BAL_BANK1_PIN,
            HAL_GPIO_DIGITAL_OUTPUT,
            HAL_GPIO_PULL_DISABLED,
            HAL_GPIO_DRIVE_HIGH,
            HAL_BAL_BANK1_OFF
        );

        HAL_GPIO_init_pin(
            HAL_BAL_BANK2_PORT,
            HAL_BAL_BANK2_PIN,
            HAL_GPIO_DIGITAL_OUTPUT,
            HAL_GPIO_PULL_DISABLED,
            HAL_GPIO_DRIVE_HIGH,
            HAL_BAL_BANK2_OFF
        );

        HAL_GPIO_init_pin(
            HAL_BAL_BANK3_PORT,
            HAL_BAL_BANK3_PIN,
            HAL_GPIO_DIGITAL_OUTPUT,
            HAL_GPIO_PULL_DISABLED,
            HAL_GPIO_DRIVE_HIGH,
            HAL_BAL_BANK3_OFF
        );

        HAL_GPIO_init_pin(
            HAL_BAL_BANK4_PORT,
            HAL_BAL_BANK4_PIN,
            HAL_GPIO_DIGITAL_OUTPUT,
            HAL_GPIO_PULL_DISABLED,
            HAL_GPIO_DRIVE_HIGH,
            HAL_BAL_BANK4_OFF
        );
    }

    while(hal_gpio_test_en) {
        /** Discharge test */
        while(hal_gpio_test_disch_en) {
            HAL_GPIO_set_pin(
                HAL_DISCHARGE_PORT,
                HAL_DISCHARGE_PIN,
                HAL_DISCHARGE_ON
            );
            NOP();
            HAL_GPIO_set_pin(
                HAL_DISCHARGE_PORT,
                HAL_DISCHARGE_PIN,
                HAL_DISCHARGE_OFF
            );
            NOP();
        }

        /** Charge test */
        while(hal_gpio_test_chrg_en) {
            HAL_GPIO_set_pin(
                HAL_CHARGE_PORT,
                HAL_CHARGE_PIN,
                HAL_CHARGE_ON
            );
            NOP();
            HAL_GPIO_set_pin(
                HAL_CHARGE_PORT,
                HAL_CHARGE_PIN,
                HAL_CHARGE_OFF
            );
            NOP();
        }

        /** BAL1 test */
        while(hal_gpio_test_bal1) {
            HAL_GPIO_set_pin(
                HAL_BAL_BANK1_PORT,
                HAL_BAL_BANK1_PIN,
                HAL_BAL_BANK1_ON
            );
            NOP();
            HAL_GPIO_set_pin(
                HAL_BAL_BANK1_PORT,
                HAL_BAL_BANK1_PIN,
                HAL_BAL_BANK1_OFF
            );
            NOP();
        }

        /** BAL2 test */
        while(hal_gpio_test_bal2) {
            HAL_GPIO_set_pin(
                HAL_BAL_BANK2_PORT,
                HAL_BAL_BANK2_PIN,
                HAL_BAL_BANK2_ON
            );
            NOP();
            HAL_GPIO_set_pin(
                HAL_BAL_BANK2_PORT,
                HAL_BAL_BANK2_PIN,
                HAL_BAL_BANK2_OFF
            );
            NOP();
        }

        /** BAL3 test */
        while(hal_gpio_test_bal3) {
            HAL_GPIO_set_pin(
                HAL_BAL_BANK3_PORT,
                HAL_BAL_BANK3_PIN,
                HAL_BAL_BANK3_ON
            );
            NOP();
            HAL_GPIO_set_pin(
                HAL_BAL_BANK3_PORT,
                HAL_BAL_BANK3_PIN,
                HAL_BAL_BANK3_OFF
            );
            NOP();
        }

        /** BAL4 test */
        while(hal_gpio_test_bal4) {
            HAL_GPIO_set_pin(
                HAL_BAL_BANK4_PORT,
                HAL_BAL_BANK4_PIN,
                HAL_BAL_BANK4_ON
            );
            NOP();
            HAL_GPIO_set_pin(
                HAL_BAL_BANK4_PORT,
                HAL_BAL_BANK4_PIN,
                HAL_BAL_BANK4_OFF
            );
            NOP();
        }
    }
}

/**
 * @brief Test ADC functionality.
 */
void TEST_adc(void)
{
    if (!hal_adc_test_en) {
        return; // ADC test is disabled
    }

    int32_t full_mv = 0;
    int32_t bank1_mv = 0;
    int32_t bank1_pot_mv = 0;
    int32_t bank2_mv = 0;
    int32_t bank2_pot_mv = 0;
    int32_t bank3_mv = 0;
    int32_t bank3_pot_mv = 0;
    int32_t bank4_mv = 0;
    int32_t bank4_pot_mv = 0;

    /** Initialize ADC */
    HAL_ADC_init();

    while(adc_test_num < ADC_NUM_TESTS) {
        adc_test_num++;
        /** Read ADC value, Bank1 */
        int32_t bank1_raw_mv = HAL_ADC_read(HAL_ADC_CHANNEL_0);
        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Bank1_raw_mV = ");
            QS_I32(0, bank1_raw_mv);
        QS_END()
        QS_FLUSH(); // Flush QSPY output
        bank1_pot_mv = ADC__TEST_CONV_BY_RATIO(bank1_raw_mv, ADC_BANK1_CONV_RATIO);
        bank1_mv = ADC_BANK_VOLT_CALC(0, bank1_pot_mv); // Calculate voltage in mV
        /** Compensate for offset */
        bank1_mv -= ADC_OFFSET;
        SDK_DelayAtLeastUs(1000u, SystemCoreClock); 

        /** Read ADC value, Bank2 */
        int32_t bank2_raw_mv = HAL_ADC_read(HAL_ADC_CHANNEL_1);
        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Bank2_raw_mV = ");
            QS_I32(0, bank2_raw_mv);
        QS_END()
        QS_FLUSH(); // Flush QSPY output
        bank2_pot_mv = ADC__TEST_CONV_BY_RATIO(bank2_raw_mv, ADC_BANK2_CONV_RATIO);
        bank2_mv = ADC_BANK_VOLT_CALC(bank1_pot_mv, bank2_pot_mv); // Calculate voltage in mV
        bank2_mv -= ADC_OFFSET;
        SDK_DelayAtLeastUs(1000u, SystemCoreClock); 

        /** Read ADC value, Bank3 */
        int32_t bank3_raw_mv = HAL_ADC_read(HAL_ADC_CHANNEL_2);
        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Bank3_raw_mV = ");
            QS_I32(0, bank3_raw_mv);
        QS_END()
        QS_FLUSH(); // Flush QSPY output
        bank3_pot_mv = ADC__TEST_CONV_BY_RATIO(bank3_raw_mv, ADC_BANK3_CONV_RATIO);
        bank3_mv = ADC_BANK_VOLT_CALC(bank2_pot_mv, bank3_pot_mv); // Calculate voltage in mV
        bank3_mv -= ADC_OFFSET;
        SDK_DelayAtLeastUs(1000u, SystemCoreClock); 

        /** Read ADC value, Bank4 */
        int32_t bank4_raw_mv = HAL_ADC_read(HAL_ADC_CHANNEL_3);
        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Bank4_raw_mV = ");
            QS_I32(0, bank4_raw_mv);
        QS_END()
        QS_FLUSH(); // Flush QSPY output
        full_mv = bank4_pot_mv = ADC__TEST_CONV_BY_RATIO(bank4_raw_mv, ADC_BANK4_CONV_RATIO);
        full_mv -= ADC_OFFSET; // Calculate full voltage in mV 
        bank4_mv = ADC_BANK_VOLT_CALC(bank3_pot_mv, bank4_pot_mv); // Calculate voltage in mV
        bank4_mv -= ADC_OFFSET;

        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Banks volt: ");
            QS_I16(0, (int16_t) bank1_mv);
            QS_I16(0, (int16_t) bank2_mv);
            QS_I16(0, (int16_t) bank3_mv);
            QS_I16(0, (int16_t) bank4_mv);
            QS_I16(0, (int16_t) full_mv);
        QS_END()

        /**Delay between tests (use NXP SDK functions) */
        SDK_DelayAtLeastUs(1000000u, SystemCoreClock); // 1 second delay
    }

    /** Deinitialize ADC */
    HAL_ADC_deinit();
    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("ADC tests completed successfully");
    QS_END()
    QS_FLUSH(); // Flush QSPY output
}

static void _IdleTask(void)
{
     /** Handle QSPY communication */
     QS_onIdle();
}


/* End of FILE */