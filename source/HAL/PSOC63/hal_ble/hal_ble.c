/**
 * @file hal_ble.c
 * @brief Source file for BLE (Bluetooth Low Energy) HAL (Hardware Abstraction Layer) functions.
 * @version 0.6.0
 * @note PSOC63 specific implementation
 * 
 *  @details  ## **Details**
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
 *                      5.3.2 Add standard SIG or custom service that will satisfy BMS use case -> AIOS <br>
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
 *                      Inplement APP callbacks - advertising, connections, GATT etc. <br> 
 *
 *               ### 6. BLE security <br>
 *                      6.1 For the first revisions the security feature is disabled <br>
 *                      6.2 "Enable RPA timeout" -> false
 */

// Cypress SDK includes 
#include "cybsp.h"
#include "cycfg_bt_settings.h"
#include "cycfg_gap.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"

// HAL includes
#include "hal.h"
#include "hal_ble.h"

// BLE services
#include "BatteryService.h"
#include "AIOService.h"

// =======================
// Functions prototype
// =======================
static wiced_result_t app_bt_management_callback_(
    wiced_bt_management_evt_t event,
    wiced_bt_management_evt_data_t *p_event_data
);
static void print_bd_address(uint8_t* bda);
static void le_app_init(void);
static wiced_bt_gatt_status_t le_app_gatt_event_callback(wiced_bt_gatt_evt_t event,
                                                         wiced_bt_gatt_event_data_t *p_event_data);
static wiced_bt_gatt_status_t le_app_connect_handler(wiced_bt_gatt_connection_status_t *p_conn_status);
static wiced_bt_gatt_status_t le_app_server_handler (wiced_bt_gatt_attribute_request_t *p_attr_req,
                                                     uint16_t *p_error_handle);
static wiced_bt_gatt_status_t le_app_read_handler( uint16_t conn_id,
                                                   wiced_bt_gatt_opcode_t opcode,
                                                   wiced_bt_gatt_read_t *p_read_req,
                                                   uint16_t len_req,
                                                   uint16_t *p_error_handle);
static wiced_bt_gatt_status_t le_app_write_handler( uint16_t conn_id,
                                                    wiced_bt_gatt_opcode_t opcode,
                                                    wiced_bt_gatt_write_req_t *p_write_req,
                                                    uint16_t len_req,
                                                    uint16_t *p_error_handle);
static wiced_bt_gatt_status_t le_app_set_value( uint16_t conn_id,
                                                uint16_t attr_handle,
                                                uint8_t *p_val,
                                                uint16_t len);
static gatt_db_lookup_table_t  *le_app_find_by_handle(uint16_t handle);
// ===================
// Private data
// ===================
/**
 * @note Default advertising and scan response parameters can be updated via HAL_BLE_startAdvertisement().
 */
static uint8_t adv_packet_flags[1] = { HAL_BLE_AD_FLAG_GENERAL_DISCOVERABLE | HAL_BLE_AD_FLAG_BLE_BREDR_NOT_SUPPORTED };
static uint8_t adv_packet_name[HAL_BLE_LOCAL_NAME_MAX_LEN] = { 0x42, 0x4D, 0x53, 0x5F, 0x4D, 0x43, 0x55 };    // "BMS_MCU"
static uint8_t adv_packet_uuid[4] = { 0x0F, 0x18, 0x15, 0x18 };
static uint8_t adv_packet_service_data[3] = { 0x0F, 0x18, 0x64 };
static wiced_bt_ble_advert_elem_t adv_packet_data[] = 
{
    /* Flags */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_FLAG, 
        .len = 1, 
        .p_data = (uint8_t*) adv_packet_flags, 
    },
    /* Complete local name */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE, 
        .len = 7, 
        .p_data = (uint8_t*) adv_packet_name, 
    },
    /* Complete list of 16-bit UUIDs available */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_16SRV_COMPLETE, 
        .len = 4, 
        .p_data = (uint8_t*) adv_packet_uuid, 
    },
    /* 16-bit Service Data */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_SERVICE_DATA, 
        .len = 3, 
        .p_data = (uint8_t*) adv_packet_service_data, 
    },
};
static uint8_t scan_resp_packet[1] = { 0x00 };
wiced_bt_ble_advert_elem_t scan_resp_packet_data[] = 
{
    /* TX Power Level  */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_TX_POWER, 
        .len = 1, 
        .p_data = (uint8_t*)scan_resp_packet, 
    },
};

