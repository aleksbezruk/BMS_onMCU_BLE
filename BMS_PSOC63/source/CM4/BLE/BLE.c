/**
 * @file  BLE.c
 *
 * @brief BLE module functions definition
 *
 * @details  ## **Details**
 *           ### 1.Useful links: <br>
 *                - https://documentation.infineon.com/html/psoc6/moa1717991724927.html#moa1717991724927 
 *                - https://github.com/Infineon/btstack 
 *                - https://github.com/Infineon/btstack-integration <br>
 *
 *           ### 2. BLE stack details: <br>
 *               2.1 Infineon/Cypress stack is used -> AIROC™ BTSTACK <br>
 *               2.2 Bluetooth® porting layer -> <br>
 *                   AIROC™ BTSTACK requires a porting layer specific to the device it is running on. <br>
 *               2.3 Porting layer for Infineon Bluetooth® devices <br>
 *                   is hosted on GitHub as a library called btstack-integration <br>
 *               2.4 Cypress/Infineon BLE stack requires OS layer based on RTOS. <br>
 *                   Porting/integration layer of btstack is based oo a FreeRTOS package. <br>
 *                   The FreeRTOS package is delivered by Cypress and their BLE stack <br>
 *                   looks like is tested on FreeRTOS, so FreeRTOS also will be used <br>
 *                   in the current project . <br>
 *           
 *             ### 3. BLE stack & FreeRTOS libraries/packages config <br>
 *                 3.1 Run library manager on 'BMS_PSOC63' folder: <br>
 *                     $ make library-manager <br>
 *                 3.2 Select library: btstack-integration <br>
 *                 3.3 Select library: btstack <br>
 *                 3.4 Select library: freertos <br>
 *
 *             ### 4. BLE stack architecture    <br>
 *                 4.1 AIROC™ BTSTACK runs on the CM4 core, <br> 
 *                     and the controller stack runs on the CM0+ core . <br>
 *                 4.2 A hardware block called inter-processor communication (IPC) <br> 
 *                     is used as HCI (Host-controller interface) . <br>
 *                     Therefore, the application uses the component called BLESS-IPC in btstack-integration. <br>
 *                 4.3 CM0P_BLESS component for CM0p build includes the prebuild image <br>
 *                     'COMPONENT_CM0P_BLESS\psoc6cm0p_bless.bin' that is a part of MTB package <br> 
 *
 *              ### 5. BLE profile for BMS implementation details <br>
 *                  5.1 Use ModusToolbox BLE configurator: <br>
 *                      5.1.1 $ make bt-configurator <br>
 *                      Note: Because PSoC™ 6 Bluetooth® LE device <br>
 *                          no longer supports BLESS middleware from MTB version 3.0 <br> 
 *                          and uses AIROC™ BTSTACK, use the option ‘AIROC™ BTSTACK with <br>
 *                          Bluetooth® LE only’ while creating a new configuration file. <br>
 *                      5.1.2 Makefile config : <br>
 *                          cm4 -> COMPONENTS=FREERTOS WICED_BLE CM0P_BLESS <br>
 *                          The two components FREERTOS and WICED_BLE are required <br> 
 *                          to include the files from FreeRTOS and btstack libraries for compilation. <br>
 *                          DEFINES=CY_RTOS_AWARE <br>
 *                          CY_RTOS_AWARE must be defined to inform the HAL <br> 
 *                          that an RTOS environment is being used. <br>
 *                  5.2 GAP settings (via bt-configurator) <br>  
 *                      5.2.1 max remote clients = 1 <br>
 *                      5.2.2 GAP role = Peripheral <br>
 *                      5.2.3 Set advertising data (full name etc.) <br>
 *                      5.2.4 Set scan response data (TX power lvl) <br>
 *                  5.3 GATT settings (via bt-configurator) <br> 
 *                      5.3.1 Starting from simple Battery Service (BAS) <br>
 *                      5.3.2 TODO: Add standard SIG or custom service that will satisfy BMS use case <br>
 *                  5.4 Stack initialization  <br>
 *                      Note: BLE Controller is implemented as part of prebuild image <br>
 *                      'psoc6cm0p_bless.bin' as described above. So according to <br>
 *                      the Cypress approach there is no need to implement  <br>
 *                      BLE user code on CM0p side . <br>
 *                      5.4.1 wiced_bt_stack_platform_t -> cybt_platform_config_init(): <br>
 *                          config platform before BLE stack init <br>
 *                      5.4.2 wiced_bt_cfg_settings_t -> wiced_bt_stack_init(): <br>
 *                      - cybt_platform_init() ;
 *                          - cybt_platform_stack_timer_init() :
 *                              - cy_rtos_init_timer() -> <br>
 *                                  init RTOS timer handled from TmrSvc task (RTOS daemon task) ; <br>
 *                      - register app management callback ; 
 *                      - host_stack_platform_interface_init() ; 
 *                      - wiced_bt_set_stack_config() ;
 *                      - cybt_platform_task_init() :
 *                          - cybt_bttask_init() -> <br>
 *                              Init RTOS 'bt_task' (bt_task_handler). Prio -> CY_RTOS_PRIORITY_HIGH . <br>
 *                  5.5 BLE RTOS threads & BLE LL IRQ config <br>
 *                      5.5.1 RTOS tasks -> <br>
 *                          For details see wiced_bt_stack_init() -> cybt_bttask_init() <br>
 *                      5.5.2 BLE LL interrupt <br>
 *                          - #define CY_BLE_IRQ bless_interrupt_IRQn ;
 *                          - bless_interrupt_IRQHandler ;
 *                          - Looks like the implementation of the IRQ handler is hidden inside CMOp prebuild image . <br>
 *                   5.6 Application callbacks <br> 
 *                      TODO: inplement APP callbacks <br> 
 *
 *               ### 6. BLE security <br>
 *                      6.1 For the first revisions the security feature is disabled <br>
 *                      6.2 "Enable RPA timeout" -> false
 * 
 * @version 0.1.0
 */

