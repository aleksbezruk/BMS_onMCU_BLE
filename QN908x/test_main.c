#include <stdbool.h>
#include <stdint.h>
// HAL
#include "hal.h"
#include "hal_led.h"

volatile bool test_green_led;
volatile bool test_red_led;
volatile uint8_t led_test;

// LED tests
#define LED_NUM_TESTS   3U
#define SMALL_DELAY 1000000u
#define BIG_DELAY 2*SMALL_DELAY
#define GREEN_ON_INTERVAL SMALL_DELAY
#define GREEN_OFF_INTERVAL 2*SMALL_DELAY
#define GREEN_TOGGLE_INTERVAL 3*SMALL_DELAY
#define RED_ON_INTERVAL SMALL_DELAY
#define RED_OFF_INTERVAL 2*SMALL_DELAY
#define RED_TOGGLE_INTERVAL 3*SMALL_DELAY

int main(void)
{
    unsigned int i = 0;

    HAL_status_t hwStatus = HAL_init_hardware();
    if (hwStatus != HAL_STATUS_OK) {
        HAL_ASSERT(0);
    }

    HAL_LED_init_green();
    HAL_LED_init_red();

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
                }
            }
        }

        /** Next tests */

    }
    return 0;
}


/* End of FILE */