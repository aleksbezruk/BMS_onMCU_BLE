/**
 * @file  BLE.h
 *
 * @brief BLE module functions declaration
 *
 * @version 0.4.0
 */

#ifndef BLE_MODULE_H
#define BLE_MODULE_H

#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"

#include "bms_events.h"

// ===========================
// Defines
// ===========================
/** BLE status */
typedef enum {
    BLE_STATUS_OK,
    BLE_STATUS_FAIL
} BLE_status_t;

/** This enumeration combines the advertising, connection states from two different
 * callbacks to maintain the status in a single state variable.
 */
typedef enum
{
    BLE_ADV_OFF_CONN_OFF,
    BLE_ADV_ON_CONN_OFF,
    BLE_ADV_OFF_CONN_ON
} BLE_adv_conn_mode_t;

// ===========================
// API
// ===========================
BLE_status_t BLE_init(void);
wiced_bt_dev_status_t BLE_startAdvertisement(uint16_t periodic_adv_int_min, uint16_t periodic_adv_int_max, wiced_bt_ble_periodic_adv_prop_t periodic_adv_properties);
wiced_bt_dev_status_t BLE_stopAdvertisement(void);
void BLE_post_evt(Ble_evt_t* evt, Evt_types_t eventType);

#endif //BLE_MODULE_H

/* [] END OF FILE */
