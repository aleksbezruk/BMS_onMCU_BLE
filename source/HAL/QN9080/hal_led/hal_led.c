/**
 * @file  hal_led.c
 *
 * @brief Implementation of LED functions.
 *
 * @note QN9080
 * 
 * @version 0.5.0
 */

#include "hal_led.h"
#include "LED.h" 

///////////////////////
// Defines
///////////////////////

///////////////////////
// Functions prototype
///////////////////////

///////////////////////
// Private data
///////////////////////

///////////////////////
// Code
///////////////////////

/**
 * @brief Generic API to init green LED
 * 
 * @param   None
 * 
 * @details Off LED, appropriate pin configuration done in \ref _initLed()
 * 
 * @retval  Nonw
 * 
 */
void HAL_LED_init_green(void)
{
    Led2Off();
}

/**
 * @brief Generic API to init red LED
 * 
 * @param   None
 * 
 * @details Off LED, appropriate pin configuration done in \ref _initLed()
 * 
 * @retval  Nonw
 * 
 */
void HAL_LED_init_red(void)
{
    Led1Off();
}

/**
 * @brief Generic API to toggle red LED
 * 
 * @param   None
 * 
 * @retval  Nonw
 * 
 */
void HAL_LED_red_toggle(void)
{
    Led1Toggle();
}

/**
 * @brief Generic API to turn red LED on
 * 
 * @param   None
 * 
 * @retval  Nonw
 * 
 */
void HAL_LED_red_On(void)
{
    Led1On();
} 

/**
 * @brief Generic API to turn red LED off
 * 
 * @param   None
 * 
 * @retval  Nonw
 * 
 */
void HAL_LED_red_Off(void)
{
    Led1Off();
}

/**
 * @brief Generic API to toggle green LED
 * 
 * @param   None
 * 
 * @retval  Nonw
 * 
 */
void HAL_LED_green_toggle(void)
{
    Led2Toggle();
}

/**
 * @brief Generic API to turm green LED on
 * 
 * @param   None
 * 
 * @retval  Nonw
 * 
 */
void HAL_LED_green_on(void)
{
    Led2On();
}

/**
 * @brief Generic API to turm green LED off
 * 
 * @param   None
 * 
 * @retval  Nonw
 * 
 */
void HAL_LED_green_off(void)
{
    Led2Off();
}


/* [] END OF FILE */