#include "BLE.h"
#include "cybsp.h"
#include "qspyHelper.h"
#include "cycfg_bt_settings.h"
#include "cycfg_gap.h"

// RTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"

///////////////////////
// Defines
///////////////////////
#define BLE_TASK_STACK_SIZE 560U   /**< bytes, aligned to 8 bytes */
#define BLE_QUEUE_SIZE 3U

///////////////////////
// Functions prototype
///////////////////////
static wiced_result_t app_bt_management_callback_(
    wiced_bt_management_evt_t event,
    wiced_bt_management_evt_data_t *p_event_data
);
static void print_bd_address(uint8_t* bda);
static void le_app_init(void);
static void bleTask_(cy_thread_arg_t arg);
static void parseQueueItem_(Ble_queue_data_t* queueItem);
static wiced_bt_gatt_status_t le_app_connect_handler(wiced_bt_gatt_connection_status_t *p_conn_status);
static wiced_bt_gatt_status_t le_app_gatt_event_callback(wiced_bt_gatt_evt_t event,
                                                         wiced_bt_gatt_event_data_t *p_event_data);
static void print_connection_id(uint16_t id);

///////////////////////
// Private data
///////////////////////
static cy_thread_t bleTaskHandle_;
static StaticQueue_t staticQueueHandle;
static cy_queue_t bleTaskQueueHandle;
static Ble_queue_data_t bleQueueSto[BLE_QUEUE_SIZE];
/** 
 *  In stack words because stack pointer should be aligned to 
 *  8 bytes boundaru per the RTOS requirements.
 */
static uint64_t bleTaskStack_[BLE_TASK_STACK_SIZE/8U];

///////////////////////
// Code
///////////////////////

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
    wiced_result_t wiced_result;
    cy_rslt_t result;

    /** Configure platform specific settings for the BT device */
    cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /* Register call back and configuration with stack */
    wiced_result = wiced_bt_stack_init(
        app_bt_management_callback_, 
        &wiced_bt_cfg_settings
    );
    if (WICED_BT_SUCCESS == wiced_result) {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("Bluetooth Stack Initialization Successful"); 
        QS_END()
        QS_FLUSH();
    }
    else {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("Bluetooth Stack Initialization failed!!"); 
        QS_END()
        QS_FLUSH();
        status = BLE_STATUS_FAIL;
    }

    /** Create RTOS task */
    if (status == BLE_STATUS_OK) {
        result = cy_rtos_thread_create(
            &bleTaskHandle_,
            bleTask_,
            "bleTask",
            bleTaskStack_,             // should be aligned to 8 bytes
            BLE_TASK_STACK_SIZE,       // in bytes
            CY_RTOS_PRIORITY_NORMAL,   // prio
            NULL                       // no args
        );
        if (result != CY_RSLT_SUCCESS) {
            status = BLE_STATUS_FAIL;
        }
    }

    return status;
}

