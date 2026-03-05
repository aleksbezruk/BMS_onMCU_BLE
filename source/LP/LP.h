/**
 * @file  LP.h
 *
 * @brief Defines Low Power modes and tickless mode of the BMS/MCU.
 *
 * @version 0.7.0
 */

#ifndef LP_MODULE_H
#define LP_MODULE_H

// =====================
// Defines
// =====================
/** BMS Low Power modes definitiom. 
 * @note 1. The modes introduce Abstarcted low power modes of BMS
 *          that allow to abstract BMS power model across different
 *          hardware/CPU vendors.
*/
typedef enum {
    LP_DISABLED_MODE = 0,   /*!< It's default mode, when BMS CPU's starts (power cycle/reboot) */
    LP_SLEEP_MODE,  /*!< CPU(s) stops instruction execution, Cortex-M CPU typicaly executes WFI(), WFE() instruction to enter Sleep mode */
    LP_DEEP_SLEEP_MODE, /*!< CPU sleep mode + peripheral deepSleep mode */
    LP_SHELF_MODE    /*!< For Future Use (TBD). BMS stops all measurements and communication. Can be activated by User Button */
} LP_modes_t;

/** Peripheral sleep readiness status */
typedef enum {
    LP_PERIPH_NOT_READY,
    LP_PERIPH_READY
} LP_periph_ready_t;

/** Low Power module operations status */
typedef enum {
    LP_INIT_STATUS_OK,
    LP_INIT_STATUS_FAIL
} LP_status_t;

// ======================
// API
// ======================
LP_status_t LP_init(void);
void LP_setMode(LP_modes_t mode);
LP_periph_ready_t LP_getPeriphStatus(void);

#endif //LP_MODULE_H

/* [] END OF FILE */
