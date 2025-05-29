/**
 * @file  hal.c
 *
 * @brief Implementation of LED functions.
 *
 * @note PSOC63
 * 
 * @version 0.5.0
 */
#include "cy_pdl.h"
#include "BSP.h"
#include "hal_led.h"

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
 * @retval  Nonw
 * 
 */
void HAL_LED_init_green(void)
{
    BSP_init_led_green(); 
}

/**
 * @brief Generic API to init red LED
 * 
 * @param   None
 * 
 * @retval  Nonw
 * 
 */
void HAL_LED_init_red(void)
{
    BSP_init_led_red();
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
    BSP_led_red_toggle();
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
    BSP_led_red_On();
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
    BSP_led_red_Off();
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
    BSP_led_green_toggle();
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
    BSP_led_green_on();
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
    BSP_led_green_off();
}

/* [] END OF FILE */