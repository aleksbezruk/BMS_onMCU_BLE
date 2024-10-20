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
    EVT_BLE
} Evt_types_t;

/** ADC measurement data event */
typedef struct {
    int16_t bank1_mv;
    int16_t bank2_mv;
    int16_t bank3_mv;
    int16_t bank4_mv;
} Evt_adc_data_t;

/** Main task queue's item data structure */
typedef struct {
    Evt_types_t evtType;
    union {
        Evt_adc_data_t adcData;
    };
} Main_queue_data_t;

#endif // EVENTS_MODULE_H

/* [] END OF FILE */
