/**
 * @file  MAIN.h
 *
 * @brief Main task data & API definition.
 *
 * @version 0.1.0
 */

#ifndef MAIN_MODULE_H
#define MAIN_MODULE_H

#include "bms_events.h"

////////////////////////////
/// Data types
////////////////////////////
/*! BMS discharge switch state */
typedef enum {
    MAIN_BMS_DISCHARGE_OFF,
    MAIN_BMS_DISCHARGE_ON
} MAIN_dischargeSw_state_t;

/*! BMS charge switch state */
typedef enum {
    MAIN_BMS_CHARGE_OFF,
    MAIN_BMS_CHARGE_ON
} MAIN_chargeSw_state_t;

/*! BMS bank balancer state */
typedef enum {
    MAIN_BMS_BALANCER_OFF,
    MAIN_BMS_BALANCER_ON
} MAIN_balancer_state_t;

/*! BMS bank number */
typedef enum {
    MAIN_BMS_BANK1,
    MAIN_BMS_BANK2,
    MAIN_BMS_BANK3,
    MAIN_BMS_BANK4
} MAIN_bank_t;
#define MAIN_BMS_BANK1_MASK  (1 << MAIN_BMS_BANK1)
#define MAIN_BMS_BANK2_MASK  (1 << MAIN_BMS_BANK2)
#define MAIN_BMS_BANK3_MASK  (1 << MAIN_BMS_BANK3)
#define MAIN_BMS_BANK4_MASK  (1 << MAIN_BMS_BANK4)

#define MAIN_BMS_ALL_BANKS  (MAIN_BMS_BANK1_MASK | \
                             MAIN_BMS_BANK2_MASK | \
                             MAIN_BMS_BANK3_MASK | \
                             MAIN_BMS_BANK4_MASK)

#define BMS_DISCHARGE_PORT GPIO_PRT9
#define BMS_DISCHARGE_PIN 5U
#define BMS_DISCHARGE_ON 0U
#define BMS_DISCHARGE_OFF 1U

#define BMS_CHARGE_PORT GPIO_PRT9
#define BMS_CHARGE_PIN 4U
#define BMS_CHARGE_ON 0U
#define BMS_CHARGE_OFF 1U

#define BMS_BAL_BANK1_PORT GPIO_PRT9
#define BMS_BAL_BANK1_PIN 3U
#define BMS_BAL_BANK1_ON 0U
#define BMS_BAL_BANK1_OFF 1U
#define BMS_BAL_BANK2_PORT GPIO_PRT9
#define BMS_BAL_BANK2_PIN 2U
#define BMS_BAL_BANK2_ON 0U
#define BMS_BAL_BANK2_OFF 1U
#define BMS_BAL_BANK3_PORT GPIO_PRT9
#define BMS_BAL_BANK3_PIN 1U
#define BMS_BAL_BANK3_ON 0U
#define BMS_BAL_BANK3_OFF 1U
#define BMS_BAL_BANK4_PORT GPIO_PRT9
#define BMS_BAL_BANK4_PIN 0U
#define BMS_BAL_BANK4_ON 0U
#define BMS_BAL_BANK4_OFF 1U

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
