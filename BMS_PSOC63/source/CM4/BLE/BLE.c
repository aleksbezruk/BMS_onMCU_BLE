/******************************************************************************
* @file  BLE.c
*
* @brief BLE module functions definition.
*
* @details
*           https://documentation.infineon.com/html/psoc6/moa1717991724927.html#moa1717991724927 
*           https://github.com/Infineon/btstack 
*           https://github.com/Infineon/btstack-integration
*
*           *********************************
*           1. *** BLE stack details: ***
*           *********************************
*             1.1 Infineon/Cypress stack is used -> AIROC™ BTSTACK
*             1.2 Bluetooth® porting layer -> 
*                 AIROC™ BTSTACK requires a porting layer specific to the device it is running on. 
*             1.3 Porting layer for Infineon Bluetooth® devices 
*                 is hosted on GitHub as a library called btstack-integration
*             1.4 Cypress/Infineon BLE stack requires OS layer based on RTOS.
*                 Porting/integration layer of btstack is based oo a FreeRTOS package.
*                 The FreeRTOS package is delivered by Cypress and their BLE stack
*                 looks like is tested on FreeRTOS, so FreeRTOS also will be used 
*                 in the current project .
*           
*==========================================================================================
*           ******************************************************
*           2. *** BLE stack & FreeRTOS libraries/packages config 
*           ******************************************************
*             2.1 Run library manager on 'BMS_PSOC63' folder:
*                    make library-manager
*             2.2 Select library: btstack-integration
*             2.3 Select library: btstack
*             2.4 Select library: freertos
*
*==========================================================================================
*           ******************************************************
*           3. BLE stack architecture
*           ******************************************************
*             3.1 AIROC™ BTSTACK runs on the CM4 core, 
*                  and the controller stack runs on the CM0+ core .
*             3.2 A hardware block called inter-processor communication (IPC) 
*                 is used as HCI (Host-controller interface) .
*                 Therefore, the application uses the component called BLESS-IPC 
*                 in btstack-integration.
*             3.3 CM0P_BLESS component for CM0p build includes the prebuild image
*                 'COMPONENT_CM0P_BLESS\psoc6cm0p_bless.bin' that is a part of MTB package 
*
*==========================================================================================
*           ******************************************************
*           4. BLE profile for BMS implementation details
*           ******************************************************
*             4.1 Use ModusToolbox BLE configurator:
*             4.1.1 $ make bt-configurator
*                     Note: Because PSoC™ 6 Bluetooth® LE device 
*                        no longer supports BLESS middleware from MTB version 3.0 
*                        and uses AIROC™ BTSTACK, use the option ‘AIROC™ BTSTACK with 
*                        Bluetooth® LE only’ while creating a new configuration file.
*             4.1.2 Makefile config :
*                   cm4 -> COMPONENTS=FREERTOS WICED_BLE CM0P_BLESS
*                   The two components FREERTOS and WICED_BLE are required 
*                   to include the files from FreeRTOS and btstack libraries for compilation.
*                   DEFINES=CY_RTOS_AWARE
*                   CY_RTOS_AWARE must be defined to inform the HAL 
*                   that an RTOS environment is being used.
*------------------------------------------------------------------------------------
*             4.2 GAP settings (via bt-configurator) 
*             4.2.1 max remote clients = 1
*             4.2.2 GAP role = Peripheral
*             4.2.3 Set advertising data (full name etc.)
*             4.2.4 Set scan response data (TX power lvl)
*------------------------------------------------------------------------------------
*             4.3 GATT settings (via bt-configurator) 
*             4.3.1 Starting from simple Battery Service (BAS)
*             4.3.2 TODO: Add standard SIG or custom service that will satisfy BMS use case
*------------------------------------------------------------------------------------
*             4.4 Stack initialization 
*             Note: BLE Controller is implemented as part of prebuild image
*                   'psoc6cm0p_bless.bin' as described above. So according to
*                   the Cypress approach there is no need to implement 
*                   BLE user code on CM0p side .
*             4.4.1 wiced_bt_stack_platform_t -> cybt_platform_config_init():
*                   config platform before BLE stack init
*             4.4.2 wiced_bt_cfg_settings_t -> wiced_bt_stack_init():
*                   - cybt_platform_init() ;
*                       - cybt_platform_stack_timer_init() :
*                           - cy_rtos_init_timer() ->
*                              init RTOS timer handled from TmrSvc task (RTOS daemon task) ;
*                   - register app management callback ;
*                   - host_stack_platform_interface_init() ;
*                   - wiced_bt_set_stack_config() ;
*                   - cybt_platform_task_init() :
*                       - cybt_bttask_init() ->
*                           Init RTOS 'bt_task' (bt_task_handler). Prio -> CY_RTOS_PRIORITY_HIGH .
*------------------------------------------------------------------------------------
*             4.5 BLE RTOS threads & BLE LL IRQ config
*             4.5.1 RTOS tasks -> 
*                   For details see wiced_bt_stack_init() -> cybt_bttask_init()
*             4.5.2 BLE LL interrupt
*                   #define CY_BLE_IRQ bless_interrupt_IRQn
*                   bless_interrupt_IRQHandler
*                   Looks like the implementation of the IRQ handler
*                   is hidden inside CMOp prebuild image .
*------------------------------------------------------------------------------------
*             4.6 Application callbacks 
*                 TODO:
*------------------------------------------------------------------------------------
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

/*******************************/
/*** Functions prototype */
/******************************/
static wiced_result_t app_bt_management_callback_(
    wiced_bt_management_evt_t event,
    wiced_bt_management_evt_data_t *p_event_data
);
static void print_bd_address(uint8_t* bda);
static void le_app_init(void);

/*******************************/
/*** Private data */
/******************************/

/*******************************/
/*** Code */
/******************************/
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

/**************************************************************************************************
* Function Name: app_bt_management_callback
***************************************************************************************************
* Summary:
*   This is a Bluetooth stack event handler function to receive management events from
*   the LE stack and process as per the application.
*
* Parameters:
*   wiced_bt_management_evt_t event             : LE event code of one byte length
*   wiced_bt_management_evt_data_t *p_event_data: Pointer to LE management event structures
*
* Return:
*  wiced_result_t: Error code from WICED_RESULT_LIST or BT_RESULT_LIST
*
*************************************************************************************************/
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
/*
        case BTM_BLE_CONNECTION_PARAM_UPDATE:
            printf("Connection parameter update status:%d, Connection Interval: %d, Connection Latency: %d, Connection Timeout: %d\n",
                                           p_event_data->ble_connection_param_update.status,
                                           p_event_data->ble_connection_param_update.conn_interval,
                                           p_event_data->ble_connection_param_update.conn_latency,
                                           p_event_data->ble_connection_param_update.supervision_timeout);
            break;
*/
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
* Function Name: le_app_init
***************************************************************************************************
* Summary:
*   This function handles application level initialization tasks and is called from the BT
*   management callback once the LE stack enabled event (BTM_ENABLED_EVT) is triggered
*   This function is executed in the BTM_ENABLED_EVT management callback.
*
* Parameters:
*   None
*
* Return:
*  None
*
*************************************************************************************************/
static void le_app_init(void)
{
    wiced_result_t wiced_result;

    wiced_bt_set_pairable_mode(FALSE, FALSE);   // no paring

    /* Set Advertisement Data */
    wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, cy_bt_adv_packet_data);

    /* Start Undirected LE Advertisements on device startup.
     * The corresponding parameters are contained in 'app_bt_cfg.c' */
    wiced_result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, 0, NULL);

    /* Failed to start advertisement. Stop program execution */
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
