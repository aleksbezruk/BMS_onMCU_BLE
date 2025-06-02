/**
 * @file  hal_gpio.h
 *
 * @brief Definition of GPIO functions (the list may be extended if needed):
 *          1. Init pin: 
 *              - Pin type config:  Diigtal Input/Output; Analog input; Alternative Func (UART, SPI etc.);
 *              - Pull-Up/Down resistor connection;
 *              - Otput buffer strenrth.
 *          2. Read Digital Input state
 *          3. Set Digital Output state
 *
 * @note PSOC63
 * 
 * @version 0.5.0
 */

#ifndef HAL_GPIO_MODULE
#define HAL_GPIO_MODULE

///////////////////////
// Defines
///////////////////////
/*! BMS discharge switch state */
typedef enum {
    HAL_BMS_DISCHARGE_OFF,
    HAL_BMS_DISCHARGE_ON
} HAL_dischargeSw_state_t;

/*! BMS charge switch state */
typedef enum {
    HAL_BMS_CHARGE_OFF,
    HAL_BMS_CHARGE_ON
} HAL_chargeSw_state_t;

/*! BMS bank balancer state */
typedef enum {
    HAL_BMS_BALANCER_OFF,
    HAL_BMS_BALANCER_ON
} HAL_balancer_state_t;

/*! BMS bank number */
typedef enum {
    HAL_BMS_BANK1,
    HAL_BMS_BANK2,
    HAL_BMS_BANK3,
    HAL_BMS_BANK4
} HAL_bank_t;
#define HAL_BMS_BANK1_MASK  (1 << HAL_BMS_BANK1)
#define HAL_BMS_BANK2_MASK  (1 << HAL_BMS_BANK2)
#define HAL_BMS_BANK3_MASK  (1 << HAL_BMS_BANK3)
#define HAL_BMS_BANK4_MASK  (1 << HAL_BMS_BANK4)

#define HAL_BMS_ALL_BANKS  (HAL_BMS_BANK1_MASK | \
                            HAL_BMS_BANK2_MASK | \
                            HAL_BMS_BANK3_MASK | \
                            HAL_BMS_BANK4_MASK)

/*! Ports, Pins definition */
#define HAL_DISCHARGE_PORT GPIO_PRT9
#define HAL_DISCHARGE_PIN 5U
#define HAL_DISCHARGE_ON 0U
#define HAL_DISCHARGE_OFF 1U

#define HAL_CHARGE_PORT GPIO_PRT9
#define HAL_CHARGE_PIN 4U
#define HAL_CHARGE_ON 0U
#define HAL_CHARGE_OFF 1U

#define HAL_BAL_BANK1_PORT GPIO_PRT9
#define HAL_BAL_BANK1_PIN 3U
#define HAL_BAL_BANK1_ON 0U
#define HAL_BAL_BANK1_OFF 1U
#define HAL_BAL_BANK2_PORT GPIO_PRT9
#define HAL_BAL_BANK2_PIN 2U
#define HAL_BAL_BANK2_ON 0U
#define HAL_BAL_BANK2_OFF 1U
#define HAL_BAL_BANK3_PORT GPIO_PRT9
#define HAL_BAL_BANK3_PIN 1U
#define HAL_BAL_BANK3_ON 0U
#define HAL_BAL_BANK3_OFF 1U
#define HAL_BAL_BANK4_PORT GPIO_PRT9
#define HAL_BAL_BANK4_PIN 0U
#define HAL_BAL_BANK4_ON 0U
#define HAL_BAL_BANK4_OFF 1U

/*! HAL GPIO port data structure definition */
typedef GPIO_PRT_Type Hal_gpio_port_t;

/*! HAL GPIO pin number definition, e.g. 0, 1, 2 ... */
typedef uint8_t Hal_gpio_pin_t;

/*! HAL GPIO pin function definition */
typedef enum {
    HAL_GPIO_DIGITAL_OUTPUT,
    HAL_GPIO_DIGITAL_INPUT,
    HAL_GPIO_ANALOG_INPUT,
    HAL_GPIO_UART_RX,
    HAL_GPIO_UART_TX
} Hal_gpio_function_t;

/*! HAL GPIO pin Pull resistor definition */
typedef enum {
    HAL_GPIO_PULL_DISABLED,
    HAL_GPIO_PULL_UP,
    HAL_GPIO_PULL_DOWN
} Hal_gpio_pullRes_t;

/*! HAL GPIO pin Pull resistor definition 
 * @note Setup current drive mode (Normal current / High current draw) 
 *        over an output pin
 */
typedef enum {
    HAL_GPIO_DRIVE_NONE,    /** < Means that it's Input */
    HAL_GPIO_DRIVE_NORMAL,
    HAL_GPIO_DRIVE_HIGH
} Hal_gpio_driveMode_t;

/*! HAL GPIO digital pin logic level: 0 or 1 */
typedef enum {
    HAL_GPIO_LOW_LEVEL,
    HAL_GPIO_HIGH_LEVEL
} Hal_gpio_pin_state_t;

/*! UART pins */
#define HAL_GPIO_UART_RX_PORT   GPIO_PRT5
#define HAL_GPIO_UART_RX_PIN    0U
#define HAL_GPIO_UART_TX_PORT   GPIO_PRT5
#define HAL_GPIO_UART_TX_PIN    1U

/*! ADC pins */
#define HAL_GPIO_ADC1_PORT  GPIO_PRT10
#define HAL_GPIO_ADC1_PIN   3U
#define HAL_GPIO_ADC2_PORT  GPIO_PRT10
#define HAL_GPIO_ADC2_PIN   2U
#define HAL_GPIO_ADC3_PORT  GPIO_PRT10
#define HAL_GPIO_ADC3_PIN   1U
#define HAL_GPIO_ADC4_PORT  GPIO_PRT10
#define HAL_GPIO_ADC4_PIN   0U

///////////////////////
// API
///////////////////////
void HAL_GPIO_init_pin(
    Hal_gpio_port_t* port, 
    Hal_gpio_pin_t pin,
    Hal_gpio_function_t func,
    Hal_gpio_pullRes_t pullRes,
    Hal_gpio_driveMode_t driveMode,
    Hal_gpio_pin_state_t initialState
);

void HAL_GPIO_deinit_pin(
    Hal_gpio_port_t* port,
    Hal_gpio_pin_t pin
);

Hal_gpio_pin_state_t HAL_GPIO_read_pin(
    Hal_gpio_port_t* port,
    Hal_gpio_pin_t pin
);

void HAL_GPIO_set_pin(
    Hal_gpio_port_t* port,
    Hal_gpio_pin_t pin,
    Hal_gpio_pin_state_t level
);

#endif  //HAL_GPIO_MODULE

/* [] END OF FILE */