/**
 * @brief Start BLE advertisement
 * 
 * @param[in] periodic_adv_int_min - minimal advertising interval 
 *            Range N: 0x0006 to 0xFFFF, Time = N * 1.25 ms
 * 
 * @param[in] periodic_adv_int_max - miximum advertising interval 
 *            Range N: 0x0006 to 0xFFFF, Time = N * 1.25 ms
 * 
 * @param[in] periodic_adv_properties - Periodic adv property 
 *            indicates which field should be include in periodic adv
 *            See \ref WICED_BT_BLE_PERIODIC_ADV_PROPERTY_INCLUDE_TX_POWER
 * 
 * @note     btm_ble_adv_scan_functions        Advertisement & Scan
 * @ingroup  btm_ble_api_functions
 * 
 * @retval See \ref wiced_bt_dev_status_t
 * 
 */
wiced_bt_dev_status_t BLE_startAdvertisement(
    uint16_t periodic_adv_int_min, 
    uint16_t periodic_adv_int_max, 
    wiced_bt_ble_periodic_adv_prop_t periodic_adv_properties
)
{
    wiced_bt_dev_status_t status = WICED_BT_SUCCESS;

    /** The AOI isn't supported, because BLE stack returns 'WICED_BT_UNSUPPORTED' */
    // status = wiced_bt_ble_set_periodic_adv_params(
    //     0xFF, //adv_handle -> Not used
    //     periodic_adv_int_min,
    //     periodic_adv_int_max,
    //     periodic_adv_properties // wiced_bt_ble_periodic_adv_prop_e
    // );

    if (WICED_BT_SUCCESS == status) {
        // QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
        //     QS_STR("Set advParam successfully");
        // QS_END()

        /** Set Advertisement Data */
        wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, cy_bt_adv_packet_data);
        /** 
         * Start Undirected LE Advertisements on device startup.
         * The corresponding parameters are contained in 'app_bt_cfg.c' 
         */
        status = wiced_bt_start_advertisements(
            BTM_BLE_ADVERT_UNDIRECTED_HIGH, 
            BLE_ADDR_PUBLIC, 
            NULL
        );
        if (WICED_BT_SUCCESS != status) {
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Start Adv failed:");
                QS_U8(0, status);
            QS_END()
        } else {
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Start Adv successfully");
            QS_END()
        }
    } else {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("Set advParam failed:");
            QS_U8(0, status);
        QS_END()
    }

    return status;
}

/**
 * @brief Stop BLE advertisement
 * 
 * @param None
 * 
 * @retval See \ref wiced_bt_dev_status_t
 * 
 */
wiced_bt_dev_status_t BLE_stopAdvertisement(void)
{
    wiced_bt_dev_status_t status;

    status = wiced_bt_start_advertisements(
        BTM_BLE_ADVERT_OFF, 
        BLE_ADDR_PUBLIC, 
        NULL
    );

    if (WICED_BT_SUCCESS == status) {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("Stop Adv successfully");
        QS_END()
    } else {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("Stop Adv failed:");
            QS_U8(0, status);
        QS_END()
    }

    return status;
}

/**
 * @brief This is a Bluetooth stack event handler function to receive management events from
 *        the LE stack and process as per the application.
 * 
 * @param[in] event LE event code of one byte length
 * 
 * @param[in] p_event_data Pointer to LE management event structures
 * 
 * @retval See \ref wiced_result_t : Error code from WICED_RESULT_LIST or BT_RESULT_LIST
 * 
 */