/*! Variables for BLE callbacks */
static volatile HAL_BLE_conn_callback_t ble_connection_callback = NULL;
static volatile HAL_BLE_init_callback_t ble_stack_init_callback = NULL;
static volatile HAL_BLE_advState_callback_t ble_advertising_state_changed_callback = NULL;
static volatile HAL_BLE_mtuChanged_callback_t ble_mtu_changed_callback = NULL;

// ===================
// Code
// ===================
/**
 * @brief Initialize BLE stack
 * @param None
 * @note If initialization fails, assert to halt the system.
 * @return None
 */
void HAL_BLE_init(void) 
{
    wiced_result_t wiced_result;

    /** Configure platform specific settings for the BT device */
    cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /* Register call back and configuration with stack */
    wiced_result = wiced_bt_stack_init(
        app_bt_management_callback_, 
        &wiced_bt_cfg_settings
    );
    if (WICED_BT_SUCCESS == wiced_result) {
        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("Bluetooth Stack Initialization Successful"); 
        QS_END()
        QS_FLUSH();
    }
    else {
        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("Bluetooth Stack Initialization failed!!"); 
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__);
    }
}

// **********************************
// Advertising Functions
// **********************************

/**
 * @brief Start BLE advertisement
 * @param[in] advData Pointer to advertisement data. 
 *            If advData=NULL, then advertising data isn't updated.
 * @param[in] scanResponseData Pointer to scan response data.
 *            If scanResponseData=NULL, then scan response data isn't updated.
 * @note Default advertising and scan response parameters are defined in GeneratedSource/cycfg_gap.c.
 *       These defaults are set during the BLE stack initialization.
 * @note The API wiced_bt_ble_set_periodic_adv_params() is not supported, 
 *       because BLE stack returns 'WICED_BT_UNSUPPORTED'.
 * @return Status of the operation \ref HAL_BLE_status_t
 */
HAL_BLE_status_t HAL_BLE_startAdvertisement(HAL_BLE_adv_data_t *advData, HAL_BLE_scan_response_data_t *scanResponseData)
{
    HAL_BLE_status_t bleStatus = HAL_BLE_SUCCESS;
    wiced_bt_dev_status_t status = WICED_BT_SUCCESS;

    /** Set Advertisement Data */
    if (advData != NULL) {
        /** Set Flags */
        adv_packet_flags[0] = (uint8_t) advData->flags;
        /** Calculate length of the char string advData->local_name (null terminated char isn't counted) */
        uint8_t name_len = strlen((char*)advData->local_name);
        HAL_ASSERT((name_len < HAL_BLE_LOCAL_NAME_MAX_LEN), __FILE__, __LINE__);
        adv_packet_data[1].len = name_len;
        memcpy((uint8_t *)adv_packet_name, (uint8_t *)advData->local_name, name_len);
        /** Update services list */
        HAL_ASSERT((advData->services > HAL_BLE_AD_SERVICE_NONE), __FILE__, __LINE__);
        if (advData->services == HAL_BLE_AD_SERVICE_BAS) {
           adv_packet_uuid[0] = 0x0F;
           adv_packet_uuid[1] = 0x18;
           adv_packet_data[2].len = 2U;
        } else if (advData->services == HAL_BLE_AD_SERVICE_AIOS) {
            adv_packet_uuid[0] = 0x15;
            adv_packet_uuid[1] = 0x18;
            adv_packet_data[2].len = 2U;
        } else {
            // Both included
            adv_packet_data[2].len = 4U;
            adv_packet_uuid[0] = 0x0F;
            adv_packet_uuid[1] = 0x18;
            adv_packet_uuid[2] = 0x15;
            adv_packet_uuid[3] = 0x18;
        }
        /** Update service data */
        adv_packet_service_data[2] = advData->service_data;
        /** Set advertising data */
        status = wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, adv_packet_data);
        bleStatus = (status == WICED_BT_SUCCESS) ? HAL_BLE_SUCCESS : HAL_BLE_ERROR;
    }

    /** Set Scan Response Data */
    if (bleStatus == HAL_BLE_SUCCESS) {
        if (scanResponseData != NULL) {
            /** Set data */
            scan_resp_packet[0] = *scanResponseData;
            /** Update scan response data */
            status = wiced_bt_ble_set_raw_scan_response_data(1U, scan_resp_packet_data);
            bleStatus = (status == WICED_BT_SUCCESS) ? HAL_BLE_SUCCESS : HAL_BLE_ERROR;
        }
    } else {
        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("Set Advertising Data failed:");
            QS_U8(0, status);
        QS_END()
    }

    /** Start Undirected LE Advertisements on device startup.
     * The corresponding parameters are contained in 'app_bt_cfg.c'
     */
    if (bleStatus == HAL_BLE_SUCCESS) {
        status = wiced_bt_start_advertisements(
            BTM_BLE_ADVERT_UNDIRECTED_HIGH, 
            BLE_ADDR_PUBLIC, 
            NULL
        );
        if (WICED_BT_SUCCESS != status) {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("Start Adv failed:");
                QS_U8(0, status);
            QS_END()
            bleStatus = HAL_BLE_ERROR;
        } else {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("Start Adv successfully");
            QS_END()
        }
    } else {
        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("Set Scan Response Data failed:");
            QS_U8(0, status);
        QS_END()
    }

    return bleStatus;
}

