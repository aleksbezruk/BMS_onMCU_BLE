/**
 * @file  MAIN.h
 *
 * @brief Main task data & API definition.
 *
 * @version 0.7.0
 */

#ifndef MAIN_MODULE_H
#define MAIN_MODULE_H

#include "bms_events.h"

// ==========================
/// BMS states definition
// ==========================
/*! BMS states */
typedef enum {
    BMS_STATE_IDLE,
    BMS_STATE_DISCHARGE,
    BMS_STATE_CHARGE,
    BMS_STATE_ERROR,
    BMS_STATE_SHELF
} BMS_state_t;

/*! System PCBA test and trim data stored in EEPROM */
typedef struct {
    uint8_t adcError;           /**< ADC error compensation [% x 100] */
    uint8_t adcInterval;        /**< ADC measurements interval [seconds] */
    uint16_t advInterval;       /**< BLE advertisement interval [ms] */
    uint32_t bank1ConvRatio;    /**< ADC bank1 convertion ratio [x10^6] */
    uint32_t bank2ConvRatio;    /**< ADC bank2 convertion ratio [x10^6] */
    uint32_t bank3ConvRatio;    /**< ADC bank3 convertion ratio [x10^6] */
    uint32_t bank4ConvRatio;    /**< ADC bank4 convertion ratio [x10^6] */
} Trim_data_t;

// ========================
/// API
// ========================
void MAIN_post_evt(Main_evt_t* evt, Evt_types_t eventType);

// ========================
/// Debug/Testing API
// ========================
/**
 * @brief Manual balancer control function for debugging/testing
 * @param[in] evt System event containing balancer switch states
 * @note This function allows manual control of balancer switches,
 *       useful for debugging and testing purposes from qspyHelper.c
 */
void MAIN_SM_charge_setBal(Evt_sys_data_t* evt);

#endif // MAIN_MODULE_H

/* [] END OF FILE */
