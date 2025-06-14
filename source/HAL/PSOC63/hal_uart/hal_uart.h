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
 * @note PSOC63
 * 
 * @version 0.5.0
 */

#ifndef HAL_UART_H
#define HAL_UART_H

#include "BSP.h"
#include <stdint.h>
#include <stdbool.h>

// ===================
// Defines
// ===================
/*! RX callback type */
typedef bspUartRxCallback HAL_UART_rxCallback_t;

/*! UART config data structure  */
typedef struct {
    uint32_t baudRate;      /*!< UART baud rate in bits per second (e.g., 9600, 115200) */
    uint8_t numStartBits;   /*!< Number of start bits (usually 1) */
    uint8_t numStopBits;    /*!< Number of stop bits (1 or 2) */
    bool enParityCheck;     /*!< Enable (true) or disable (false) parity check */
} HAL_UART_config_t;

/*! UART status */
typedef enum {
    HAL_UART_SUCCESS,
    HAL_UART_FAIL
} HAL_UART_status_t;

// ===================
// API
// ===================
/**
 * @brief Initializes the UART peripheral with the specified configuration and RX callback.
 *
 * @param config Pointer to the UART configuration structure.
 * @param callback RX callback function to be called on data reception.
 * @return HAL_UART_status_t Returns HAL_UART_SUCCESS on success, HAL_UART_FAIL otherwise.
 */
HAL_UART_status_t HAL_UART_init(HAL_UART_config_t *config, HAL_UART_rxCallback_t callback);

bool HAL_UART_isTxReady(void);
void HAL_UART_txData(uint8_t *data, uint16_t len);
bool HAL_UART_isTxActive(void);

#endif  //HAL_UART_H

/* [] END OF FILE */
