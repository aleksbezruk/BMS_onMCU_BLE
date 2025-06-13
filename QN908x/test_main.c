#include <stdbool.h>
#include <stdint.h>
// HAL
#include "hal.h"
#include "hal_led.h"
#include "hal_gpio.h"
// QSPY
#include "qspyHelper.h"

// LED stuff
volatile bool test_green_led;
volatile bool test_red_led;
volatile uint8_t led_test;

// LED tests
#define LED_NUM_TESTS   3U
#define SMALL_DELAY 100000u
#define BIG_DELAY 2*SMALL_DELAY
#define GREEN_ON_INTERVAL SMALL_DELAY
#define GREEN_OFF_INTERVAL 2*SMALL_DELAY
#define GREEN_TOGGLE_INTERVAL 3*SMALL_DELAY
#define RED_ON_INTERVAL SMALL_DELAY
#define RED_OFF_INTERVAL 2*SMALL_DELAY
#define RED_TOGGLE_INTERVAL 3*SMALL_DELAY

// GPIO test
volatile bool hal_gpio_test_en;
volatile bool hal_gpio_test_disch_en;
volatile bool hal_gpio_test_chrg_en;
volatile bool hal_gpio_test_bal1;
volatile bool hal_gpio_test_bal2;
volatile bool hal_gpio_test_bal3;
volatile bool hal_gpio_test_bal4;

// UART test
volatile bool isSentPacket;

///////////////////////////////////
/// Fuc Prototypes
///////////////////////////////////
static void TEST_gpio(void);
static void _IdleTask(void);

//////////////////////////////////
/// Tests code ...
//////////////////////////////////
static void OS_initTimer_(void)
{
    (void)SysTick_Config(SystemCoreClock / HAL_TICKS_PER_SEC);
    NVIC_SetPriority(SysTick_IRQn, 1U);
}

int main(void)
{
    unsigned int i = 0;

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
        /** LED tests */
        if (led_test < LED_NUM_TESTS) {
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
                    led_test++;
                } else {
                    // nothing todo, continue test
                    _IdleTask();
                }
            }
        }

        /** GPIO tests */
        TEST_gpio();

        /** Next tests */
        if (!isSentPacket) {
            QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
                QS_STR("Tests completed.");
            QS_END()
            isSentPacket = true;
        }

        _IdleTask();
    }
    return 0;
}


//////////////////////////////
/// Tests implementation
//////////////////////////////
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
            __asm("nop");
            HAL_GPIO_set_pin(
                HAL_DISCHARGE_PORT,
                HAL_DISCHARGE_PIN,
                HAL_DISCHARGE_OFF
            );
            __asm("nop");
        }

        /** Charge test */
        while(hal_gpio_test_chrg_en) {
            HAL_GPIO_set_pin(
                HAL_CHARGE_PORT,
                HAL_CHARGE_PIN,
                HAL_CHARGE_ON
            );
            __asm("nop");
            HAL_GPIO_set_pin(
                HAL_CHARGE_PORT,
                HAL_CHARGE_PIN,
                HAL_CHARGE_OFF
            );
            __asm("nop");
        }

        /** BAL1 test */
        while(hal_gpio_test_bal1) {
            HAL_GPIO_set_pin(
                HAL_BAL_BANK1_PORT,
                HAL_BAL_BANK1_PIN,
                HAL_BAL_BANK1_ON
            );
            __asm("nop");
            HAL_GPIO_set_pin(
                HAL_BAL_BANK1_PORT,
                HAL_BAL_BANK1_PIN,
                HAL_BAL_BANK1_OFF
            );
            __asm("nop");
        }

        /** BAL2 test */
        while(hal_gpio_test_bal2) {
            HAL_GPIO_set_pin(
                HAL_BAL_BANK2_PORT,
                HAL_BAL_BANK2_PIN,
                HAL_BAL_BANK2_ON
            );
            __asm("nop");
            HAL_GPIO_set_pin(
                HAL_BAL_BANK2_PORT,
                HAL_BAL_BANK2_PIN,
                HAL_BAL_BANK2_OFF
            );
            __asm("nop");
        }

        /** BAL3 test */
        while(hal_gpio_test_bal3) {
            HAL_GPIO_set_pin(
                HAL_BAL_BANK3_PORT,
                HAL_BAL_BANK3_PIN,
                HAL_BAL_BANK3_ON
            );
            __asm("nop");
            HAL_GPIO_set_pin(
                HAL_BAL_BANK3_PORT,
                HAL_BAL_BANK3_PIN,
                HAL_BAL_BANK3_OFF
            );
            __asm("nop");
        }

        /** BAL4 test */
        while(hal_gpio_test_bal4) {
            HAL_GPIO_set_pin(
                HAL_BAL_BANK4_PORT,
                HAL_BAL_BANK4_PIN,
                HAL_BAL_BANK4_ON
            );
            __asm("nop");
            HAL_GPIO_set_pin(
                HAL_BAL_BANK4_PORT,
                HAL_BAL_BANK4_PIN,
                HAL_BAL_BANK4_OFF
            );
            __asm("nop");
        }
    }
}

static void _IdleTask(void)
{
     /** Handle QSPY communication */
     QS_onIdle();
}


/* End of FILE */