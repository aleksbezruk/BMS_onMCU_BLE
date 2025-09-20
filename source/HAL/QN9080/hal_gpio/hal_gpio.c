/**
 * @file  hal_gpio.c
 *
 * @brief Implementation of GPIO functions.
 *
 * @note QN9080
 *
 * @version 0.6.0
 */

#include "hal_gpio.h"
#include "hal.h"

// ===================
// Defines
// ===================

// ===================
// Functions prototype
// ===================
static uint32_t _get_inputBuf_config(Hal_gpio_function_t func, Hal_gpio_pullRes_t pullRes);
static uint32_t _get_pinFunc_config(Hal_gpio_function_t func);
static uint8_t _get_portNum(Hal_gpio_port_t* port);

// ===================
// Private data
// ===================
const gpio_pin_config_t gpio_deinitPin_config = {
    .pinDirection = kGPIO_DigitalInput,
    .outputLogic = 0u // N/A for input
};

// ===================
// Code
// ===================
/**
 * @brief Initialize a pin with settings listed below. 
 * 
 * @param[in]   port    pointer to pin's Port data structure
 * 
 * @param[in]   pin     pin to init
 * 
 * @param[in]   func    pin function
 * 
 * @param[in]   pullRes pull resistor config
 * 
 * @param[in]   driveMode   output pin current drive setting
 * 
 * @param[in]   initialState    initial state LOW or HIGH
 * 
 * @note    Almost all pins (except some pins on Port B)
 *          have default GPIO function -> IOCON_FUNC0 = 0
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
    HAL_ASSERT((port != NULL), __FILE__, __LINE__);
    HAL_ASSERT((func <= HAL_GPIO_UART_TX), __FILE__, __LINE__);
    HAL_ASSERT((pullRes <= HAL_GPIO_PULL_DOWN), __FILE__, __LINE__);
    HAL_ASSERT((driveMode <= HAL_GPIO_DRIVE_HIGH), __FILE__, __LINE__);
    HAL_ASSERT((initialState <= HAL_GPIO_HIGH_LEVEL), __FILE__, __LINE__);

    gpio_pin_config_t gpioa_digitOut_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = initialState
    };
    gpio_pin_config_t gpioa_digitInput_config = {
        .pinDirection = kGPIO_DigitalInput,
        .outputLogic = 0u // N/A for input
    };
    uint32_t func_config = IOCON_FUNC0;    // by Deafult GPIO func
    uint32_t driveStrength = IOCON_DRIVE_LOW;
    uint32_t bufMode = IOCON_MODE_HIGHZ;

    /** Configure pin's output/input buffer */
    if (driveMode == HAL_GPIO_DRIVE_HIGH) {
        // 'Strong Drive' output mode, without Pull resistors
        if (func == HAL_GPIO_DIGITAL_OUTPUT) {
            GPIO_PinInit(port, pin, &gpioa_digitOut_config);
        }
        driveStrength = IOCON_DRIVE_HIGH;
    } else if (driveMode == HAL_GPIO_DRIVE_NORMAL){
        // Config Pull-Up/Down for Output
        HAL_ASSERT((pullRes != HAL_GPIO_PULL_DISABLED), __FILE__, __LINE__);
        GPIO_PinInit(port, pin, &gpioa_digitOut_config);
        bufMode = (pullRes == HAL_GPIO_PULL_UP)? IOCON_MODE_PULLUP : IOCON_MODE_PULLDOWN;
    } else {
        // No output drive -> Input
        if (func == HAL_GPIO_DIGITAL_INPUT) {
            GPIO_PinInit(port, pin, &gpioa_digitInput_config);  // n/a for pins with Alterante funcs (UART, SPI, etc.)
        }
        bufMode = _get_inputBuf_config(func, pullRes);
    }

    /** Get pin funcs  */
    func_config = _get_pinFunc_config(func);

    /** Configure pin function (multiplexer) & other prop. */
    const uint32_t pin_config = (/* Selects pin function */
                                func_config |
                                /* Selects Buffer Mode */
                                bufMode |
                                /* Enable buffer strength */
                                driveStrength);
    /* PORTA PIN 3 (coords: 8) is configured as FC2_SDA_SSEL0 */
    uint8_t port_num = _get_portNum(port);
    IOCON_PinMuxSet(IOCON, port_num, pin, pin_config);
}

/**
 * @brief De-init a pin. 
 * 
 * @param[in]   port    pointer to pin's Port dat structure
 * 
 * @param[in]   pin     pin to de-init
 * 
 * @details Set pin to High-Z mode but doesn't disable clock.
 * 
 * @retval None
 * 
 */
