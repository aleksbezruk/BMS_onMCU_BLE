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
// OSAL
#include "OSAL.h"
#endif // BMS_DISABLE_RTOS

// BMS Application
#include "MAIN.h"

#include "BLE_qn9080.h"

// Include for SYSCON register access
#include "fsl_device_registers.h"

#include "NVM_Interface.h"

// ===================
// HardFault Handler
// ===================
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

void HardFault_Handler_C(uint32_t *stack_frame) {
    volatile fault_stack_frame_t *frame = (fault_stack_frame_t *)stack_frame;
    volatile uint32_t fault_pc = frame->pc;
    volatile uint32_t fault_lr = frame->lr;
    volatile uint32_t fault_r0 = frame->r0;
    volatile uint32_t fault_r1 = frame->r1;
    volatile uint32_t fault_r2 = frame->r2;
    volatile uint32_t fault_r3 = frame->r3;
    volatile uint32_t cfsr = SCB->CFSR;  // Configurable Fault Status Register
    volatile uint32_t hfsr = SCB->HFSR;  // HardFault Status Register
    volatile uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address Register
    volatile uint32_t bfar = SCB->BFAR;   // BusFault Address Register
    
    // Check if this is the expected nvds_get fault
    bool is_nvds_fault = (fault_pc == 0x0302559d) || (fault_r3 == 0x0302559d);
    
    // Log fault information via QSPY
    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("HardFault: PC=");
        QS_U32(0, fault_pc);
        QS_STR(" LR=");
        QS_U32(0, fault_lr);
        QS_STR(" R0=");
        QS_U32(0, fault_r0);
        QS_STR(" R1=");
        QS_U32(0, fault_r1);
        QS_STR(" R2=");
        QS_U32(0, fault_r2);
        QS_STR(" R3=");
        QS_U32(0, fault_r3);
        QS_STR(" CFSR=");
        QS_U32(0, cfsr);
        QS_STR(" HFSR=");
        QS_U32(0, hfsr);
        QS_STR(" MMFAR=");
        QS_U32(0, mmfar);
        QS_STR(" BFAR=");
        QS_U32(0, bfar);
        if (is_nvds_fault) {
            QS_STR(" [NVDS_GET ROM ACCESS FAULT]");
        }
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
        for(volatile int j = 0; j < 500000; j++); // Slower blink in fault state
    }
}

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
static void Main_Test_Task(OSAL_arg_t argument);
static void Led_Test_Task(OSAL_arg_t argument);
static void Adc_Test_Task(OSAL_arg_t argument);

static void parseQueueItem_(Main_queue_data_t* queueItem);
static void handleSystemEvt_(Evt_sys_data_t* evt);
static void blinkTimerCallback_(OSAL_TimerArg_t arg);
static void led_blink_alive_(void);
#endif // BMS_DISABLE_RTOS

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
#define LED_TEST_TASK_PRIORITY   (OSAL_BLE_TASK_PRIORITY)
#define LED_TEST_TASK_INTERVAL   (1000U)    // in milliseconds

#define ADC_TEST_TASK_STACK_SIZE  (560U)
#define ADC_TEST_TASK_PRIORITY   (OSAL_ADC_TASK_PRIORITY)
#define ADC_TEST_TASK_INTERVAL   (10000U)    // in milliseconds

#define MAIN_TEST_TASK_STACK_SIZE  (2048U)  // Further increased for BLE initialization debugging
#define MAIN_TEST_TASK_PRIORITY   (OSAL_MAIN_TASK_PRIORITY)
#define MAIN_TEST_TASK_INTERVAL   (1000U)    // in milliseconds
#define MAIN_TASK_QUEUE_SIZE      (5U)

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

// OSAL data
#ifndef BMS_DISABLE_RTOS
// LED task handle
OSAL_TASK_DEFINE(LedTask);

// ADC task handle
OSAL_TASK_DEFINE(AdcTask);

// Main task and queue handles
OSAL_TASK_DEFINE(MainTask);
OSAL_QUEUE_DEFINE(mainTaskQueueHandle);

/**
 * @brief Timer for LED blinking
 */
OSAL_TIMER_DEFINE(blinkTimer);
static volatile uint8_t ledBlinkCntr_;

#endif // BMS_DISABLE_RTOS

// ========================
/// Tests code ...
// ========================
/**
 * @brief Main function for the test application.
 */
