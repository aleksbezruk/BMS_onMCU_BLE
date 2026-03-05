/**
 * @file  BatteryService.c
 *
 * @brief SIG Standard Battery service functions implementation
 *
 * @version 0.7.0
 */

#include <stdbool.h>

#include "BatteryService.h"
#include "qspyHelper.h"

// HAL
#include "hal.h"
#include "hal_ble.h"

// =======================
// Private data
// =======================
static volatile uint8_t batLevel_;
static volatile bool clientSubscribed;
static volatile bool forceNotification;

// =======================
// Public APIs
// =======================
/**
 * @brief Update battery percent level for Battery_Level characteristic.
 * 
 * @details Update value in GATT DB, send notification to BLE Client 
 *          if subscription is active. 
 * 
 * @note GATT DB is defined in 'GeneratedSource/cycfg_gatt_db.c'
 * 
 * @param[in] batLvl - battery level in percent.
 * 
 * @retval None
 */
void BAS_updateBatLevel(uint8_t batLvl)
{
    /** Update attribute value in GATT DB */
    if (batLevel_ != batLvl) {
        batLevel_ = batLvl;
        HAL_BLE_attribute_t attr;
        attr.attribute = HAL_BLE_ATTR_BATTERY_LEVEL;
        attr.p_value = (uint8_t *) &batLevel_;
        attr.length = sizeof(batLevel_);
        HAL_BLE_updateAttribute(&attr);
    }
}

/**
 * @brief Send Battery_Level notification to subscribed Client.
 * 
 * @param[in] batLvl - battery level in percent.
 * 
 * @param[in] conn_id - connection ID.
 * 
 * @retval None
 */
void BAS_sendNotification(uint8_t batLvl, uint16_t conn_id)
{
    if (clientSubscribed) {
        if (forceNotification) {
            forceNotification = false;
        }
        /** Send notification to Client */
        HAL_BLE_attribute_t attr;
        attr.attribute = HAL_BLE_ATTR_BATTERY_LEVEL;
        attr.p_value = &batLvl;
        attr.length = sizeof(batLvl);
        if (HAL_BLE_send_notif(&attr, conn_id) != HAL_BLE_SUCCESS) {
            QS_BEGIN_ID(BLE_BAS, 0 /*prio/ID for local Filters*/)
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
void BAS_handleCccdWritten(uint16_t cccd_val)
{
    if (cccd_val == 0x0001U) {
        QS_BEGIN_ID(BLE_BAS, 0 /*prio/ID for local Filters*/)
            QS_STR("Client subscribed to notif");                                                                                                                                                                                                           
        QS_END()
        clientSubscribed = true;
        forceNotification = true;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
    } else if (cccd_val == 0x0000U) {
        QS_BEGIN_ID(BLE_BAS, 0 /*prio/ID for local Filters*/)
            QS_STR("Client unsubscribed from notif");
        QS_END()
        clientSubscribed = false;                                                                                                                           
        forceNotification = false;
    } else {
        // Just ignore
        QS_BEGIN_ID(BLE_BAS, 0 /*prio/ID for local Filters*/)
            QS_STR("Unexpected CCCD val:");
            QS_U16(0, cccd_val);
        QS_END()
    }
}

/**
 * @brief Check if a notification is pending.
 *
 * @retval true if notification is pending, false otherwise.
 */
bool BAS_isNotifPending(void)
{
    return forceNotification;
}

/* [] END OF FILE */
