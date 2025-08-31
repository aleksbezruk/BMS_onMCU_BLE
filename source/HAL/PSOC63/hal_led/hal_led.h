/**
 * @file  hal_led.h
 *
 * @brief Definition of LED functions
 *
 * @note PSOC63
 * 
 * @version 0.5.0
 */

#ifndef HAL_LED_MODULE
#define HAL_LED_MODULE

// ===================
// Defines
// ===================

// ===================
// API
// ===================
void HAL_LED_init_green(void);
void HAL_LED_init_red(void);
void HAL_LED_red_toggle(void);
void HAL_LED_red_On(void); 
void HAL_LED_red_Off(void);
void HAL_LED_green_toggle(void);
void HAL_LED_green_on(void);
void HAL_LED_green_off(void);

#endif  //HAL_LED_MODULE

/* [] END OF FILE */
