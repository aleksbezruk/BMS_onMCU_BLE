/**
 * @file  MAIN.h
 *
 * @brief Main task data & API definition.
 *
 * @version 0.4.0
 */

#ifndef MAIN_MODULE_H
#define MAIN_MODULE_H

#include "bms_events.h"

////////////////////////////
/// BMS states definition
////////////////////////////
/*! BMS states */
typedef enum {
    BMS_STATE_IDLE,
    BMS_STATE_DISCHARGE,
    BMS_STATE_CHARGE,
    BMS_STATE_ERROR,
    BMS_STATE_SHELF
} BMS_state_t;

////////////////////////////
/// API
////////////////////////////
void MAIN_post_evt(Main_evt_t* evt, Evt_types_t eventType);

#endif // MAIN_MODULE_H

/* [] END OF FILE */