wiced_result_t app_bt_management_callback_(
    wiced_bt_management_evt_t event,
    wiced_bt_management_evt_data_t *p_event_data
)
{
    wiced_result_t wiced_result = WICED_BT_SUCCESS;
    wiced_bt_device_address_t bda = { 0 };
    wiced_bt_ble_advert_mode_t *p_adv_mode = NULL;

    switch (event)
    {
        case BTM_ENABLED_EVT:
        {
            /* Bluetooth Controller and Host Stack Enabled */
            if (WICED_BT_SUCCESS == p_event_data->enabled.status) {
                wiced_bt_set_local_bdaddr((uint8_t *)cy_bt_device_address, BLE_ADDR_PUBLIC);
                wiced_bt_dev_read_local_addr(bda);
                print_bd_address(bda);

                /* Perform application-specific initialization */
                le_app_init();
            }
            else {
                QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                    QS_STR("Bluetooth Disabled"); 
                QS_END()
            }

            break;
        }
        case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
        {
            /* Advertisement State Changed */
            p_adv_mode = &p_event_data->ble_advert_state_changed;

            if (BTM_BLE_ADVERT_OFF == *p_adv_mode) {
                QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                    QS_STR("Advertisement stopped"); 
                QS_END()
            }
            else {
                QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                    QS_STR("Advertisement started"); 
                QS_END()
            }
            break;
        }
        case BTM_BLE_CONNECTION_PARAM_UPDATE:
        {
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Conn params upd sts: "); 
                QS_U8(0, p_event_data->ble_connection_param_update.status);
                QS_STR("ConnInt: ");
                QS_U16(0, p_event_data->ble_connection_param_update.conn_interval);
                QS_STR("ConnLat: ");
                QS_U16(0, p_event_data->ble_connection_param_update.conn_latency);
                QS_STR("Timeout: ");
                QS_U16(0, p_event_data->ble_connection_param_update.supervision_timeout);
            QS_END()
            break;
        }
        default:
        {
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Unhandled Bluetooth Management Event:"); 
                QS_U8(0, event);
            QS_END()
            break;
        }   
    }

    return wiced_result;
}

/**************************************************************************************************
* Function Name: le_app_gatt_event_callback
***************************************************************************************************
* Summary:
*   This function handles GATT events from the BT stack.
*
* Parameters:
*   wiced_bt_gatt_evt_t event                   : LE GATT event code of one byte length
*   wiced_bt_gatt_event_data_t *p_event_data    : Pointer to LE GATT event structures
*
* Return:
*  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
*
**************************************************************************************************/
static wiced_bt_gatt_status_t le_app_gatt_event_callback(wiced_bt_gatt_evt_t event,
                                                         wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;
    // wiced_bt_gatt_attribute_request_t *p_attr_req = &p_event_data->attribute_request;

    // uint16_t error_handle = 0;
    /* Call the appropriate callback function based on the GATT event type, and pass the relevant event
     * parameters to the callback function */
    switch ( event )
    {
        case GATT_CONNECTION_STATUS_EVT:
            gatt_status = le_app_connect_handler( &p_event_data->connection_status );
            break;

        /** @todo Popu;ate the callback with implementation for GATT events */

        default:
            break;
    }

    return gatt_status;
}

/**************************************************************************************************
* Function Name: le_app_connect_handler
***************************************************************************************************
* Summary:
*   This callback function handles connection status changes.
*
* Parameters:
*   wiced_bt_gatt_connection_status_t *p_conn_status  : Pointer to data that has connection details
*
* Return:
*  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
*
**************************************************************************************************/
static wiced_bt_gatt_status_t le_app_connect_handler(wiced_bt_gatt_connection_status_t *p_conn_status)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS ;

    if ( NULL != p_conn_status )
    {
        if ( p_conn_status->connected )
        {
            /* Device has connected */
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Client connected");
            QS_END()
            print_bd_address(p_conn_status->bd_addr);
            print_connection_id(p_conn_status->conn_id);
        }
        else
        {
            /* Device has disconnected */
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Client disconnected");
            QS_END()
            QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
                QS_STR("Reason:");
                QS_U8(0, p_conn_status->reason);
            QS_END()

            /* Restart the advertisements */
            wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, 0, NULL);
        }
    } else {
        gatt_status = WICED_BT_GATT_ERROR;
    }

    return gatt_status;
}

