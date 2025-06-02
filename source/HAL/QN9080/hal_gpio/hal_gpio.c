/**
 * @file  hal_gpio.c
 *
 * @brief Implementation of GPIO functions.
 *
 * @note QN9080
 * 
 * @version 0.5.0
 */

#include "hal_gpio.h"
#include "hal.h"

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
 * @brief Initialize a pin with settings listed below. 
 * 
 * @param[in]   port    pointer to pin's Port dat structure
 * 
 * @param[in]   pin     pin to init
 * 
 * @param[in]   func    pim function
 * 
 * @param[in]   pullRes pull resistor config
 * 
 * @param[in]   driveMode   output pin current drive setting
 * 
 * @param[in]   initialState    initial state LOW or HIGH
 * 
 * @retval None
 * 
 */
void HAL_GPIO_init_pin(
    Hal_gpio_port_t* port, 
    Hal_gpio_pin_t pin,
    Hal_gpio_function_t func,
    Hal_gpio_pullRes_t pullRes,
    Hal_gpio_driveMode_t driveMode,
    Hal_gpio_pin_state_t initialState
)
{
    HAL_ASSERT(port != NULL);
    HAL_ASSERT(func <= HAL_GPIO_UART_TX);
    HAL_ASSERT(pullRes <= HAL_GPIO_PULL_DOWN);
    HAL_ASSERT(driveMode <= HAL_GPIO_DRIVE_HIGH);
    HAL_ASSERT(initialState <= HAL_GPIO_HIGH_LEVEL);

}

/**
 * @brief De-init a pin. 
 * 
 * @param[in]   port    pointer to pin's Port dat structure
 * 
 * @param[in]   pin     pin to init
 * 
 * @details Set pin to High-Z mode but doesn't disable clock.
 * 
 * @retval None
 * 
 */
void HAL_GPIO_deinit_pin(Hal_gpio_port_t* port, Hal_gpio_pin_t pin)
{
    HAL_ASSERT(port != NULL);

}

/**
 * @brief Read a digital pin state.
 * 
 * @param[in]   port    pointer to pin's Port dat structure
 * 
 * @param[in]   pin     pin to init
 * 
 * @retval HAL_GPIO_LOW_LEVEL or HAL_GPIO_HIGH_LEVEL
 * 
 */
Hal_gpio_pin_state_t HAL_GPIO_read_pin(Hal_gpio_port_t* port, Hal_gpio_pin_t pin)
{
    HAL_ASSERT(port != NULL);

}

/**
 * @brief Set a digital pin state. 
 * 
 * @param[in]   port    pointer to pin's Port dat structure
 * 
 * @param[in]   pin     pin to init
 * 
 * @param[in]   level   LOW or HIGH
 * 
 * @retval HAL_GPIO_LOW_LEVEL or HAL_GPIO_HIGH_LEVEL
 * 
 */
void HAL_GPIO_set_pin(Hal_gpio_port_t* port, Hal_gpio_pin_t pin, Hal_gpio_pin_state_t level)
{
    HAL_ASSERT(port != NULL);
    HAL_ASSERT(level <= HAL_GPIO_HIGH_LEVEL);

}

///////////////////////
// Auxiliary functions
///////////////////////
/**
 * @brief Determine input buffer configuration based on pin function (DIGITAL / ANALOG).
 * 
 * @param[in]   func    pim function
 * 
 * @param[in]   pullRes pull resistor config
 * 
 * @retval Input buffer config. \ref CY_GPIO_DM_ANALOG, CY_GPIO_DM_PULLUP etc.
 * 
 */
static uint32_t _get_inputBuf_config(Hal_gpio_function_t func, Hal_gpio_pullRes_t pullRes)
{
    uint32_t bufConfig;

    switch (func)
    {
        case HAL_GPIO_ANALOG_INPUT:
        {
            bufConfig = CY_GPIO_DM_ANALOG;
            break;
        }

        default:
        {
            // Not analog (digital) input
            if (pullRes == HAL_GPIO_PULL_DISABLED) {
                bufConfig = CY_GPIO_DM_HIGHZ;
            } else if (pullRes == HAL_GPIO_PULL_UP) {
                bufConfig = CY_GPIO_DM_PULLUP;
            } else if (pullRes == HAL_GPIO_PULL_DOWN) {
                bufConfig = CY_GPIO_DM_PULLDOWN;
            } else {
                // smth. went wrong ...
                HAL_ASSERT(false);
            }
            break;
        }
    }

    return bufConfig;
}

/**
 * @brief Determine Pin Function configuration (HSIOM) based on pin function.
 * 
 * @param[in]   func    pim function
 *
 * @retval ???
 * 
 */
static en_hsiom_sel_t _get_pinFunc_config(Hal_gpio_function_t func)
{
    en_hsiom_sel_t hsiom_config;

    switch (func)
    {
        case HAL_GPIO_DIGITAL_OUTPUT:
        case HAL_GPIO_DIGITAL_INPUT:
        case HAL_GPIO_ANALOG_INPUT:
        {
            hsiom_config = HSIOM_SEL_GPIO;
            break;
        }

        case HAL_GPIO_UART_RX:
        {
            hsiom_config = P5_0_SCB5_UART_RX;
            break;
        }

        case HAL_GPIO_UART_TX:
        {
            hsiom_config = P5_1_SCB5_UART_TX;
            break;
        }

        default:
        {
            HAL_ASSERT(false);
            break;
        }
    }

    return hsiom_config;
}

/* [] END OF FILE */