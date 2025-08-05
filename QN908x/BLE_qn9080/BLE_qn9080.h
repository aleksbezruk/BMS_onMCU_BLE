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
    BLE_QN9080_STATUS_SUCCESS = 0,  // Success status
    BLE_QN9080_STATUS_OK = 0,       // Alias for backward compatibility
    BLE_QN9080_STATUS_ERROR,
    BLE_QN9080_STATUS_INVALID_PARAM,
    BLE_QN9080_STATUS_NOT_INITIALIZED
} BLE_qn9080_status_t;

// Function declarations
void BLE_init(void);
BLE_qn9080_status_t BLE_StopAdvertising(void);

// GATT BMS functions
BLE_qn9080_status_t BLE_UpdateBMSCharacteristics(deviceId_t deviceId);
void BLE_SetBMSData(uint8_t batteryLevel);

// NVM Application Layer Functions (required by BLE host library)
uint16_t App_NvmRead(uint8_t mEntryId, void* pBuff, uint16_t buffLen, uint16_t offset);
uint16_t App_NvmWrite(uint8_t mEntryId, void* pBuff, uint16_t buffLen, uint16_t offset);
uint16_t App_NvmErase(uint8_t mEntryId);

#endif // BLE_QN9080_H

/* [] END OF FILE */