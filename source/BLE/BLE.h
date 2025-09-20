/**
 * @file  BLE.h
 *
 * @brief BLE module functions declaration
 *
 * @version 0.6.0
 */

#ifndef BLE_MODULE_H
#define BLE_MODULE_H

#include "hal_ble.h"

#include "bms_events.h"

// ===========================
// Defines
// ===========================
/*! BLE status */
typedef enum {
    BLE_STATUS_OK,              /**< Success */
    BLE_STATUS_FAIL,            /**< General failure */
    BLE_STATUS_START_ADV_FAIL,  /**< Start advertisement failure */
    BLE_STATUS_STOP_ADV_FAIL     /**< Stop advertisement failure */
} BLE_status_t;

/*!
 * This enumeration combines the advertising, connection states from two different
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
BLE_status_t BLE_startAdvertisement(HAL_BLE_adv_data_t *p_adv_data);
BLE_status_t BLE_stopAdvertisement(void);
void BLE_post_evt(Ble_evt_t* evt, Evt_types_t eventType);

#endif //BLE_MODULE_H

/* [] END OF FILE */