int main(void)
{
    /** Initialize HAL */
    HAL_status_t hwStatus = HAL_init_hardware();
    if (hwStatus != HAL_STATUS_OK) {
        HAL_ASSERT(0, __FILE__, __LINE__); // Hardware initialization failed
    }

    /** Initialize LED */
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
#ifndef BMS_DISABLE_RTOS
    /** Main task creation */
    OSAL_Status_t status = OSAL_SUCCESS;
    OSAL_TASK_CREATE(
        OSAL_TASK_GET_HANDLE(MainTask),
        Main_Test_Task,
        "Main_Test_Task",
        NULL,  // Stack pointer
        MAIN_TEST_TASK_STACK_SIZE,
        MAIN_TEST_TASK_PRIORITY,
        NULL, // Argument to pass to the task
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__); // Task creation failed
    }
    /** Create an event Queue for main task */
    OSAL_QUEUE_CREATE(
        mainTaskQueueHandle,
        "mainTaskQueue",
        MAIN_TASK_QUEUE_SIZE,
        sizeof(Main_queue_data_t),
        NULL, // Queue storage (not used in this implementation)
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__); // Queue creation failed
    }

    /** Start RTOS scheduler */
    vTaskStartScheduler();
    HAL_ASSERT(0, __FILE__, __LINE__); // Scheduler should never return
#else
    while(1) {
        _runTests_noRTOS();
    }
#endif // BMS_DISABLE_RTOS

    // If we reach here, it means the scheduler returned unexpectedly
    HAL_ASSERT(0, __FILE__, __LINE__); // Scheduler should never return
    // This is a fail-safe to prevent the application from running into undefined behavior
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Scheduler returned unexpectedly");
    QS_END()
    QS_FLUSH(); // Flush QSPY output

    // Return 0 to indicate normal termination (though this should never happen)
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
static void Led_Test_Task(OSAL_arg_t argument)
{
    (void)argument; // Unused parameter
    static uint8_t iter = 0;

    /** Start HW timer to count milliseconds */
    hal_time_init();

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
        } else if (iter == 4) {
            // Red LED on
            HAL_LED_red_On();
        } else if (iter == 5) {
            // Red LED off
            HAL_LED_red_Off();
        } else if (iter == 6) {
            // Red LED toggle
            HAL_LED_red_toggle();
        } else if (iter == 7) {
            // Red LED toggle
            HAL_LED_red_toggle();
        }

        iter++;
        if (iter == 8U) {
            // Reset iteration counter
            iter = 0;

            QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
                QS_STR("LED test is running");
            QS_END()
        }

        // OSA task delay
        OSAL_TASK_DELAY(LED_TEST_TASK_INTERVAL);
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
#else
static void Adc_Test_Task(OSAL_arg_t argument)
{
    (void)argument; // Unused parameter

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

    /** Init LED blink timer */
    OSAL_Status_t status = OSAL_SUCCESS;
    OSAL_TIMER_CREATE(
        OSAL_TIMER_GET_HANDLE(blinkTimer),
        OSAL_TIMER_TYPE_ONE_SHOT, // One-shot timer for LED blinking
        blinkTimerCallback_,
        0U, // arg
        status,
        100U // 100 ms interval for blinking
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__); // Timer creation failed
    }

    while (true) {
        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("ADC test is running");
        QS_END()

        led_blink_alive_();

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
        hal_time_delay(10u); // 10 ms delay

        /** Read ADC value, Bank2 */
        int32_t bank2_raw_mv = HAL_ADC_read(HAL_ADC_CHANNEL_1);
        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Bank2_raw_mV = ");
            QS_I32(0, bank2_raw_mv);
        QS_END()
        QS_FLUSH(); // Flush QSPY output
        bank2_pot_mv = ADC__TEST_CONV_BY_RATIO(bank2_raw_mv, ADC_BANK2_CONV_RATIO);
        bank2_mv = ADC_BANK_VOLT_CALC(bank1_pot_mv, bank2_pot_mv); // Calculate voltage in mV
        /** Compensate for offset */
        bank2_mv -= ADC_OFFSET;
        hal_time_delay(10u); // 10 ms delay

        /** Read ADC value, Bank3 */
        int32_t bank3_raw_mv = HAL_ADC_read(HAL_ADC_CHANNEL_2);
        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Bank3_raw_mV = ");
            QS_I32(0, bank3_raw_mv);
        QS_END()
        QS_FLUSH(); // Flush QSPY output
        bank3_pot_mv = ADC__TEST_CONV_BY_RATIO(bank3_raw_mv, ADC_BANK3_CONV_RATIO);
        bank3_mv = ADC_BANK_VOLT_CALC(bank2_pot_mv, bank3_pot_mv); // Calculate voltage in mV
        /** Compensate for offset */
        bank3_mv -= ADC_OFFSET;
        hal_time_delay(10u); // 10 ms delay

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

        static uint8_t vbat = 90U; // fake for battery voltage level
        vbat++;
        if (vbat > 100U) {
            vbat = 90U; // reset to 90% after reaching 100%
        }
        BLE_qn9080_status_t sendNorifStatus = BLE_UpdateBMSCharacteristics(vbat);
        if (sendNorifStatus != BLE_QN9080_STATUS_OK) {
            QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
                QS_STR("Failed to send battery status via BLE");
            QS_END()
            QS_FLUSH(); // Flush QSPY output
            HAL_ASSERT(0, __FILE__, __LINE__); // BLE update failed
        }

        OSAL_TASK_DELAY(ADC_TEST_TASK_INTERVAL);
    }
}

