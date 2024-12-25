/**
 * @file  BatteryService.h
 *
 * @brief SIG Standard Battery service functions declaration
 *
 * @version 0.1.0
 */

#ifndef BLE_BAS_MODULE_H
#define BLE_BAS_MODULE_H

#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "cycfg_gatt_db.h"

#include "bms_events.h"

///////////////////////
// Defines
///////////////////////

///////////////////////
// API
///////////////////////
void BAS_updateBatLevel(uint8_t batLvl);
void BAS_handleCccdWritten(uint8_t* p_val);
void BAS_sendNotification(uint8_t batLvl, uint16_t conn_id);

#endif //BLE_BAS_MODULE_H

/* [] END OF FILE */
