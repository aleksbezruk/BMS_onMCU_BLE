#include <stdbool.h>
#include <stdint.h>

// FSL
#include "fsl_common.h"

// HAL
#include "hal.h"
#include "hal_led.h"
#include "hal_gpio.h"
#include "hal_adc.h"
#include "hal_time.h"

// QSPY
#include "qspyHelper.h"
#include "qs.h"

#ifndef BMS_DISABLE_RTOS
// OSA
#include "fsl_os_abstraction.h"
#include "fsl_os_abstraction_free_rtos.h"

// FreeRTOS
#include "FreeRTOS.h"
#endif // BMS_DISABLE_RTOS

// BMS Application
#include "MAIN.h"

// ===================
// Defines
// ===================
// LED tests
#define TEST_LED_NUM_ITERS   3U
#define GREEN_ON_DURATION    1000U // 1 second
#define GREEN_OFF_DURATION   1000U // 1 second
#define RED_ON_DURATION      1000U // 1 second
#define RED_OFF_DURATION     1000U // 1 second

// ========================
// Function Prototypes
// ========================
#ifdef BMS_DISABLE_RTOS
static void TEST_led(void);
static void TEST_gpio(void);
static void TEST_adc(void);
static void _IdleTask(void);
static void _runTests_noRTOS(void);
#else
static void Led_Test_Task(osaTaskParam_t argument);
#endif // BMS_DISABLE_RTOS

// UART tests
// TBD

// ADC tests
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

#define NOP() __asm("nop")

// OSA
#ifndef BMS_DISABLE_RTOS
#define LED_TEST_TASK_STACK_SIZE  (560U)
#define LED_TEST_TASK_PRIORITY   (configMAX_PRIORITIES - 5U)
#endif // BMS_DISABLE_RTOS

// ===================
// Private data
// ===================
// Test completion flag
static volatile bool isTestsCompleted = false;
// QSPY packet transmission flag
static volatile bool isTxPacket = true;
// LED test iteration counter
static volatile uint8_t led_test_num;

// GPIO tests
/** GPIO test enable flag */
static volatile bool hal_gpio_test_en;
/** GPIO test discharge flag */
static volatile bool hal_gpio_test_disch_en;
/** GPIO test charge flag */
static volatile bool hal_gpio_test_chrg_en;
/** GPIO test balancer flag 1 */
static volatile bool hal_gpio_test_bal1;
/** GPIO test balancer flag 2 */
static volatile bool hal_gpio_test_bal2;
/** GPIO test balancer flag 3 */
static volatile bool hal_gpio_test_bal3;
/** GPIO test balancer flag 4 */
static volatile bool hal_gpio_test_bal4;

// ADC tests
/** ADC test enable flag */
static volatile bool hal_adc_test_en = true;
/** ADC test number of iterations */
static volatile uint8_t adc_test_num;

// OSA data
#ifndef BMS_DISABLE_RTOS
static OSA_TASK_DEFINE(Led_Test_Task, LED_TEST_TASK_PRIORITY, 1, LED_TEST_TASK_STACK_SIZE, FALSE );
static osaTaskId_t ledTaskId = NULL;
#endif // BMS_DISABLE_RTOS

// ========================
/// Tests code ...
// ========================
/**
 * @brief Main function for the test application.
 */
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
#ifdef BMS_DISABLE_RTOS
    QS_FUN_DICTIONARY(&_IdleTask);
#endif // BMS_DISABLE_RTOS

#ifdef BMS_DISABLE_RTOS
    /** Start HW timer to count milliseconds */
    hal_time_init();
#endif // BMS_DISABLE_RTOS

    /** Run tests */
    while(1) {
#ifdef BMS_DISABLE_RTOS
        _runTests_noRTOS();
#else
        /** LED task creation */
        ledTaskId = OSA_TaskCreate(OSA_TASK(Led_Test_Task), NULL);
        if( NULL == ledTaskId ) {
            HAL_ASSERT(0); // Task creation failed
        }
        /** Start RTOS scheduler */
        vTaskStartScheduler();
#endif  // BMS_DISABLE_RTOS
    }
    return 0;
}

#ifdef BMS_DISABLE_RTOS
/**
 * @brief Run tests in a loop without RTOS.
 * This function is called when the RTOS is disabled.
 * It runs the LED, GPIO, and ADC tests sequentially.
 * After all tests are completed, it sends a message to QSPY indicating that the tests are completed.
 * If the tests are already completed, it enters an idle task loop.
 *
 * @param None
 * 
 * @return None
 */