/**
 * @brief Stop BLE advertisement
 *
 * @param None
 *
 * @retval See \ref HAL_BLE_status_t
 *
 */
HAL_BLE_status_t HAL_BLE_stopAdvertisement(void)
{
    HAL_BLE_status_t bleStatus = HAL_BLE_SUCCESS;

    wiced_bt_dev_status_t status = wiced_bt_start_advertisements(
        BTM_BLE_ADVERT_OFF, 
        BLE_ADDR_PUBLIC, 
        NULL
    );

    if (WICED_BT_SUCCESS == status) {
        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("Stop Adv successfully");
        QS_END()
    } else {
        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("Stop Adv failed:");
            QS_U8(0, status);
        QS_END()
        bleStatus = HAL_BLE_ERROR;
    }

    return bleStatus;
}

/**
 * @brief Update BLE advertising data
 *
 * @note For now, only the battery level is updated
 *
 * @param[in] advData The new advertising data
 *
 * @retval See \ref HAL_BLE_status_t
 */
HAL_BLE_status_t HAL_BLE_updateAdvertisingData(HAL_BLE_adServiceData_t advData)
{
    /** Update battery level in advertisement data */
    adv_packet_service_data[2] = advData;
    /** Call BLE stack to update advertisement data */
    wiced_bt_dev_status_t status = wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, adv_packet_data);
    return (status == WICED_BT_SUCCESS) ? HAL_BLE_SUCCESS : HAL_BLE_ERROR;
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
     * Pairing & bonding (link encryption) isn't supported for now
     *
     * TODO: add authentication & link encryption in future releases
     */
    wiced_bt_set_pairable_mode(FALSE, FALSE);

    /** Set Advertisement Data */
    wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, cy_bt_adv_packet_data);

    /** Register with BT stack to receive GATT callback */
    gatt_status = wiced_bt_gatt_register(le_app_gatt_event_callback);
    QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
        QS_STR("GATT event Handler registration status:");
        QS_U16(0, gatt_status);
    QS_END()

    /** Initialize GATT Database */
    gatt_status = wiced_bt_gatt_db_init(gatt_database, gatt_database_len, NULL);
    QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
        QS_STR("GATT database initialization status:");
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
        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("Failed to start advertisement!"); 
        QS_END()
        QS_FLUSH();

        HAL_ASSERT(0, __FILE__, __LINE__);
    }
}

// **********************************
// Register callbacks APIs
// **********************************/

/**
 * @brief Register a callback function to be called on BLE connection events
 *
 * @param[in] callback The callback function to register
 *
 * @retval None
 */
void HAL_BLE_registerConnectionCallback(HAL_BLE_conn_callback_t callback)
{
    HAL_ASSERT((callback != NULL), __FILE__, __LINE__);
    ble_connection_callback = callback;
}

