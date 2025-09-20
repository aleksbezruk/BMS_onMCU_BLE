/**
 * @file  AIOService.c
 *
 * @brief SIG Automation IO service (AIOS) functions implementation
 *
 * @version 0.6.0
 */

#include <stdbool.h>
#include <string.h>

#include "MAIN.h"
#include "AIOService.h"
#include "qspyHelper.h"

// HAL
#include "hal.h"
#include "hal_ble.h"

// =======================
// Private data
// =======================
static volatile bool clientSubscribed;
static volatile uint8_t state;  // like app buffer to hold value while sending over BLE 

// =======================
// Public APIs
// =======================
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
    // Full Battery Voltage
    HAL_BLE_attribute_t attr;
    attr.attribute = HAL_BLE_ATTR_AIOS_FULLBAT_VALUE;
    attr.length = sizeof(vbatEvt->adcData.full_mv);
    attr.p_value = (uint8_t*)&vbatEvt->adcData.full_mv;
    HAL_BLE_updateAttribute(&attr);
    // Bank1
    attr.attribute = HAL_BLE_ATTR_AIOS_BANK1_VALUE;
    attr.length = sizeof(vbatEvt->adcData.bank1_mv);
    attr.p_value = (uint8_t*)&vbatEvt->adcData.bank1_mv;
    HAL_BLE_updateAttribute(&attr);
    // Bank2
    attr.attribute = HAL_BLE_ATTR_AIOS_BANK2_VALUE;
    attr.length = sizeof(vbatEvt->adcData.bank2_mv);
    attr.p_value = (uint8_t*)&vbatEvt->adcData.bank2_mv;
    HAL_BLE_updateAttribute(&attr);
    // Bank3
    attr.attribute = HAL_BLE_ATTR_AIOS_BANK3_VALUE;
    attr.length = sizeof(vbatEvt->adcData.bank3_mv);
    attr.p_value = (uint8_t*)&vbatEvt->adcData.bank3_mv;
    HAL_BLE_updateAttribute(&attr);
    // Bank4
    attr.attribute = HAL_BLE_ATTR_AIOS_BANK4_VALUE;
    attr.length = sizeof(vbatEvt->adcData.bank4_mv);
    attr.p_value = (uint8_t*)&vbatEvt->adcData.bank4_mv;
    HAL_BLE_updateAttribute(&attr);
}

/**
 * @brief Update switch state for Digital characteristic.
 * 
 * @details Update value in GATT DB.
 * 
 * @note 1. GATT DB is defined in 'GeneratedSource/cycfg_gatt_db.c' for PSOC63.
 *       2. Switches's state is hold by Main task in one variable to avoid any
 *          race condition issues. AIOS only notifies Main task about user request
 *          to enable Discharge/Charge. AIOS_updateSwitchState is invoked when Main task
 *          send response to the user request & update switch state value in GATT Server DB.
 * 
 * @param[in] swStates - switches state.
 * 
 * @retval None
 */
void AIOS_updateSwitchState(uint8_t swStates)
{
    HAL_BLE_attribute_t attr;
    attr.attribute = HAL_BLE_ATTR_AIOS_DIGITAL_IO_VALUE;
    attr.length = sizeof(swStates);
    attr.p_value = (uint8_t*)&swStates;
    HAL_BLE_updateAttribute(&attr);
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
        HAL_BLE_attribute_t attr;
        attr.attribute = HAL_BLE_ATTR_AIOS_DIGITAL_IO_VALUE;
        attr.p_value = (uint8_t *) &state;
        attr.length = sizeof(state);
        if (HAL_BLE_send_notif(&attr, conn_id) != HAL_BLE_SUCCESS) {
            QS_BEGIN_ID(BLE_AIOS, 0 /*prio/ID for local Filters*/)
                QS_STR("Send notif error");
            QS_END()
            HAL_ASSERT(0, __FILE__, __LINE__);
        }
    }
}

/**
 * @brief Handle CCCD written event.
 * 
 * @param[in] cccd_val - CCCD value requested by Client.
 * 
 * @retval None
 */
void AIOS_handleCccdWritten(uint16_t cccd_val)
{
    if (cccd_val == 0x0001U) {
        QS_BEGIN_ID(BLE_AIOS, 0 /*prio/ID for local Filters*/)
            QS_STR("Client subscribed to notif");
        QS_END()
        clientSubscribed = true;
    } else if (cccd_val == 0x0000U) {
        QS_BEGIN_ID(BLE_AIOS, 0 /*prio/ID for local Filters*/)
            QS_STR("Client unsubscribed from notif");
        QS_END()
        clientSubscribed = false;
    } else {
        // Just ignore
        QS_BEGIN_ID(BLE_AIOS, 0 /*prio/ID for local Filters*/)
            QS_STR("Unexpected CCCD val:");
            QS_U16(0, cccd_val);
        QS_END()
    }
}

/**
 * @brief Handle user request to set switches' written event.
 * 
 * @details Notifies Main task about user request.
 * 
 * @param[in] swState_val - Switch state value requested by Client.
 * 
 * @retval None
 */
void AIOS_handleSetSwicthWritten(uint8_t swState_val)
{
    uint8_t swStates;

    swStates = swState_val; // Just ignore 3 MSB's as Reserved

    Evt_sys_data_t evt = {0};
    evt.swStates = swStates;
    MAIN_post_evt((Main_evt_t*) &evt, EVT_SYSTEM);
}

/* [] END OF FILE */
