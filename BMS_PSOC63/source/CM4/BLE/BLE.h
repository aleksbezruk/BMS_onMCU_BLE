/**
 * @file  BLE.h
 *
 * @brief BLE module functions declaration
 *
 * @version 0.1.0
 */

#ifndef BLE_MODULE_H
#define BLE_MODULE_H

///////////////////////
// Defines
///////////////////////
/** BLE status */
typedef enum {
    BLE_STATUS_OK,
    BLE_STATUS_FAIL
} BLE_status_t;

///////////////////////
// API
///////////////////////
BLE_status_t BLE_init(void);

#endif //BLE_MODULE_H

/* [] END OF FILE */