/**
 * @brief Register a callback function to be called on BLE stack initialization events
 *
 * @param[in] callback The callback function to register
 *
 * @retval None
 */
void HAL_BLE_registerStackInitCallback(HAL_BLE_init_callback_t callback)
{
    HAL_ASSERT((callback != NULL), __FILE__, __LINE__);
    ble_stack_init_callback = callback;
}

/**
 * @brief Register a callback function to be called on BLE advertising state changed events
 *
 * @param[in] callback The callback function to register
 *
 * @retval None
 */
void HAL_BLE_registerAdvertisingStateChangedCallback(HAL_BLE_advState_callback_t callback)
{
    HAL_ASSERT((callback != NULL), __FILE__, __LINE__);
    ble_advertising_state_changed_callback = callback;
}

/**
 * @brief Register a callback function to be called on BLE MTU changed events
 *
 * @param[in] callback The callback function to register
 *
 * @retval None
 */
void HAL_BLE_registerMtuChangedCallback(HAL_BLE_mtuChanged_callback_t callback)
{
    HAL_ASSERT((callback != NULL), __FILE__, __LINE__);
    ble_mtu_changed_callback = callback;
}

// ====================
// GATT APIs
// ====================
/**
 * @brief Update a BLE attribute
 *
 * @param[in] attr Pointer to the attribute structure
 *
 * @retval None
 */
void HAL_BLE_updateAttribute(HAL_BLE_attribute_t *attr)
{
    HAL_ASSERT((attr != NULL), __FILE__, __LINE__);

    // Update the attribute in the GATT database
    switch (attr->attribute) {
        case HAL_BLE_ATTR_BATTERY_LEVEL:
        {
            // Update battery level characteristic
            uint8_t* pBasValDB = app_gatt_db_ext_attr_tbl[2].p_data;
            __disable_irq();    // short critical section to avoid race condition with BLE stack
            pBasValDB[0] = *(uint8_t *)attr->p_value;
            __enable_irq();
            break;
        }

        case HAL_BLE_ATTR_AIOS_FULLBAT_VALUE:
        {
            uint8_t* pFullBatValDB;
            __disable_irq();   // short critical section to avoid race condition with BLE stack
            pFullBatValDB = app_gatt_db_ext_attr_tbl[8].p_data;
            memcpy(pFullBatValDB, (uint8_t *)attr->p_value, attr->length);
            __enable_irq();
            break;
        }

        case HAL_BLE_ATTR_AIOS_BANK1_VALUE:
        {
            uint8_t* pBank1ValDB;
            __disable_irq();   // short critical section to avoid race condition with BLE stack
            pBank1ValDB = app_gatt_db_ext_attr_tbl[10].p_data;
            memcpy(pBank1ValDB, (uint8_t *)attr->p_value, attr->length);
            __enable_irq();
            break;
        }

        case HAL_BLE_ATTR_AIOS_BANK2_VALUE:
        {
            uint8_t* pBank2ValDB;
            __disable_irq();   // short critical section to avoid race condition with BLE stack
            pBank2ValDB = app_gatt_db_ext_attr_tbl[12].p_data;
            memcpy(pBank2ValDB, (uint8_t *)attr->p_value, attr->length);
            __enable_irq();
            break;
        }

        case HAL_BLE_ATTR_AIOS_BANK3_VALUE:
        {
            uint8_t* pBank3ValDB;
            __disable_irq();   // short critical section to avoid race condition with BLE stack
            pBank3ValDB = app_gatt_db_ext_attr_tbl[14].p_data;
            memcpy(pBank3ValDB, (uint8_t *)attr->p_value, attr->length);
            __enable_irq();
            break;
        }

        case HAL_BLE_ATTR_AIOS_BANK4_VALUE:
        {
            uint8_t* pBank4ValDB;
            __disable_irq();   // short critical section to avoid race condition with BLE stack
            pBank4ValDB = app_gatt_db_ext_attr_tbl[16].p_data;
            memcpy(pBank4ValDB, (uint8_t *)attr->p_value, attr->length);
            __enable_irq();
            break;
        }

        case HAL_BLE_ATTR_AIOS_DIGITAL_IO_VALUE:
        {
            uint8_t* pDigitalIOValDB;
            __disable_irq();   // short critical section to avoid race condition with BLE stack
            pDigitalIOValDB = app_gatt_db_ext_attr_tbl[5].p_data;
            memcpy(pDigitalIOValDB, (uint8_t *)attr->p_value, attr->length);
            __enable_irq();
            break;
        }

        default:
        {
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
        }
    }
}