static void _runTests_noRTOS(void)
{
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
#endif // BMS_DISABLE_RTOS

// ==========================
/// Tests implementation
// ==========================
#ifdef BMS_DISABLE_RTOS
/**
 * @brief Test LED functionality.
 * 
 * This function toggles the green and red LEDs in a specific pattern.
 * It will turn on the green LED, then turn it off, toggle it, and then
 * switch to the red LED, repeating the process until all tests are completed.
 */
static void TEST_led(void)
{
    while (led_test_num < TEST_LED_NUM_ITERS) {
        // Green
        HAL_LED_green_on();
        hal_time_delay(GREEN_ON_DURATION);
        HAL_LED_green_off();
        hal_time_delay(GREEN_OFF_DURATION);
        HAL_LED_green_toggle();
        hal_time_delay(GREEN_ON_DURATION);
        HAL_LED_green_toggle();
        hal_time_delay(GREEN_OFF_DURATION);

        // Red
        HAL_LED_red_On();
        hal_time_delay(RED_ON_DURATION);
        HAL_LED_red_Off();
        hal_time_delay(RED_OFF_DURATION);
        HAL_LED_red_toggle();
        hal_time_delay(RED_ON_DURATION);
        HAL_LED_red_toggle();
        hal_time_delay(RED_OFF_DURATION);
        led_test_num++;
    }

    // All tests completed, turn off both LEDs
    HAL_LED_green_off();
    HAL_LED_red_Off();

    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("LED tests completed successfully");
    QS_END()

    QS_FLUSH(); // Flush QSPY output
}
#else
static void Led_Test_Task(osaTaskParam_t argument)
{
    (void)argument; // Unused parameter
    static uint8_t iter = 0;

    while (true) {
        if (iter == 0) {
            // Green LED on
            HAL_LED_green_on();
        } else if (iter == 1) {
            // Green LED off
            HAL_LED_green_off();
        } else if (iter == 2) {
            // Green LED toggle
            HAL_LED_green_toggle();
        } else if (iter == 3) {
            // Green LED toggle
            HAL_LED_green_toggle();
        }

        iter++;
        if (iter == 4U) {
            // Reset iteration counter
            iter = 0;
        }

        // OSA task delay
        OSA_TimeDelay(1000U); // 1 second delay
    }
}
#endif // BMS_DISABLE_RTOS

#ifdef BMS_DISABLE_RTOS
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
#endif // BMS_DISABLE_RTOS

#ifdef BMS_DISABLE_RTOS
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
        hal_time_delay(1000u); // 1 second delay

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
        hal_time_delay(1000u); // 1 second delay

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
        hal_time_delay(1000u); // 1 second delay

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
        hal_time_delay(10000u); // 10 seconds delay
    }

    /** Deinitialize ADC */
    HAL_ADC_deinit();
    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("ADC tests completed successfully");
    QS_END()
    QS_FLUSH(); // Flush QSPY output
}
#endif // BMS_DISABLE_RTOS

#if defined(BMS_DISABLE_RTOS)
/**
 * @brief Idle task function.
 * This function is called when the system is idle.
 * It handles QSPY communication by calling the QS_onIdle function.
 * This function is only defined when the RTOS is disabled.
 */
static void _IdleTask(void)
{
     /** Handle QSPY communication */
     QS_onIdle();
}
#else
/**
 * @brief Idle task function.
 * This function is called when the system is idle.
 * It handles QSPY communication by calling the QS_onIdle function.
 * This function is only defined when the RTOS is enabled.
 */
void vApplicationIdleHook(void)
{
    /** Handle QSPY communication */
    QS_onIdle();
}

/**
 * @brief Stack overflow hook function.
 * This function is called when a stack overflow is detected in a task.
 * It logs the task name and ID to QSPY.
 * 
 * @param xTask Handle of the task that caused the stack overflow.
 * @param pcTaskName Name of the task that caused the stack overflow.
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    /** Handle stack overflow */
    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("Stack overflow detected in task: ");
        QS_STR(pcTaskName);
    QS_END()
    QS_FLUSH(); // Flush QSPY output
}

// Stub, test Discharge switch only for QN9080DK
void MAIN_post_evt(Main_evt_t* evt, Evt_types_t eventType)
{
    (void) eventType;
    static bool dischSwInit = false;

    if (evt->sysEvtData.swStates == 0x01) {
        if (!dischSwInit) {
            dischSwInit = true;
            HAL_GPIO_init_pin(
                HAL_DISCHARGE_PORT,
                HAL_DISCHARGE_PIN,
                HAL_GPIO_DIGITAL_OUTPUT,
                HAL_GPIO_PULL_DISABLED,
                HAL_GPIO_DRIVE_HIGH,
                HAL_DISCHARGE_OFF
            );
        }
        // Set discharge switch ON
        HAL_GPIO_set_pin(
            HAL_DISCHARGE_PORT,
            HAL_DISCHARGE_PIN,
            HAL_DISCHARGE_ON
        );
    } else {
        // Set discharge switch OFF
        HAL_GPIO_set_pin(
            HAL_DISCHARGE_PORT,
            HAL_DISCHARGE_PIN,
            HAL_DISCHARGE_OFF
        );
    }
}

#endif // BMS_DISABLE_RTOS

/* End of FILE */