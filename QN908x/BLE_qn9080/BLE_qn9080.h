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

BLE_status_t BLE_startAdvertisement(uint16_t periodic_adv_int_min, uint16_t periodic_adv_int_max, wiced_bt_ble_periodic_adv_prop_t periodic_adv_properties);

#endif // BLE_QN9080_H
/* [] END OF FILE */