/**
 * @brief Send a notification to a BLE client
 *
 * @param[in] attr Pointer to the attribute to notify
 * @param[in] conn_id Connection ID of the BLE client
 *
 * @retval See \ref HAL_BLE_status_t
 */
HAL_BLE_status_t HAL_BLE_send_notif(HAL_BLE_attribute_t *attr, uint16_t conn_id)
{
    HAL_ASSERT((attr != NULL), __FILE__, __LINE__);

    uint16_t handle = 0;

    // Convert generic attr->handle to PSOC63 handle
    switch (attr->attribute)
    {
        case HAL_BLE_ATTR_BATTERY_LEVEL:
        {
            handle = HDLC_BAS_BATTERY_LEVEL_VALUE;
            break;
        }

        case HAL_BLE_ATTR_AIOS_DIGITAL_IO_VALUE:
        {
            handle = HDLC_AUTOMATION_IO_DIGITAL_IO_VALUE;
            break;
        }

        default:
        {
            HAL_ASSERT(0, __FILE__, __LINE__);
            break;
        }
    }

    // Send notification to the BLE client
    wiced_bt_gatt_status_t status = wiced_bt_gatt_server_send_notification(
        conn_id,
        handle,
        attr->length,
        attr->p_value,
        NULL
    );

    if (WICED_BT_SUCCESS != status) {
        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("Send notif error: ");
            QS_U16(0, status);
        QS_END()
        return HAL_BLE_ERROR;
    }

    return HAL_BLE_SUCCESS;
}

