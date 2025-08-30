/**
 * @file  BLE.c
 *
 * @brief BLE module functions definition
 *
 * @version 0.4.0
 */

// standard includes
#include <string.h>
#include <stdint.h>

// QSPY includes
#include "qspyHelper.h"

// BLE includes
#include "BLE.h"

// RTOS includes
#include "OSAL.h"

// BLE Services
#include "BatteryService.h"
#include "AIOService.h"

// HAL
#include "hal.h"
#include "hal_ble.h"

// =======================
// Defines
// =======================
/**
 * BLE task stack size
 */
#define BLE_TASK_STACK_SIZE 1600U   /**< bytes, aligned to 8 bytes */
#define BLE_QUEUE_SIZE 8U

#define BLE_INVALID_CONN_ID 255U

// =======================
// Functions prototype
// =======================
static void bleTask_(OSAL_arg_t arg);
static void parseQueueItem_(Ble_queue_data_t* queueItem);

static void print_connection_id(uint16_t id);
// Callbacks
void ble_conn_callback(uint16_t connId, bool connected, uint16_t disconnectReason);
void ble_init_callback(bool success);
void ble_adv_state_callback(HAL_BLE_adv_state_t advState);
void ble_mtu_changed_callback(uint16_t mtu);

// =======================  
// Private data
// =======================
/** BLE task */
OSAL_TASK_DEFINE(bleTask);

#if !defined(BMS_DISABLE_OSAL_STATIC_ALL)
/** 
 *  @note In stack words because stack pointer should be aligned to 
 *  8 bytes boundary per the RTOS requirements.
 */
static uint64_t bleTaskStack_[BLE_TASK_STACK_SIZE/8U];
#else
/*! For some ports (e.g. QN908x) the OSAL static allocation is not supported */
#define bleTaskStack_ NULL
#endif  // !BMS_DISABLE_OSAL_STATIC_ALL

/** Event queue */
OSAL_QUEUE_DEFINE(bleTaskQueueHandle);
/** 
 *  Queue for BLE events, see \ref Ble_queue_data_t
 *  @note The queue size should be enough to handle all BLE events
 *        in the worst case scenario.
 */
#define BLE_QUEUE_SIZE 8U
#if !defined(BMS_DISABLE_OSAL_STATIC_ALL)
static Ble_queue_data_t bleQueueSto[BLE_QUEUE_SIZE];
#else
/*! For some ports (e.g. QN908x) the OSAL static allocation is not supported */
#define bleQueueSto NULL
#endif  // !BMS_DISABLE_OSAL_STATIC_ALL

/** BLE advertisement data */
static volatile uint8_t adv_battery_service_data = 0x64;    // 100% start value at initialization

/** BLE state variables */
static volatile uint16_t client_id = BLE_INVALID_CONN_ID;
static volatile BLE_adv_conn_mode_t ble_adv_conn_state = BLE_ADV_OFF_CONN_OFF;

// =======================
// Code
// =======================

/**
 * @brief Init BLE peripheral, BLE stack
 * 
 * @param None
 * 
 * @retval See \ref BLE_status_t
 * 
 */
BLE_status_t BLE_init(void)
{
    BLE_status_t status = BLE_STATUS_OK;

    /** Register BLE connection callback */
    HAL_BLE_registerConnectionCallback(ble_conn_callback);
    /** Register BLE stack initialization callback */
    HAL_BLE_registerStackInitCallback(ble_init_callback);
    /** Register BLE advertising state callback */
    HAL_BLE_registerAdvertisingStateChangedCallback(ble_adv_state_callback);
    /** Register BLE MTU changed callback */
    HAL_BLE_registerMtuChangedCallback(ble_mtu_changed_callback);

    /** Init BLE stack */
    HAL_BLE_init();

    /** Create RTOS task */
    OSAL_Status_t osal_status = OSAL_SUCCESS;
    OSAL_TASK_CREATE(
        OSAL_TASK_GET_HANDLE(bleTask),
        bleTask_,
        "bleTask",
        bleTaskStack_,             // should be aligned to 8 bytes
        BLE_TASK_STACK_SIZE,       // in bytes
        OSAL_BLE_TASK_PRIORITY,    // prio
        NULL,                      // no args
        osal_status
    );
    if (osal_status != OSAL_SUCCESS) {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("BLE Task Creation failed:");
            QS_U8(0, osal_status);
        QS_END()
        status = BLE_STATUS_FAIL;
    }

    return status;
}

/**
 * @brief Start BLE advertisement
 *
 * @param[in] p_adv_data Pointer to advertisement data
 * 
 * @retval See \ref BLE_status_t
 * 
 */
