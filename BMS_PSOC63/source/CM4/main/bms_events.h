/**
 * @file  bms_events.h
 *
 * @brief Defines BMS application's events.
 *
 * @version 0.1.0
 */

#ifndef EVENTS_MODULE_H
#define EVENTS_MODULE_H

#include <stdint.h>

/** BMS events types */
typedef enum {
    EVT_ADC,
    EVT_BLE,
    EVT_SYSTEM,
    EVT_TYPE_MAX
} Evt_types_t;

/** ADC measurement data event */
typedef struct {
    int16_t bank1_mv;
    int16_t bank2_mv;
    int16_t bank3_mv;
    int16_t bank4_mv;
    int16_t full_mv;
} Evt_adc_data_t;

/** System event data */
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

/** Main task generic event type */
typedef union {
    Evt_adc_data_t adcData;
    Evt_sys_data_t sysEvtData;
} Main_evt_t;


/** Main task queue's item data structure */
typedef struct {
    Evt_types_t evtType;
    Main_evt_t evtData;
} Main_queue_data_t;

#endif // EVENTS_MODULE_H

/* [] END OF FILE */