void HAL_GPIO_deinit_pin(Hal_gpio_port_t* port, Hal_gpio_pin_t pin)
{
    HAL_ASSERT((port != NULL), __FILE__, __LINE__);

    /** Configure pin as Input to minimize current consumption */
    GPIO_PinInit(port, pin, &gpio_deinitPin_config);

    /** Configure pin function (multiplexer) & other prop. */
    const uint32_t pin_config = (/* Selects pin function */
                                IOCON_FUNC0 |
                                /* Selects Buffer Mode */
                                IOCON_MODE_PULLDOWN |
                                /* Enable buffer strength */
                                IOCON_DRIVE_LOW);
    /* PORTA PIN 3 (coords: 8) is configured as FC2_SDA_SSEL0 */
    uint8_t port_num = _get_portNum(port);
    IOCON_PinMuxSet(IOCON, port_num, pin, pin_config);
}

/**
 * @brief Read a digital pin state.
 * 
 * @param[in]   port    pointer to pin's Port dat structure
 * 
 * @param[in]   pin     pin to read
 * 
 * @retval HAL_GPIO_LOW_LEVEL or HAL_GPIO_HIGH_LEVEL
 * 
 */
Hal_gpio_pin_state_t HAL_GPIO_read_pin(Hal_gpio_port_t* port, Hal_gpio_pin_t pin)
{
    HAL_ASSERT((port != NULL), __FILE__, __LINE__);

    Hal_gpio_pin_state_t state = (Hal_gpio_pin_state_t) GPIO_ReadPinInput(port, pin);

    return state;
}

/**
 * @brief Set a digital pin state. 
 * 
 * @param[in]   port    pointer to pin's Port dat structure
 * 
 * @param[in]   pin     pin to set
 * 
 * @param[in]   level   LOW or HIGH
 * 
 * @retval HAL_GPIO_LOW_LEVEL or HAL_GPIO_HIGH_LEVEL
 * 
 */
void HAL_GPIO_set_pin(Hal_gpio_port_t* port, Hal_gpio_pin_t pin, Hal_gpio_pin_state_t level)
{
    HAL_ASSERT((port != NULL), __FILE__, __LINE__);
    HAL_ASSERT((level <= HAL_GPIO_HIGH_LEVEL), __FILE__, __LINE__);

    GPIO_WritePinOutput(port, pin, level);
}

// ===================
// Auxiliary functions
// ===================
/**
 * @brief Determine input buffer configuration based on pin function (DIGITAL / ANALOG).
 * 
 * @param[in]   func    pin function
 * 
 * @param[in]   pullRes pull resistor config
 * 
 * @retval Input buffer config.
 * 
 */
static uint32_t _get_inputBuf_config(Hal_gpio_function_t func, Hal_gpio_pullRes_t pullRes)
{
    uint32_t bufConfig;

    switch (func)
    {
        case HAL_GPIO_ANALOG_INPUT:
        {
            bufConfig = IOCON_MODE_HIGHZ;
            break;
        }

        default:
        {
            // Not analog (digital) input
            if (pullRes == HAL_GPIO_PULL_DISABLED) {
                bufConfig = IOCON_MODE_HIGHZ;
            } else if (pullRes == HAL_GPIO_PULL_UP) {
                bufConfig = IOCON_MODE_PULLUP;
            } else if (pullRes == HAL_GPIO_PULL_DOWN) {
                bufConfig = IOCON_MODE_PULLDOWN;
            } else {
                // smth. went wrong ...
                HAL_ASSERT(0, __FILE__, __LINE__);
            }
            break;
        }
    }

    return bufConfig;
}

/**
 * @brief Determine Pin Function configuration (Multiplexer config) based on pin function.
 * 
 * @param[in]   func    pin function
 *
 * @retval pin alternate function. \ref IOCON_FUNC_i, i = 0...7
 * 
 */
static uint32_t _get_pinFunc_config(Hal_gpio_function_t func)
{
    uint32_t func_config;

    switch (func)
    {
        case HAL_GPIO_DIGITAL_OUTPUT:
        case HAL_GPIO_DIGITAL_INPUT:
        {
            func_config = IOCON_FUNC0;
            break;
        }

        case HAL_GPIO_ANALOG_INPUT:
        {
            func_config = IOCON_FUNC1;
            break;
        }

        case HAL_GPIO_UART_RX:
        {
            func_config = IOCON_FUNC4;
            break;
        }

        case HAL_GPIO_UART_TX:
        {
            func_config = IOCON_FUNC4;
            break;
        }

        default:
        {
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
        }
    }

    return func_config;
}

/**
 * @brief Gets port number.
 *
 * @param[in]   port    pin's Port
 *
 * @retval port number: 0 -> PortA, 1 -> PortB
 *
 */
static uint8_t _get_portNum(Hal_gpio_port_t* port)
{
    uint8_t portNum = 0xFFu; // Default invalid value

    if (port == GPIOA) {
        portNum = 0u;
    } else if (port == GPIOB) {
        portNum = 1u;
    } else {
        HAL_ASSERT(0, __FILE__, __LINE__);  // smth. went wrong
        // Optionally handle error, e.g., return default or error value
    }

    return portNum;
}

/* [] END OF FILE */