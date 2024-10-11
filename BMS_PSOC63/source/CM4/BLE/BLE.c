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
 *                      TODO: inplement APP callbacks
 *
 * @version 0.1.0
 */

#include "BLE.h"
#include "cybsp.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "qspyHelper.h"
#include "cycfg_bt_settings.h"
#include "cycfg_gap.h"

///////////////////////
// Functions prototype
///////////////////////
static wiced_result_t app_bt_management_callback_(
    wiced_bt_management_evt_t event,
    wiced_bt_management_evt_data_t *p_event_data
);
static void print_bd_address(uint8_t* bda);
static void le_app_init(void);

///////////////////////
// Private data
///////////////////////

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

    /** 
     * Paring & bonding (link encryption) isn't supported for now 
     *
     * TODO: add authontication & link encryption in future 
     */
    wiced_bt_set_pairable_mode(FALSE, FALSE);

    /** Set Advertisement Data */
    wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, cy_bt_adv_packet_data);

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

static void print_bd_address(uint8_t* bda)
{
    QS_BEGIN_ID(BLE_TRACE, 0 /*prio/ID for local Filters*/)
        QS_STR("Local Bluetooth Address: ");
        QS_MEM(bda, BD_ADDR_LEN);
    QS_END()
}

/* [] END OF FILE */
