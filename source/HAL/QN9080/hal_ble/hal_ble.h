/**
 * @file hal_ble.h
 * @brief Header file for BLE (Bluetooth Low Energy) HAL (Hardware Abstraction Layer) functions.
 * @version 0.7.0
 * @note QN9080 specific implementation
 */

#ifndef HAL_BLE_H
#define HAL_BLE_H

#include <stdbool.h>
#include <stdint.h>

// ===========================
// Defines
// ===========================

/*! Maximum length of the local name */
#define HAL_BLE_LOCAL_NAME_MAX_LEN  15U

/*! HAL BLE status */
typedef enum {
    HAL_BLE_SUCCESS,
    HAL_BLE_ERROR
} HAL_BLE_status_t;

/*! HAL BLE advertisement flags mask */
typedef enum {
    HAL_BLE_AD_FLAG_LIMITED_DISCOVERABLE = (1 << 0), /**< Limited discoverable mode */
    HAL_BLE_AD_FLAG_GENERAL_DISCOVERABLE = (1 << 1), /**< General discoverable mode */
    HAL_BLE_AD_FLAG_BLE_BREDR_NOT_SUPPORTED = (1 << 2) /**< BLE/BR/EDR not supported */
} HAL_BLE_adFlags_t;

/*! HAL BLE advertisement services list mask */
typedef enum {
    HAL_BLE_AD_SERVICE_NONE = 0, /**< No services */
    HAL_BLE_AD_SERVICE_BAS = (1 << 0), /**< Battery Service */
    HAL_BLE_AD_SERVICE_AIOS = (1 << 1)  /**< Automation IO Service */
} HAL_BLE_adServicesList_t;

/*! HAL BLE advertisement BAS data (Battery level in percent). */
typedef uint8_t HAL_BLE_adServiceData_t;

/*! Advertising data structure */
typedef struct {
    HAL_BLE_adFlags_t flags;
    char local_name[HAL_BLE_LOCAL_NAME_MAX_LEN];
    HAL_BLE_adServicesList_t services;
    HAL_BLE_adServiceData_t service_data;   /**< Service Data, for now only BAS data */
} HAL_BLE_adv_data_t;

/*! HAL BLE scan response data for TX level. */
typedef int8_t HAL_BLE_scan_response_data_t;

/*! HAL BLE advertisement state */
typedef enum {
    HAL_BLE_ADV_STATE_OFF,
    HAL_BLE_ADV_STATE_ON
} HAL_BLE_adv_state_t;

/*! HAL BLE connection callback type */
typedef void (*HAL_BLE_conn_callback_t)(uint16_t connId, bool connected, uint16_t disconnectReason);

/*! HAL BLE initialization callback type */
typedef void (*HAL_BLE_init_callback_t)(bool success);

/*! HAL BLE advertising state changed callback type */
typedef void (*HAL_BLE_advState_callback_t)(HAL_BLE_adv_state_t advState);

/*! HAL BLE MTU changed callback type */
typedef void (*HAL_BLE_mtuChanged_callback_t)(uint16_t mtu);

/*! HAL BLE attribute type */
typedef enum {
    HAL_BLE_ATTR_BATTERY_LEVEL,
    HAL_BLE_ATTR_BAS_CCCD,
    HAL_BLE_ATTR_AIOS_DIGITAL_IO_VALUE,
    HAL_BLE_ATTR_AIOS_BANK1_VALUE,
    HAL_BLE_ATTR_AIOS_BANK2_VALUE,
    HAL_BLE_ATTR_AIOS_BANK3_VALUE,
    HAL_BLE_ATTR_AIOS_BANK4_VALUE,
    HAL_BLE_ATTR_AIOS_FULLBAT_VALUE,
    HAL_BLE_ATTR_AIOS_CCCD,
    HAL_BLE_ATTR_AIOS_TRIM_VALUE
} HAL_BLE_attribute_type_t;

/*! HAL BLE attribute structure */
typedef struct {
    HAL_BLE_attribute_type_t attribute;
    uint8_t *p_value;
    uint16_t length;
} HAL_BLE_attribute_t;

// ===========================
// API
// ===========================
void HAL_BLE_init(void);
HAL_BLE_status_t HAL_BLE_startAdvertisement(HAL_BLE_adv_data_t *advData, HAL_BLE_scan_response_data_t *scanResponseData);
HAL_BLE_status_t HAL_BLE_stopAdvertisement(void);
HAL_BLE_status_t HAL_BLE_updateAdvertisingData(HAL_BLE_adServiceData_t advData);
void HAL_BLE_updateAttribute(HAL_BLE_attribute_t *attr);
HAL_BLE_status_t HAL_BLE_send_notif(HAL_BLE_attribute_t *attr, uint16_t conn_id);
void HAL_BLE_registerConnectionCallback(HAL_BLE_conn_callback_t callback);
void HAL_BLE_registerStackInitCallback(HAL_BLE_init_callback_t callback);
void HAL_BLE_registerAdvertisingStateChangedCallback(HAL_BLE_advState_callback_t callback);
void HAL_BLE_registerMtuChangedCallback(HAL_BLE_mtuChanged_callback_t callback);

#endif // HAL_BLE_H

/* [] END OF FILE */