/**
 * @brief Blinks the alive LED
 * @details This function starts a timer that will blink the green LED
 * @param None
 * @retval None
 * 
 * @attention Should be non-blocking call
 *            to avoid starvation of other threads especiaaly BLE stack
 */
static void led_blink_alive_(void)
{
    ledBlinkCntr_ = 0;
    // start timer
    OSAL_Status_t status = OSAL_SUCCESS;
    OSAL_TIMER_START(
        OSAL_TIMER_GET_HANDLE(blinkTimer),
        100U, // 100 ms
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }
}

/**
 * @brief Main task function.
 * This function is called when the RTOS is enabled.
 * It runs the main task code in a loop.
 * 
 * @param argument Argument passed to the task (unused).
 */
static void Main_Test_Task(OSAL_arg_t argument)
{
    (void)argument; // Unused parameter

    OSAL_Status_t status = OSAL_SUCCESS;
    Main_queue_data_t queueItem;

    // Debug: Turn on red LED to indicate we're in the task
    HAL_LED_red_On();
    
    // Add stack canary to detect stack overflow
    volatile uint32_t stack_canary = 0xDEADBEEF;
    
    // Log current stack pointer
    uint32_t current_sp;
    __asm volatile ("mov %0, sp" : "=r" (current_sp));
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Main task start: SP=");
        QS_U32(0, current_sp);
        QS_STR(" Canary=");
        QS_U32(0, stack_canary);
    QS_END()
    QS_FLUSH();
    
    // Small delay to ensure QSPY message is sent
    for(volatile int i = 0; i < 100000; i++);
    
    // Check stack canary before BLE init
    if (stack_canary != 0xDEADBEEF) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Stack corruption detected before BLE_init!");
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__);
    }
    
    // Test ROM accessibility before BLE initialization
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Testing ROM accessibility...");
    QS_END()
    QS_FLUSH();
    
    // Check system control register for memory remapping
    volatile uint32_t sys_mode_ctrl = SYSCON->SYS_MODE_CTRL;
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("SYS_MODE_CTRL=");
        QS_U32(0, sys_mode_ctrl);
        QS_STR(" REMAP=");
        QS_U32(0, sys_mode_ctrl & 0x3);
    QS_END()
    QS_FLUSH();
    
    // Try to read from ROM entry point (should be accessible)
    volatile uint32_t *rom_entry = (volatile uint32_t *)0x03000738;
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Attempting to read ROM entry point at 0x03000738...");
    QS_END()
    QS_FLUSH();
    
    // Simple read test - if ROM is accessible, this should work
    volatile uint32_t rom_value = *rom_entry;
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("ROM entry value=");
        QS_U32(0, rom_value);
    QS_END()
    QS_FLUSH();
    
    // Try reading the specific nvds_get function address
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Testing nvds_get address 0x0302559d...");
    QS_END()
    QS_FLUSH();
    
    volatile uint32_t *nvds_get_addr = (volatile uint32_t *)0x0302559d;
    volatile uint32_t nvds_value = *nvds_get_addr;
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("nvds_get instruction=");
        QS_U32(0, nvds_value);
    QS_END()
    QS_FLUSH();
    
    // Initialize BLE stack in task context with large stack
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Calling BLE_init...");
    QS_END()
    QS_FLUSH();

    BLE_init();
    
    // Check stack canary after BLE init
    if (stack_canary != 0xDEADBEEF) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Stack corruption detected after BLE_init!");
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__);
    }
    
    // Debug: Turn off red LED to indicate BLE init completed
    HAL_LED_red_Off();
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_init completed successfully - waiting for initialization complete event...");
    QS_END()
    QS_FLUSH();

    /** LED task creation */
    status = OSAL_SUCCESS;
    OSAL_TASK_CREATE(
        OSAL_TASK_GET_HANDLE(LedTask),
            Led_Test_Task, 
            "Led_Test_Task",
            NULL,  // Stack pointer
            LED_TEST_TASK_STACK_SIZE,
            LED_TEST_TASK_PRIORITY, 
            NULL, // Argument to pass to the task
            status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__); // Task creation failed
    }

    /** ADC task creation */
    status = OSAL_SUCCESS;
    // ADC task creation
    OSAL_TASK_CREATE(
        OSAL_TASK_GET_HANDLE(AdcTask),
        Adc_Test_Task,
        "Adc_Test_Task",
        NULL,  // Stack pointer
        ADC_TEST_TASK_STACK_SIZE,
        ADC_TEST_TASK_PRIORITY,
        NULL, // Argument to pass to the task
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__); // Task creation failed
    }

    while (true) {
        /** Wait for event */
        OSAL_QUEUE_GET(
            OSAL_QUEUE_GET_HANDLE(mainTaskQueueHandle),
            &queueItem,
            OSAL_QUEUE_TIMEOUT_NEVER, // wait forever
            status
        );
        if (status != OSAL_SUCCESS) {
            HAL_ASSERT(0, __FILE__, __LINE__);
        }
        
            parseQueueItem_(&queueItem);
    }
}