BLE_status_t BLE_startAdvertisement(HAL_BLE_adv_data_t *p_adv_data)
{
    HAL_BLE_status_t status = HAL_BLE_startAdvertisement(p_adv_data, NULL);
    return (status == HAL_BLE_SUCCESS) ? BLE_STATUS_OK : BLE_STATUS_START_ADV_FAIL;
}

/**
 * @brief Stop BLE advertisement
 * 
 * @param None
 * 
 * @retval See \ref BLE_status_t
 * 
 */
BLE_status_t BLE_stopAdvertisement(void)
{
    HAL_BLE_status_t status = HAL_BLE_stopAdvertisement();
    return (status == HAL_BLE_SUCCESS) ? BLE_STATUS_OK : BLE_STATUS_STOP_ADV_FAIL;
}

/**
 * @brief BLE task's handler
 * 
 * @param[in] arg the argument passed from the thread create call to the entry function
 * 
 * @retval None
 */
static void bleTask_(OSAL_arg_t arg)
{
    (void) arg;
    OSAL_Status_t osal_status = OSAL_SUCCESS;
    Ble_queue_data_t queueItem;

    /** Create an event Queue */
    OSAL_QUEUE_CREATE(
        bleTaskQueueHandle, // handle name
        "bleTaskQueue",
        BLE_QUEUE_SIZE, // queue size
        sizeof(Ble_queue_data_t),
        bleQueueSto,
        osal_status
    );
    if (osal_status != OSAL_SUCCESS) {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("BLE Task Queue Creation failed:");
            QS_U8(0, osal_status);
        QS_END()
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    /** Events processing loop */
    while(1) {
        /** Wait for event on OSAL */
        OSAL_QUEUE_GET(
            OSAL_QUEUE_GET_HANDLE(bleTaskQueueHandle), 
            &queueItem, 
            OSAL_QUEUE_TIMEOUT_NEVER,
            osal_status
        );
        if (osal_status != OSAL_SUCCESS) {
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("BLE Task Queue Get failed:");
                QS_U8(0, osal_status);
            QS_END()
            /** If failed to get an event from the queue, assert */
            // Note: This is a fatal error, so we assert here.
            HAL_ASSERT(0, __FILE__, __LINE__);
        }

        /** Handle an evt */
        parseQueueItem_(&queueItem);

        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("BLE task handle evt:");
            QS_U8(0, queueItem.evtType);
        QS_END()
    }
}

// =======================
/// Queue functions/APIs
// =======================
/**
 * @brief Posts an event into BLE task event queue
 *
 * @param[in] evt Event to post
 *
 * @param[in] eventType Event type to post
 *
 * @retval None
 *
 */
void BLE_post_evt(Ble_evt_t* evt, Evt_types_t eventType)
{
    HAL_ASSERT((evt != NULL) && (eventType < EVT_TYPE_MAX), __FILE__, __LINE__);

    OSAL_Status_t status = OSAL_SUCCESS;
    Ble_queue_data_t queueItem;

    queueItem.evtType = eventType;
    memcpy((uint8_t*) &queueItem.evtData, (uint8_t*) evt, sizeof(Ble_evt_t));

    OSAL_QUEUE_PUT(
        OSAL_QUEUE_GET_HANDLE(bleTaskQueueHandle), 
        &queueItem, 
        OSAL_QUEUE_TIMEOUT_NEVER,
        status
    );
    if (status != OSAL_SUCCESS) {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("BLE Task Queue Put failed:");
            QS_U8(0, status);
        QS_END()
        HAL_ASSERT(0, __FILE__, __LINE__);
    }
}

/**
 * @brief Parses a queue item.
 *
 * @param[in] queueItem Queue item to parse
 *
 * @retval None
 */
static void parseQueueItem_(Ble_queue_data_t* queueItem)
{
    switch (queueItem->evtType)
    {
        case EVT_BLE_ADV_ON:
        {
            HAL_BLE_adv_data_t *adv_data = (HAL_BLE_adv_data_t *)&queueItem->evtData.advParam;
            BLE_startAdvertisement(adv_data);
            break;
        }

        case EVT_BLE_ADV_OFF:
        {
            BLE_stopAdvertisement();
            break;
        }

        case EVT_BLE_VBAT:
        {
            /** Is bat level changed ? */
            if ((queueItem->evtData.vbat.batLvlPercent != adv_battery_service_data) ||
                 BAS_isNotifPending()) {
                /** Update battery level in advertisement data */
                adv_battery_service_data = queueItem->evtData.vbat.batLvlPercent;
                HAL_BLE_status_t status = HAL_BLE_updateAdvertisingData(adv_battery_service_data);
                HAL_ASSERT((status == HAL_BLE_SUCCESS), __FILE__, __LINE__);

                /** Update Battery level in GATT DB for BAS */
                BAS_updateBatLevel(queueItem->evtData.vbat.batLvlPercent);
                /** Is client connected ? */
                if (ble_adv_conn_state == BLE_ADV_OFF_CONN_ON) {
                    BAS_sendNotification(queueItem->evtData.vbat.batLvlPercent, client_id);
                }
            }

            /** Send VBAT to AIOS */
            AIOS_updateVbat(&queueItem->evtData.vbat);
            break;
        }

        case EVT_SYSTEM:
        {
            /** Send switches' state to AIOS */
            AIOS_updateSwitchState(queueItem->evtData.sysData.swStates);
            /** Is client connected ? */
            if (ble_adv_conn_state == BLE_ADV_OFF_CONN_ON) {
                AIOS_sendNotification(queueItem->evtData.sysData.swStates, client_id);
            }
            break;
        }

        default:
        {
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("ERROR: Unhandled BLE Task Queue Item case:");
                QS_U8(0, queueItem->evtType);
            QS_END()
            /** If the event type is not handled, assert */
            // Note: This is a fatal error, so we assert here.
            HAL_ASSERT(0, __FILE__, __LINE__);   // unexpected evt
            break;
        }
    }
}

// =================================
// Application Callbacks
// =================================

/**
 * @brief BLE Stack Initialization Callback
 *
 * @param[in] success Initialization status
 *
 * @retval None
 */
void ble_init_callback(bool success)
{
    if (success) {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("BLE Stack Initialized");
        QS_END()
    } else {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("BLE Stack Initialization Failed");
        QS_END()
        HAL_ASSERT(0, __FILE__, __LINE__);
    }
}

/**
 * @brief BLE connection callback
 *
 * @param[in] connId Connection ID
 * @param[in] connected Connection status
 *
 * @retval None
 */
void ble_conn_callback(uint16_t connId, bool connected, uint16_t disconnectReason)
{
    if (connected) {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("Client connected");
        QS_END()
        // print_bd_address(p_conn_status->bd_addr);
        print_connection_id(connId);
        /* Store the connection ID */
        client_id = connId;
        ble_adv_conn_state = BLE_ADV_OFF_CONN_ON;
    } else {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("Client disconnected, Reason:");
            QS_U8(0, disconnectReason);
        QS_END()

        /** Invalidate the connection id to indicate disconnected state */
        client_id = BLE_INVALID_CONN_ID;

        /** Restart the advertisements */
        HAL_BLE_status_t advStatus = HAL_BLE_startAdvertisement(NULL, NULL);
        HAL_ASSERT((advStatus == HAL_BLE_SUCCESS), __FILE__, __LINE__);
        ble_adv_conn_state = BLE_ADV_ON_CONN_OFF;
    }
}

/**
 * @brief BLE Advertising State Callback
 *
 * @param[in] advState Advertising state
 *
 * @retval None
 */
void ble_adv_state_callback(HAL_BLE_adv_state_t advState)
{
    switch (advState)
    {
        case HAL_BLE_ADV_STATE_OFF:
        {
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Advertising State: OFF");
            QS_END();
            if(0 == client_id) {
                ble_adv_conn_state = BLE_ADV_OFF_CONN_OFF;
            } else {
                ble_adv_conn_state = BLE_ADV_OFF_CONN_ON;
            }
            break;
        }

        case HAL_BLE_ADV_STATE_ON:
        {
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Advertising State: ON");
            QS_END();
            ble_adv_conn_state = BLE_ADV_ON_CONN_OFF;
            break;
        }

        default:
        {
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Advertising State: UNKNOWN");
            QS_END();
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
        }
    }
}

/**
 * @brief BLE MTU changed callback
 *
 * @param[in] mtu The new MTU size
 *
 * @note Resulted MTU size will be the minimum of requested and preferred MTU sizes 
 *       (applicable for PSOC63 port). For QN9080 resulted MTU is reported in a separate event.
 *
 * @retval None
 */
void ble_mtu_changed_callback(uint16_t mtu)
{
    QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
        QS_STR("BLE MTU Changed:");
        QS_U16(0, mtu);
    QS_END()
}

// ============================
/// Misc spare functions
// ============================
/**
 * Print the Connection ID
 * @param[in] id Connection ID to print
 * @note This function is used to print the connection ID in the debug output.
 * 
 * @return None
 */
static void print_connection_id(uint16_t id)
{
    QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
        QS_STR("Connection Id: ");
        QS_U16(0, id);
    QS_END()
}

/* [] END OF FILE */
