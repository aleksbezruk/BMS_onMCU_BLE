/**
 * @file  AIOService.h
 *
 * @brief SIG Automation IO service (AIOS) functions declaration
 * 
 * @note Actually the AIOS isn't "fully standard SIG servvice", 
 *       because its characteristics UUIDs & number of instances are not described in SIG spec:
 *       1. Digital characteristic - UUID isn't defined, so custon characteristic definition is used.
 *       2. Analog characteristic - the same as Digital char 
 *          + variable number of the char's instances complicates to have common AIOS structure
 *          thats why I guess SIG group didn't put any restrictions to AIOS charactersitics definition
 *          because in almost all cases it's application dependent. 
 *
 * @version 0.7.0
 */

#ifndef BLE_AIOS_MODULE_H
#define BLE_AIOS_MODULE_H


#include "bms_events.h"

// =======================
// Defines
// =======================

// =======================
// API
// =======================
void AIOS_updateVbat(Evt_ble_vbat_t* vbatEvt);
void AIOS_updateSwitchState(uint8_t swStates);
void AIOS_sendNotification(uint8_t swState, uint16_t conn_id);
void AIOS_handleCccdWritten(uint16_t cccd_val);
void AIOS_handleSetSwicthWritten(uint8_t swState_val);
void AIOS_handleSetTrim(Evt_sys_pcba_test_t *trim);
void AIOS_sendNotifTrimValue(Evt_sys_pcba_test_t *trim, uint16_t conn_id);
void AIOS_handleTrimCccdWritten(uint16_t cccd_val);

#endif //BLE_AIOS_MODULE_H

/* [] END OF FILE */
