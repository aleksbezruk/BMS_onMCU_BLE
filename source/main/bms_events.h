/**
 * @file  bms_events.h
 *
 * @brief Defines BMS application's events.
 *
 * @version 0.7.0
 */

#ifndef EVENTS_MODULE_H
#define EVENTS_MODULE_H

#include <stdint.h>

#include "hal_ble.h"

/*! BMS events types */
typedef enum {
    EVT_ADC,
    EVT_SYSTEM,
    EVT_BLE_ADV_ON,
    EVT_BLE_ADV_OFF,
    EVT_BLE_VBAT,
    EVT_PCBA_TEST_TRIM,
    EVT_TYPE_MAX
} Evt_types_t;

// ==================
// ADC events
// ==================
/*! ADC measurement data event */
typedef struct {
    int16_t bank1_mv;
    int16_t bank2_mv;
    int16_t bank3_mv;
    int16_t bank4_mv;
    int16_t full_mv;
} Evt_adc_data_t;

// ==================
// System events
// ==================
/*! System event data */
typedef union {
    struct {
        uint8_t setDischState: 1;
        uint8_t setChargeState: 1;
        uint8_t setBank1Balancer: 1;
        uint8_t setBank2Balancer: 1;
        uint8_t setBank3Balancer: 1;
        uint8_t setBank4Balancer: 1;
        uint8_t RFU: 2;
    };
    uint8_t swStates;
} Evt_sys_data_t;

/*! PCBA trim modes/commands */
typedef enum {
    PCBA_TRIM_CMD,
    PCBA_REBOOT_CMD,
    PCBA_MAX_CMD
} Pcba_trim_cmds_t;

/*! System PCBA test and trim data */
typedef struct {
    uint8_t mode;               /**< 1 - trim, 2 - reboot */
    uint8_t adcError;           /**< ADC error compensation [% x 100] */
    uint32_t bank1ConvRatio;    /**< ADC bank1 convertion ratio [x10^6] */
    uint32_t bank2ConvRatio;    /**< ADC bank2 convertion ratio [x10^6] */
    uint32_t bank3ConvRatio;    /**< ADC bank3 convertion ratio [x10^6] */
    uint32_t bank4ConvRatio;    /**< ADC bank4 convertion ratio [x10^6] */
    uint8_t adcInterval;        /**< ADC measurements interval [seconds] */
    uint16_t advInterval;       /**< BLE advertisement interval [ms] */
} Evt_sys_pcba_test_t;

typedef Evt_sys_data_t Switch_state_t;

// ==================
// BLE events
// ==================
/*! BLE advertsing paramaters event */
typedef struct {
    HAL_BLE_adFlags_t advFlags;
    char local_name[HAL_BLE_LOCAL_NAME_MAX_LEN];
    HAL_BLE_adServicesList_t services;
    HAL_BLE_adServiceData_t service_data;
} Evt_ble_adv_param_t;

/*! BLE battery measurement data event */
typedef struct {
    uint8_t batLvlPercent;
    Evt_adc_data_t adcData;
} Evt_ble_vbat_t;

// =========================================
// Tasks' queues & generic event structure
// =========================================
/*! Main task generic event type */
typedef union {
    Evt_adc_data_t adcData;
    Evt_sys_data_t sysEvtData;
    Evt_sys_pcba_test_t pcbaTestTrim;
} Main_evt_t;

/*! BLE task generic event type */
typedef union {
    Evt_ble_adv_param_t advParam;
    Evt_ble_vbat_t vbat;
    Evt_sys_data_t sysData;
} Ble_evt_t;

/*! Main task queue's item data structure */
typedef struct {
    Evt_types_t evtType;
    Main_evt_t evtData;
} Main_queue_data_t;

/*! BLE task queue's item data structure */
typedef struct {
    Evt_types_t evtType;
    Ble_evt_t evtData;
} Ble_queue_data_t;

#endif // EVENTS_MODULE_H

/* [] END OF FILE */