/**
 * @brief This function handles application level initialization tasks and is called from the BT
 *        management callback once the LE stack enabled event (BTM_ENABLED_EVT) is triggered
 *   This function is executed in the BTM_ENABLED_EVT management callback.
 * 
 * @param None
 * 
 * @retval None
 */
static void le_app_init(void)
{
    wiced_result_t wiced_result;
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;

    /** 
     * Paring & bonding (link encryption) isn't supported for now 
     *
     * TODO: add authontication & link encryption in future 
     */
    wiced_bt_set_pairable_mode(FALSE, FALSE);

    /** Set Advertisement Data */
    wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, cy_bt_adv_packet_data);

    /* Register with BT stack to receive GATT callback */
    gatt_status = wiced_bt_gatt_register(le_app_gatt_event_callback);
    QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
        QS_STR("GATT event Handler registration status:");
        QS_U16(0, gatt_status);
    QS_END()

    /** 
     * Start Undirected LE Advertisements on device startup.
     * The corresponding parameters are contained in 'app_bt_cfg.c' 
     */
    wiced_result = wiced_bt_start_advertisements(
        BTM_BLE_ADVERT_UNDIRECTED_HIGH, 
        BLE_ADDR_PUBLIC, 
        NULL
    );

    /** 
     * IF Failed to start advertisement. Stop program execution 
     */
    if (WICED_BT_SUCCESS != wiced_result) {
        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("Failed to start advertisement!"); 
        QS_END()
        QS_FLUSH();

        CY_ASSERT(0);
    }
}

/**
 * @brief BLE task's handler
 * 
 * @param[in] arg the argument passed from the thread create call to the entry function
 * 
 * @retval None
 */
static void bleTask_(cy_thread_arg_t arg)
{
    (void) arg;
    cy_rslt_t result;
    Ble_queue_data_t queueItem;

    /** Create an event Queue */
    bleTaskQueueHandle = xQueueCreateStatic(BLE_QUEUE_SIZE,
                                     sizeof(Ble_queue_data_t),
                                     (uint8_t *) bleQueueSto,
                                     &staticQueueHandle);
    if (bleTaskQueueHandle == NULL) {
        CY_ASSERT(0);
    }

    /** Events processing loop */
    while(1) {
        /** Wait for event */
        result = cy_rtos_queue_get(&bleTaskQueueHandle,
                                   &queueItem,
                                   CY_RTOS_NEVER_TIMEOUT);
        if (result != CY_RSLT_SUCCESS) {
            CY_ASSERT(0);
        }

        /** Handle an evt */
        parseQueueItem_(&queueItem);

        QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
            QS_STR("BLE task handle evt:");
            QS_U8(0, queueItem.evtType);
        QS_END()
    }
}

/////////////////////////
/// Queue functions/APIs
/////////////////////////
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
    CY_ASSERT((evt != NULL) && (eventType < EVT_TYPE_MAX));

    Ble_queue_data_t queueItem;

    queueItem.evtType = eventType;
    memcpy((uint8_t*) &queueItem.evtData, (uint8_t*) evt, sizeof(Ble_evt_t));

    cy_rslt_t result = cy_rtos_queue_put(&bleTaskQueueHandle,
                                         &queueItem,
                                         CY_RTOS_NEVER_TIMEOUT);
    if (result != CY_RSLT_SUCCESS) {
        CY_ASSERT(0);
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
            BLE_startAdvertisement(
                queueItem->evtData.advData.periodicAdvIntMin,
                queueItem->evtData.advData.periodicAdvIntMax,
                queueItem->evtData.advData.periodicAdvProp
            );
            break;
        }

        case EVT_BLE_ADV_OFF:
        {
            BLE_stopAdvertisement();
            break;
        }

        default:
            CY_ASSERT(0);   // unexpected evt
            break;
    }
}

/////////////////////////
/// Misc spare functions
/////////////////////////
static void print_bd_address(uint8_t* bda)
{
    QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
        QS_STR("Local Bluetooth Address: ");
        QS_MEM(bda, BD_ADDR_LEN);
    QS_END()
}

static void print_connection_id(uint16_t id)
{
    QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
        QS_STR("Connection Id: ");
        QS_U16(0, id);
    QS_END()
}

/* [] END OF FILE */
