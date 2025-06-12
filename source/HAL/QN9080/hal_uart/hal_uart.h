/**
 * @file  hal_uart.h
 *
 * @brief Definition of UART functions:
 *          1. Init UART:
 *              - Enable TX & RX - full duplex;
 *              - Config Baud Rate; 
 *              - Config start, stop bits;
 *              - Config Parity Check.
 *          2. Receive Raw data byte-by-byte and notify about data to Application (QSPY).
 *          3. Transmit Raw data requested by Application (QSPY). 
 *
 * @note QN9080
 * 
 * @version 0.5.0
 */

#ifndef HAL_UART_MODULE
#define HAL_UART_MODULE

#include <stdint.h>
#include <stdbool.h>

///////////////////////
// Defines
///////////////////////
/*! RX callback type */
typedef void (*HAL_UART_rxCallback_t)(uint8_t *data, uint16_t len);

/*! UART config data structure  */
typedef struct {
    uint32_t baudRate;      /*< 115200, 230400, ... , 921600 */
    uint8_t numStartBits;   /*< 1, 2, ... */
    uint8_t numStopBits;    /*< 1, 2, ... */
    bool enParityCheck;     /*< RFU */
} HAL_UART_config_t;

/*! UART status */
typedef enum {
    HAL_UART_SUCCESS,
    HAL_UART_FAIL
} HAL_UART_status_t;

///////////////////////
// API
///////////////////////
HAL_UART_status_t HAL_UART_init(HAL_UART_config_t *config, HAL_UART_rxCallback_t callback);
bool HAL_UART_isTxReady(void);
void HAL_UART_txData(uint8_t *data, uint16_t len);
bool HAL_UART_isTxActive(void);

#endif  //HAL_UART_MODULE

/* [] END OF FILE */
