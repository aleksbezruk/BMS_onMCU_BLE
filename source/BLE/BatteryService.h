/**
 * @file  BatteryService.h
 *
 * @brief SIG Standard Battery service functions declaration
 *
 * @version 0.5.0
 */

#ifndef BLE_BAS_MODULE_H
#define BLE_BAS_MODULE_H

#include "bms_events.h"

#include <stdbool.h>

// =======================
// Defines
// =======================

// =======================
// API
// =======================
void BAS_updateBatLevel(uint8_t batLvl);
void BAS_handleCccdWritten(uint16_t cccd_val);
void BAS_sendNotification(uint8_t batLvl, uint16_t conn_id);
bool BAS_isNotifPending(void);

#endif //BLE_BAS_MODULE_H

/* [] END OF FILE */
