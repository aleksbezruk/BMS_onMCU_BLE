/**
 * @file BLE_qn9080.h
 * @brief Header file for BLE_qn9080 module
 * @note The BLE_qn9080 module is used to evaluate and bring up the BLE functionality
 *       on the QN9080 platform.
 * @details The module uses the QN9080's BLE stack and its APIs to manage advertisements and connections.
 * @version 0.1.0
 */

#ifndef BLE_QN9080_H
#define BLE_QN9080_H

#include "stdint.h"
#include "ble_general.h"

// BLE module status codes
typedef enum {
    BLE_QN9080_STATUS_OK = 0,
    BLE_QN9080_STATUS_ERROR,
    BLE_QN9080_STATUS_INVALID_PARAM,
    BLE_QN9080_STATUS_NOT_INITIALIZED
} BLE_qn9080_status_t;

// Function declarations
void BLE_init(void);
BLE_qn9080_status_t BLE_StopAdvertising(void);

#endif // BLE_QN9080_H

/* [] END OF FILE */