// ====================
// BLE stack callbacks
// ====================
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
            wiced_bt_set_local_bdaddr((uint8_t *)cy_bt_device_address, BLE_ADDR_PUBLIC);
            wiced_bt_dev_read_local_addr(bda);
            print_bd_address(bda);
            /* Bluetooth Controller and Host Stack Enabled */
            if (WICED_BT_SUCCESS == p_event_data->enabled.status) {
                /* Perform application-specific initialization */
                le_app_init();
                ble_stack_init_callback(true);
            } else {
                ble_stack_init_callback(false);
            }
            break;
        }

        case BTM_DISABLED_EVT:
        {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("BLE stack disabled."); 
            QS_END()
            break;
        }

        case BTM_RE_START_EVT:
        {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("BLE stack restarted: ");
                QS_U32(0, p_event_data->enabled.status); 
            QS_END()
            break;
        }

        case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
        {
            /* Advertisement State Changed */
            p_adv_mode = &p_event_data->ble_advert_state_changed;

            if (BTM_BLE_ADVERT_OFF == *p_adv_mode) {
                ble_advertising_state_changed_callback(HAL_BLE_ADV_STATE_OFF);
            }
            else {
                ble_advertising_state_changed_callback(HAL_BLE_ADV_STATE_ON);
            }
            break;
        }

        case BTM_BLE_CONNECTION_PARAM_UPDATE:
        {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("Conn params upd sts "); 
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

        case BTM_BLE_PHY_UPDATE_EVT:
        {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("BLE Physical link update: "); 
                QS_U8(0, p_event_data->ble_phy_update_event.status);
                QS_STR("tx_phy: ");
                QS_U16(0, p_event_data->ble_phy_update_event.tx_phy);
                QS_STR("rx_phy: ");
                QS_U16(0, p_event_data->ble_phy_update_event.rx_phy);
            QS_END()
            break;
        }

        case BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
        {
            wiced_result = WICED_BT_ERROR; // not supported for now
            break;
        }

        case BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT:
        {
            wiced_result = WICED_BT_ERROR; // not supported for now
            break;
        }

        case BTM_BLE_DATA_LENGTH_UPDATE_EVENT:
        {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("LE data length update event rcvd.");
                QS_STR("TX octets:");
                QS_U16(0, p_event_data->ble_data_length_update_event.max_tx_octets);
                QS_STR("RX octets:");
                QS_U16(0, p_event_data->ble_data_length_update_event.max_rx_octets);
            QS_END()
            break;
        }

        case BTM_BLE_DEVICE_ADDRESS_UPDATE_EVENT:
        {
            QS_BEGIN_ID(BLE_HAL , 0 /*prio/ID for local Filters*/)
                QS_STR("update random device address: ");
                QS_U8(0, p_event_data->ble_addr_update_event.status);
            QS_END()
            print_bd_address(p_event_data->ble_addr_update_event.bdaddr);
            break;
        }

        default:
        {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
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
    wiced_bt_gatt_attribute_request_t *p_attr_req = &p_event_data->attribute_request;

    uint16_t error_handle = 0;
    /** 
     * Call the appropriate callback function based on the GATT event type, and pass the relevant event
     * parameters to the callback function.
     */
    switch ( event )
    {
        case GATT_CONNECTION_STATUS_EVT:
        {
            gatt_status = le_app_connect_handler( &p_event_data->connection_status );
            break;
        }

        case GATT_ATTRIBUTE_REQUEST_EVT:
        {
            gatt_status = le_app_server_handler(p_attr_req, &error_handle);
            if(gatt_status != WICED_BT_GATT_SUCCESS) {
                wiced_bt_gatt_server_send_error_rsp(
                    p_attr_req->conn_id,
                    p_attr_req->opcode,
                    error_handle,
                    gatt_status
                );
            }
            break;
        }

        default:
        {
            gatt_status = WICED_BT_GATT_ERROR;
            break;
        }
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

    if ( NULL != p_conn_status ) {
        if ( p_conn_status->connected ) {
            /* Device has connected */
            ble_connection_callback((uint16_t)p_conn_status->conn_id, true, 0U);
        } else {
            /* Device has disconnected */
            ble_connection_callback((uint16_t)p_conn_status->conn_id, false, p_conn_status->reason);
        }
    } else {
        gatt_status = WICED_BT_GATT_ERROR;
    }

    return gatt_status;
}

/**************************************************************************************************
* Function Name: le_app_server_handler
***************************************************************************************************
* Summary:
*   This function handles GATT server events from the BT stack.
*
* Parameters:
*  p_attr_req     Pointer to LE GATT connection status
*
* Return:
*  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
*
**************************************************************************************************/
static wiced_bt_gatt_status_t le_app_server_handler (wiced_bt_gatt_attribute_request_t *p_attr_req, 
                                                     uint16_t *p_error_handle)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;
    switch (p_attr_req->opcode)
    {
        case GATT_REQ_READ:
        case GATT_REQ_READ_BLOB:
        {
             /* Attribute read request */
            gatt_status = le_app_read_handler(
                p_attr_req->conn_id,
                p_attr_req->opcode,
                &p_attr_req->data.read_req,
                p_attr_req->len_requested,
                p_error_handle
            );
            break;
        }

        case GATT_REQ_WRITE:
        case GATT_CMD_WRITE:
        {
             /* Attribute write request */
            gatt_status = le_app_write_handler(
                p_attr_req->conn_id,
                p_attr_req->opcode,
                &p_attr_req->data.write_req,
                p_attr_req->len_requested,
                p_error_handle
            );
            break;
        }

        case GATT_REQ_MTU:
        {
            gatt_status = wiced_bt_gatt_server_send_mtu_rsp(
                p_attr_req->conn_id,
                p_attr_req->data.remote_mtu,
                CY_BT_MTU_SIZE
            );
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("MTU request:");
                QS_U16(0, p_attr_req->data.remote_mtu);
                QS_STR("MTU response:");
                QS_U16(0, CY_BT_MTU_SIZE);
            QS_END()
            ble_mtu_changed_callback(CY_BT_MTU_SIZE); // minimal MTU size for PSOC63 in current implementation
            break;
        }

        case GATT_HANDLE_VALUE_NOTIF:
        {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("Notification send complete");
            QS_END()
            break;
        }

        /**
         * @todo Clarify do we need to handle this case ?
         */
        // case GATT_REQ_READ_BY_TYPE:
        // {
        //     gatt_status = app_bt_gatt_req_read_by_type_handler(
        //         p_attr_req->conn_id,
        //         p_attr_req->opcode,
        //         &p_attr_req->data.read_by_type,
        //         p_attr_req->len_requested,
        //         p_error_handle
        //     );
        //     break;

        // }

        default:
        {
            QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                QS_STR("ERROR: Unhandled GATT Connection Request case:");
                QS_U8(0, p_attr_req->opcode);
            QS_END()
            gatt_status = WICED_BT_GATT_ERROR;
            break;
        }
    }

    return gatt_status;
}

/**************************************************************************************************
* Function Name: le_app_read_handler
***************************************************************************************************
* Summary:
*   This function handles Read Requests received from the client device
*
* Parameters:
* @param conn_id       Connection ID
* @param opcode        LE GATT request type opcode
* @param p_read_req    Pointer to read request containing the handle to read
* @param len_req       length of data requested
*
* Return:
*  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
*
**************************************************************************************************/
static wiced_bt_gatt_status_t le_app_read_handler( uint16_t conn_id,
                                                   wiced_bt_gatt_opcode_t opcode,
                                                   wiced_bt_gatt_read_t *p_read_req,
                                                   uint16_t len_req,
                                                   uint16_t *p_error_handle)
{

    gatt_db_lookup_table_t  *puAttribute;
    int          attr_len_to_copy;
    uint8_t     *from;
    int          to_send;

    *p_error_handle = p_read_req->handle;

    puAttribute = le_app_find_by_handle(p_read_req->handle);
    if ( NULL == puAttribute ) {
        return WICED_BT_GATT_INVALID_HANDLE;
    }

    attr_len_to_copy = puAttribute->cur_len;
    if (p_read_req->offset >= puAttribute->cur_len) {
        return WICED_BT_GATT_INVALID_OFFSET;
    }

    to_send = MIN(len_req, attr_len_to_copy - p_read_req->offset);
    from = ((uint8_t *)puAttribute->p_data) + p_read_req->offset;

    return wiced_bt_gatt_server_send_read_handle_rsp(conn_id, opcode, to_send, from, NULL); /* No need for context, as buff not allocated */;
}

/**************************************************************************************************
* Function Name: le_app_write_handler
***************************************************************************************************
* Summary:
*   This function handles Write Requests received from the client device
*
* Parameters:
*  @param conn_id       Connection ID
*  @param opcode        LE GATT request type opcode
*  @param p_write_req   Pointer to LE GATT write request
*  @param len_req       length of data requested
*
* Return:
*  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
*
**************************************************************************************************/
static wiced_bt_gatt_status_t le_app_write_handler( uint16_t conn_id,
                                                    wiced_bt_gatt_opcode_t opcode,
                                                    wiced_bt_gatt_write_req_t *p_write_req,
                                                    uint16_t len_req,
                                                    uint16_t *p_error_handle)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_INVALID_HANDLE;

    *p_error_handle = p_write_req->handle;

    /* Attempt to perform the Write Request */
    gatt_status = le_app_set_value(
        conn_id,
        p_write_req->handle,
        p_write_req->p_val,
        p_write_req->val_len
    );

    if( WICED_BT_GATT_SUCCESS != gatt_status ) {
        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
            QS_STR("WARNING: GATT set attr err status");
            QS_U8(0, gatt_status);
        QS_END()
    } else if(GATT_REQ_WRITE == opcode) {
        wiced_bt_gatt_server_send_write_rsp(conn_id, opcode, p_write_req->handle);
    }

    return (gatt_status);
}

