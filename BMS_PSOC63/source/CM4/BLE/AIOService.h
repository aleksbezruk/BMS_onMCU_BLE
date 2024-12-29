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
 * @version 0.1.0
 */

#ifndef BLE_AIOS_MODULE_H
#define BLE_AIOS_MODULE_H

#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "cycfg_gatt_db.h"

#include "bms_events.h"

///////////////////////
// Defines
///////////////////////

///////////////////////
// API
///////////////////////
void AIOS_updateVbat(Evt_ble_vbat_t* vbatEvt);
void AIOS_updateSwitchState(uint8_t swStates);
void AIOS_sendNotification(uint8_t swState, uint16_t conn_id);
void AIOS_handleCccdWritten(uint8_t* p_val);
void AIOS_handleSetSwicthWritten(uint8_t* p_val);

#endif //BLE_AIOS_MODULE_H

/* [] END OF FILE */