/**
 * @brief Callback for the LED blink timer
 * @details This function is called by the OSAL timer when it expires.
 *          It toggles the green LED state to create a blink effect.
 *
 * @param[in] arg Timer argument (unused)
 * @retval None
 * @note 3 "on-off" blinks
 */
static void blinkTimerCallback_(OSAL_TimerArg_t arg)
{
    (void) arg; // unused argument

    // Blink green LED
    // 3 blinks, 100 ms each
    // 100 ms on, 100 ms off
    if (ledBlinkCntr_%2 == 0) {
        HAL_LED_green_on();
    } else {
        HAL_LED_green_off();
    }

    ledBlinkCntr_++;
    if (ledBlinkCntr_ < 6U) {
        // re-start timer to continue blink
        OSAL_Status_t status = OSAL_SUCCESS;
        OSAL_TIMER_START(
            OSAL_TIMER_GET_HANDLE(blinkTimer),
            100U, // 100 ms
            status
        );
        if (status != OSAL_SUCCESS) {
            HAL_ASSERT(0, __FILE__, __LINE__);
        }
    }
}

/**
 * @brief Parses a queue item.
 * 
 * @param[in] queueItem Queue item to parse
 * 
 * @retval None
 */
static void parseQueueItem_(Main_queue_data_t* queueItem)
{
    switch (queueItem->evtType)
    {
        case EVT_SYSTEM:
        {
            handleSystemEvt_(&queueItem->evtData.sysEvtData);
            break;
        }

        default:
        {
            QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
                QS_STR("Unknown event type received in Main task: ");
                QS_U8(0, queueItem->evtType);
            QS_END()
            QS_FLUSH(); // Flush QSPY output
        }
    }
}

/**
 * @brief Handles an System event
 * 
 * @param[in] evt Pointer to incoming event
 * 
 * @retval None
 */
static void handleSystemEvt_(Evt_sys_data_t* evt)
{
    uint8_t swState_ = evt->swStates;

    QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
        QS_STR("Sys evt, set switches state: ");
        QS_U8(0, swState_);
    QS_END()
}

/** 
 * @brief Posts an event into Main task event queue
 * 
 * @param[in] evt Event to post
 * 
 * @param[in] eventType Event type to post
 * 
 * @retval None
 * 
 */
void MAIN_post_evt(Main_evt_t* evt, Evt_types_t eventType)
{
    HAL_ASSERT((evt != NULL) && (eventType < EVT_TYPE_MAX), __FILE__, __LINE__);

    OSAL_Status_t status = OSAL_SUCCESS;
    Main_queue_data_t queueItem;

    queueItem.evtType = eventType;
    memcpy((uint8_t*) &queueItem.evtData, (uint8_t*) evt, sizeof(Main_evt_t));

    OSAL_QUEUE_PUT(
        OSAL_QUEUE_GET_HANDLE(mainTaskQueueHandle),
        &queueItem,
        OSAL_QUEUE_TIMEOUT_NEVER,
        status
    );
    if (status != OSAL_SUCCESS) {
        HAL_ASSERT(0, __FILE__, __LINE__);
    }
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
    NvIdle();

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

#if 0
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
#endif // 0

#endif // BMS_DISABLE_RTOS

/* End of FILE */