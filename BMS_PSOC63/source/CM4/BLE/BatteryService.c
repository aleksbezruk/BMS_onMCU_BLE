/**
 * @file  BatteryService.c
 *
 * @brief SIG Standard Battery service functions implementation
 *
 * @version 0.1.0
 */

#include "BatteryService.h"
#include "qspyHelper.h"

#include "cybsp.h"

#include <stdbool.h>

///////////////////////
// Private data
///////////////////////
static volatile uint8_t batLevel_;
static volatile bool clientSubscribed;
static volatile bool forceNotification;

///////////////////////
// Public APIs
///////////////////////
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
    uint8_t* pBasValDB;

    /** Update attribute value in GATT DB */
    if (batLevel_ != batLvl) {
        batLevel_ = batLvl;
        pBasValDB = app_gatt_db_ext_attr_tbl[2].p_data;
        __disable_irq();    // short critical section to avoid race condition with BLE stack
        pBasValDB[0] = batLvl;
        __enable_irq();
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
        wiced_bt_gatt_status_t status = wiced_bt_gatt_server_send_notification(
            conn_id, 
            HDLC_BAS_BATTERY_LEVEL_VALUE,       //attr_handle
            1U,                                 // val_len 
            (uint8_t *) &batLevel_,             // p_app_buffer 
            NULL                                // p_app_ctxt
        );
        if (WICED_BT_SUCCESS != status) {
            QS_BEGIN_ID(BLE_BAS, 0 /*prio/ID for local Filters*/)
                QS_STR("Send notif error: ");
                QS_U16(0, status);
            QS_END()
            QS_FLUSH();
            CY_ASSERT(0);
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
void BAS_handleCccdWritten(uint8_t* p_val)
{
    uint16_t cccd = ((uint16_t) p_val[0]) | (((uint16_t) p_val[1]) << 8);

    if (cccd == 0x0001U) {
        QS_BEGIN_ID(BLE_BAS, 0 /*prio/ID for local Filters*/)
            QS_STR("Client subscribed to notif");
        QS_END()
        clientSubscribed = true;
        forceNotification = true;
    } else if (cccd == 0x0000U) {
        QS_BEGIN_ID(BLE_BAS, 0 /*prio/ID for local Filters*/)
            QS_STR("Client unsubscribed from notif");
        QS_END()
        clientSubscribed = false;
        forceNotification = false;
    } else {
        // Just ignore
        QS_BEGIN_ID(BLE_BAS, 0 /*prio/ID for local Filters*/)
            QS_STR("Unexpected CCCD val:");
            QS_U16(0, cccd);
        QS_END()
    }
}

bool BAS_isNotifPending(void)
{
    return ((forceNotification == true)? true: false);
}

/* [] END OF FILE */