/**************************************************************************************************
* Function Name: le_app_set_value
***************************************************************************************************
* Summary:
*   This function handles writing to the attribute handle in the GATT database using the
*   data passed from the BT stack. The value to write is stored in a buffer
*   whose starting address is passed as one of the function parameters
*
* Parameters:
* @param conn_id      Connection ID
* @param attr_handle  GATT attribute handle
* @param p_val        Pointer to LE GATT write request value
* @param len          length of GATT write request
*
*
* Return:
*   wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e in wiced_bt_gatt.h
*
**************************************************************************************************/
static wiced_bt_gatt_status_t le_app_set_value( uint16_t conn_id,
                                                uint16_t attr_handle,
                                                uint8_t *p_val,
                                                uint16_t len)
{
    (void) conn_id;
    wiced_bool_t isHandleInTable = WICED_FALSE;
    wiced_bool_t validLen = WICED_FALSE;
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_INVALID_HANDLE;

    /** Check for a matching handle entry */
    for (size_t i = 0; i < app_gatt_db_ext_attr_tbl_size; i++) {
        if (app_gatt_db_ext_attr_tbl[i].handle == attr_handle) {
            /** Detected a matching handle in external lookup table */
            isHandleInTable = WICED_TRUE;

            /** Check if the buffer has space to store the data */
            validLen = (app_gatt_db_ext_attr_tbl[i].max_len >= len);

            if (validLen) {
                /** Value fits within the supplied buffer; copy over the value */
                app_gatt_db_ext_attr_tbl[i].cur_len = len;
                memcpy(app_gatt_db_ext_attr_tbl[i].p_data, p_val, len);
                gatt_status = WICED_BT_GATT_SUCCESS;

                /** 
                 * Add code for any action required when this attribute is written.
                 */
                switch ( attr_handle )
                {
                    case HDLD_BAS_BATTERY_LEVEL_CLIENT_CHAR_CONFIG:
                    {
                        uint16_t basCccd = ((uint16_t) p_val[0]) | (((uint16_t) p_val[1]) << 8);
                        BAS_handleCccdWritten(basCccd);
                        break;
                    }

                    case HDLD_AUTOMATION_IO_DIGITAL_IO_CLIENT_CHAR_CONFIG:
                    {
                        uint16_t aiosCccd = ((uint16_t) p_val[0]) | (((uint16_t) p_val[1]) << 8);
                        AIOS_handleCccdWritten(aiosCccd);
                        break;
                    }

                    case HDLC_AUTOMATION_IO_DIGITAL_IO_VALUE:
                    {
                        AIOS_handleSetSwicthWritten(p_val[0]);
                        break;
                    }

                    default:
                    {
                        QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                            QS_STR("WARNING: Write attr not implemented yet.");
                        QS_END()
                        break;
                    }
                }
            } else {
                /* Value to write does not meet size constraints */
                gatt_status = WICED_BT_GATT_INVALID_ATTR_LEN;
            }
            break;
        }
    }

    if (!isHandleInTable) {
        /* TODO: Add code to read value for handles not contained within generated lookup table.
         * This is a custom logic that depends on the application, and is not used in the
         * current application yet. If the value for the current handle is successfully written in the
         * below code snippet, then set the result using:
         * res = WICED_BT_GATT_SUCCESS; */
        switch ( attr_handle )
        {
            default:
            {
                /* The write operation was not performed for the indicated handle */
                QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
                    QS_STR("Write Request to Invalid Handle:");
                    QS_U8(0, attr_handle);
                QS_END()
                gatt_status = WICED_BT_GATT_WRITE_NOT_PERMIT;
                break;
            }
        }
    }

    return gatt_status;
}

/*******************************************************************************
 * Function Name : le_app_find_by_handle
 * *****************************************************************************
 * Summary : @brief  Find attribute description by handle
 *
 * @param handle    handle to look up
 *
 * @return gatt_db_lookup_table_t   pointer containing handle data
 ******************************************************************************/
static gatt_db_lookup_table_t  *le_app_find_by_handle(uint16_t handle)
{
    int i;

    for (i = 0; i < app_gatt_db_ext_attr_tbl_size; i++) {
        if (handle == app_gatt_db_ext_attr_tbl[i].handle ) {
            return (&app_gatt_db_ext_attr_tbl[i]);
        }
    }

    return NULL;
}

// ============================
/// Misc spare functions
// ============================
/**
 * Print the Bluetooth Device Address
 * @param[in] bda Pointer to the Bluetooth Device Address
 * @note This function is used to print the local Bluetooth address
 *       in the debug output.
 * 
 * @return None
 */
static void print_bd_address(uint8_t* bda)
{
    QS_BEGIN_ID(BLE_HAL, 0 /*prio/ID for local Filters*/)
        QS_STR("Local Bluetooth Address: ");
        QS_MEM(bda, BD_ADDR_LEN);
    QS_END()
}

/* [] END OF FILE */
