/**
 * @file  MAIN.h
 *
 * @brief Main task data & API definition.
 *
 * @version 0.6.0
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
