/**
 * @file  AIOService.c
 *
 * @brief SIG Automation IO service (AIOS) functions implementation
 *
 * @version 0.4.0
 */

#include <stdbool.h>
#include <string.h>

#include "MAIN.h"
#include "AIOService.h"
#include "qspyHelper.h"

// HAL
#include "hal.h"

///////////////////////
// Private data
///////////////////////
static volatile bool clientSubscribed;
static volatile uint8_t state;  // like app buffer to hold value while sending over BLE 

///////////////////////
// Public APIs
///////////////////////
/**
 * @brief Update battery level for Analog characteristics.
 * 
 * @details Update value in GATT DB.
 * 
 * @note GATT DB is defined in 'GeneratedSource/cycfg_gatt_db.c'
 * 
 * @param[in] vbatEvt - battery measurement event.
 * 
 * @retval None
 */
void AIOS_updateVbat(Evt_ble_vbat_t* vbatEvt)
{
    uint8_t* pFullBatValDB;
    uint8_t* pBank1ValDB;
    uint8_t* pBank2ValDB;
    uint8_t* pBank3ValDB;
    uint8_t* pBank4ValDB;

    /** Update attribute value in GATT DB */
    pFullBatValDB = app_gatt_db_ext_attr_tbl[8].p_data;
    pBank1ValDB = app_gatt_db_ext_attr_tbl[10].p_data;
    pBank2ValDB = app_gatt_db_ext_attr_tbl[12].p_data;
    pBank3ValDB = app_gatt_db_ext_attr_tbl[14].p_data;
    pBank4ValDB = app_gatt_db_ext_attr_tbl[16].p_data;
    __disable_irq();    // short critical section to avoid race condition with BLE stack
    memcpy(pFullBatValDB, (uint8_t*) &vbatEvt->adcData.full_mv, sizeof(vbatEvt->adcData.full_mv));
    memcpy(pBank1ValDB, (uint8_t*) &vbatEvt->adcData.bank1_mv, sizeof(vbatEvt->adcData.bank1_mv));
    memcpy(pBank2ValDB, (uint8_t*) &vbatEvt->adcData.bank2_mv, sizeof(vbatEvt->adcData.bank2_mv));
    memcpy(pBank3ValDB, (uint8_t*) &vbatEvt->adcData.bank3_mv, sizeof(vbatEvt->adcData.bank3_mv));
    memcpy(pBank4ValDB, (uint8_t*) &vbatEvt->adcData.bank4_mv, sizeof(vbatEvt->adcData.bank4_mv));
    __enable_irq();
}

/**
 * @brief Update switch state for Digital characteristic.
 * 
 * @details Update value in GATT DB.
 * 
 * @note 1. GATT DB is defined in 'GeneratedSource/cycfg_gatt_db.c'.
 *       2. Switches's state is hold by Main task in one variable to avoid any
 *          race condition issues. AIOS only notifies Main task about user request
 *          to enable Discharge/Charge. AIOS_updateSwitchState is invoked when Main task
 *          send response to the user request & update switch state value in GATT Server DB.
 * 
 * @param[in] swStates - swState.
 * 
 * @retval None
 */
void AIOS_updateSwitchState(uint8_t swStates)
{
    uint8_t* pSwStateValDB;

    /** Update attribute value in GATT DB */
    pSwStateValDB = app_gatt_db_ext_attr_tbl[5].p_data;
    __disable_irq();    // short critical section to avoid race condition with BLE stack
    pSwStateValDB[0] = swStates;
    __enable_irq();
}

/**
 * @brief Send AUTOMATION_IO_DIGITAL_IO_VALUE notification to subscribed Client.
 * 
 * @param[in] swState - switches state.
 * 
 * @param[in] conn_id - connection ID.
 * 
 * @retval None
 */
void AIOS_sendNotification(uint8_t swState, uint16_t conn_id)
{
    state = swState;

    if (clientSubscribed) {
        /** Send notification to Client */
        wiced_bt_gatt_status_t status = wiced_bt_gatt_server_send_notification(
            conn_id, 
            HDLC_AUTOMATION_IO_DIGITAL_IO_VALUE,    //attr_handle
            1U,                                     // val_len 
            (uint8_t *) &state,                     // p_app_buffer 
            NULL                                    // p_app_ctxt
        );
        if (WICED_BT_SUCCESS != status) {
            QS_BEGIN_ID(BLE_AIOS, 0 /*prio/ID for local Filters*/)
                QS_STR("Send notif error: ");
                QS_U16(0, status);
            QS_END()
            QS_FLUSH();
            HAL_ASSERT(0);
        }
    }
}

/**
 * @brief Handle CCCD written event.
 * 
 * @param[in] p_val - CCCD value requested by Client.
 * 
 * @retval None
 */
void AIOS_handleCccdWritten(uint8_t* p_val)
{
    uint16_t cccd = ((uint16_t) p_val[0]) | (((uint16_t) p_val[1]) << 8);

    if (cccd == 0x0001U) {
        QS_BEGIN_ID(BLE_AIOS, 0 /*prio/ID for local Filters*/)
            QS_STR("Client subscribed to notif");
        QS_END()
        clientSubscribed = true;
    } else if (cccd == 0x0000U) {
        QS_BEGIN_ID(BLE_AIOS, 0 /*prio/ID for local Filters*/)
            QS_STR("Client unsubscribed from notif");
        QS_END()
        clientSubscribed = false;
    } else {
        // Just ignore
        QS_BEGIN_ID(BLE_AIOS, 0 /*prio/ID for local Filters*/)
            QS_STR("Unexpected CCCD val:");
            QS_U16(0, cccd);
        QS_END()
    }
}

/**
 * @brief Handle user request to set switches' written event.
 * 
 * @details Notifies Main task about user request.
 * 
 * @param[in] p_val - CCCD value requested by Client.
 * 
 * @retval None
 */
void AIOS_handleSetSwicthWritten(uint8_t* p_val)
{
    uint8_t swStates;

    swStates = p_val[0]; // Just ignore 3 MSB's as Reserved

    Evt_sys_data_t evt = {0};
    evt.swStates = swStates;
    MAIN_post_evt((Main_evt_t*) &evt, EVT_SYSTEM);
}

/* [] END OF FILE */
