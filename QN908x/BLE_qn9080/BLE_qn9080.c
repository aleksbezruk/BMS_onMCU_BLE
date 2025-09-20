/**
 * @file BLE_qn9080.c
 * @brief Source file for BLE_qn9080 module
 * @note The BLE_qn9080 module is used to evaluate and bring up the BLE functionality
 *       on the QN9080 platform.
 * @details The module uses the QN9080's BLE stack and its APIs to manage advertisements and connections.
 * @version 0.6.0
 */

#include "BLE_qn9080.h"

// Standard includes
#include <string.h>

// FSL includes
#include "MemManager.h"
#include "TimersManager.h"
#include "SecLib.h"
#include "RNG_Interface.h"
#if gAppUseNvm_d
#include "NVM_Interface.h"
#endif // gAppUseNvm_d

// HAL includes
#include "hal.h"
#include "hal_led.h"

// NVDS includes
// QSPY for debugging
#include "qs.h"
#include "MAIN.h"

// NXP BLE includes
#include "controller_interface.h"
#include "ble_controller_task_config.h"
#include "ble_general.h"
#include "gap_interface.h"
#include "gap_types.h"

// BLE Host Task includes
#include "ble_host_tasks.h"
#include "ble_host_task_config.h"

// GATT includes
#include "gatt_interface.h"
#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
#include "gatt_database.h"
#include "gatt_db_app_interface.h"
#include "ble_sig_defines.h"
#include "att_errors.h"  // ATT error codes
#include "gatt_db/gatt_db_handles.h"  // Include GATT database handle enums

// ============================
// Defines & private variables
// ============================
#define BLE_EXTENDED_LOGS 0

// Forward declarations for missing function prototypes
bool_t Ble_CheckMemoryStorage(void);

// Define minimal NVM data for testing
typedef struct {
    uint32_t configVersion;
    uint32_t testCounter;
    uint8_t  reserved[8];
} bms_test_config_t;

// Test configuration data stored in RAM and backed up to NVM
static bms_test_config_t bmsTestConfig = {
    .configVersion = 0x00010001,  // Version 1.1
    .testCounter = 0,
    .reserved = {0}
};

// Register the test config as an NVM dataset
// This will create an entry in the .NVM_TABLE section
NVM_RegisterDataSet(&bmsTestConfig, 
                    1,                           // 1 element
                    sizeof(bms_test_config_t),   // Element size
                    0x0001,                      // Unique data entry ID
                    gNVM_MirroredInRam_c);       // Mirrored in RAM

// Static advertising data - NXP style implementation
/* Compute the number of elements of an array */
#define NumberOfElements(x) (sizeof(x)/sizeof((x)[0]))

// Set up advertising parameters
static gapAdvertisingParameters_t advParams = {
    .minInterval = 2048,   // 1.28s (2048 * 0.625ms) - very slow for maximum visibility 
    .maxInterval = 2048,   // 1.28s - same as min for consistent timing
    .advertisingType = gAdvConnectableUndirected_c,
    .ownAddressType = gBleAddrTypePublic_c,
    .peerAddressType = gBleAddrTypePublic_c,
    .peerAddress = {0, 0, 0, 0, 0, 0},
    .channelMap = gGapAdvertisingChannelMapDefault_c,
    .filterPolicy = gProcessAll_c
};

static const uint8_t adData0[1] = { (gapAdTypeFlags_t)(gLeGeneralDiscoverableMode_c | gBrEdrNotSupported_c) };
static const char adData1[] = "QN9080_BMS";

// Complete list of 16-bit UUIDs: Battery Service (0x180F) and Automation IO Service (0x1815)
static const uint8_t adData2[4] = { 0x0F, 0x18, 0x15, 0x18 }; // Little-endian format

// 16-bit service data for Battery Service (0x180F) with battery level
static uint8_t adData3[3] = { 0x0F, 0x18, 90 }; // Service UUID (little-endian) + battery level (90%)

static const gapAdStructure_t advScanStruct[] = {
    {
        .length = NumberOfElements(adData0) + 1,  // Data elements + AD type byte
        .adType = gAdFlags_c,
        .aData = (uint8_t*)adData0
    },
    {
        .length = NumberOfElements(adData1) + 1,  // String length (no null terminator) + AD type byte
        .adType = gAdShortenedLocalName_c,
        .aData = (uint8_t*)adData1
    },
    {
        .length = NumberOfElements(adData2) + 1,  // UUID list length + AD type byte
        .adType = gAdComplete16bitServiceList_c,
        .aData = (uint8_t*)adData2
    },
    {
        .length = NumberOfElements(adData3) + 1,  // Service data length + AD type byte
        .adType = gAdServiceData16bit_c,
        .aData = (uint8_t*)adData3
    }
};

static gapAdvertisingData_t g_advData = {
    .cNumAdStructures = sizeof(advScanStruct) / sizeof(gapAdStructure_t),
    .aAdStructures = (gapAdStructure_t*)advScanStruct
};

// TX Power Level for scan response data (typical value: 0 dBm)
static const uint8_t scanRspData0[1] = { 0x00 }; // 0 dBm TX power

static const gapAdStructure_t scanRspStruct[] = {
    {
        .length = NumberOfElements(scanRspData0) + 1,  // Data elements + AD type byte
        .adType = gAdTxPowerLevel_c,
        .aData = (uint8_t*)scanRspData0
    }
};

static gapScanResponseData_t g_scanRspData = {
    .cNumAdStructures = sizeof(scanRspStruct) / sizeof(gapAdStructure_t),
    .aAdStructures = (gapAdStructure_t*)scanRspStruct
};

#if BLE_EXTENDED_LOGS == 1
static volatile uint32_t g_advertisingCallbackCount = 0;
#endif // BLE_EXTENDED_LOGS

// BMS data for GATT characteristics
static uint8_t g_batteryLevel = 90;    // Battery level percentage (0-100%)
static volatile bool isSendAiosNotification = false; // Flag to track if AIO notifications should be sent
static volatile uint8_t aiosSwitchState = 0; // switches state

static deviceId_t client_deviceId = gInvalidDeviceId_c; // Device ID for the current connection
static bool isBatteryServiceSubscribed = false; // Flag to track if Battery Service is subscribed
static volatile bool isAIOServiceSubscribed = false; // Flag to track if Automation IO Service is subscribed

// Structure to store discovered GATT handles
typedef struct {
    uint16_t batteryServiceHandle;          // Battery Service declaration handle
    uint16_t batteryLevelValueHandle;       // Battery Level characteristic value handle
    uint16_t batteryLevelCccdHandle;        // Battery Level CCCD handle
    uint16_t aiosServiceHandle;             // Automation IO Service declaration handle
    uint16_t digitalIoValueHandle;          // Digital IO Value characteristic value handle
    uint16_t digitalIoCccdHandle;           // Digital IO Value CCCD handle
    bool     handlesDiscovered;             // Flag indicating if handles were successfully discovered
} gatt_handles_t;

// Global variable to store discovered GATT handles
static gatt_handles_t g_gattHandles = {
    .batteryServiceHandle = 0x0000,
    .batteryLevelValueHandle = 0x0000,
    .batteryLevelCccdHandle = 0x0000,
    .aiosServiceHandle = 0x0000,
    .digitalIoValueHandle = 0x0000,
    .digitalIoCccdHandle = 0x0000,
    .handlesDiscovered = false
};

static bool isBLEstackInitialized = false;
static bool isAdvertising = false; // Track advertising state for proper state management

// State machine for advertising data updates
typedef enum {
    ADV_UPDATE_IDLE,           // No update in progress
    ADV_UPDATE_STOPPING,       // Waiting for advertising to stop
    ADV_UPDATE_SETTING_DATA,   // Waiting for advertising data to be set
    ADV_UPDATE_STARTING        // Waiting for advertising to start
} adv_update_state_t;

static adv_update_state_t advUpdateState = ADV_UPDATE_IDLE;
static uint8_t pendingBatteryLevel = 0; // Battery level waiting to be updated

/*! Global variable required by BLE host library */
bool_t gHostInitResetController = TRUE;

// ========================
// Function Prototypes
// ========================
// Forward declarations for BLE callbacks
static bleResult_t BLE_HostToControllerInterface(hciPacketType_t packetType, void* pPacket, uint16_t packetSize);
// static void App_SecLibMultCallback(computeDhKeyParam_t *pData);

// Forward declarations for advertising functions
static void BLE_SetupAdvertisingParameters(void);
static void BLE_SetupAdvertisingData(void);
static void BLE_StartAdvertisingInternal(void);
static void BLE_AdvertisingCallback(gapAdvertisingEvent_t* pAdvertisingEvent);
static void BLE_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent);

// Forward declarations for advertising update state machine
static bleResult_t BLE_StartAdvertisingDataUpdate(uint8_t batteryLevel);

// Forward declarations for GATT functions
static void BLE_GattServerCallback(deviceId_t deviceId, gattServerEvent_t* pServerEvent);
static void BLE_InitializeGattDatabase(void);
static void _HAL_BLE_DiscoverGattHandles(void);

/*! *********************************************************************************
 * \brief  Discover GATT Handles using NXP symbolic names and API functions
 * 
 * This function uses the symbolic service name from gatt_db.h for the service handle,
 * then uses NXP API functions to discover characteristic and CCCD handles.
 ********************************************************************************** */
static void _HAL_BLE_DiscoverGattHandles(void)
{
    bleResult_t result;
    bleUuid_t batteryLevelCharUuid;
    bleUuid_t digitalIoCharUuid;

#if (BLE_EXTENDED_LOGS == 1)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_DiscoverGattHandles: Using NXP symbolic service name + API discovery");
    QS_END()
#endif // BLE_EXTENDED_LOGS

    // Reset handles
    g_gattHandles.batteryLevelValueHandle = 0x0000;
    g_gattHandles.batteryLevelCccdHandle = 0x0000;
    g_gattHandles.digitalIoValueHandle = 0x0000;
    g_gattHandles.digitalIoCccdHandle = 0x0000;
    g_gattHandles.handlesDiscovered = false;
    
    // Use symbolic names from gatt_db.h for the service handles (NXP approach)
    g_gattHandles.batteryServiceHandle = service_battery;
    g_gattHandles.aiosServiceHandle = service_automation_io;

#if (BLE_EXTENDED_LOGS == 1)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Battery Service handle (symbolic): 0x");
        QS_U16(0, g_gattHandles.batteryServiceHandle);
        QS_STR(" AIOS Service handle (symbolic): 0x");
        QS_U16(0, g_gattHandles.aiosServiceHandle);
    QS_END()

#endif // (BLE_EXTENDED_LOGS == 1)

    // Set up Battery Level Characteristic UUID (0x2A19)
    batteryLevelCharUuid.uuid16 = gBleSig_BatteryLevel_d;
    
    // Find Battery Level characteristic value handle using NXP API
    result = GattDb_FindCharValueHandleInService(
        g_gattHandles.batteryServiceHandle,
        gBleUuidType16_c,
        &batteryLevelCharUuid,
        &g_gattHandles.batteryLevelValueHandle
    );
    
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Failed to find Battery Level characteristic handle: ");
            QS_U32(0, result);
        QS_END()
        return;
    }

#if (BLE_EXTENDED_LOGS == 1)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Battery Level value handle found: 0x");
        QS_U16(0, g_gattHandles.batteryLevelValueHandle);
    QS_END()

#endif // (BLE_EXTENDED_LOGS == 1)

    // Find Battery Level CCCD handle using the proper NXP API
    result = GattDb_FindCccdHandleForCharValueHandle(
        g_gattHandles.batteryLevelValueHandle,
        &g_gattHandles.batteryLevelCccdHandle
    );
    
#if (BLE_EXTENDED_LOGS == 1)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("GattDb_FindCccdHandleForCharValueHandle result: ");
        QS_U32(0, result);
        QS_STR(" handle: 0x");
        QS_U16(0, g_gattHandles.batteryLevelCccdHandle);
    QS_END()
#endif // (BLE_EXTENDED_LOGS == 1)

    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("CCCD discovery failed or returned invalid handle");
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    // Set up Digital IO Characteristic UUID (128-bit PSOC63 compatible)
    // PSOC63 UUID: 37AF9AE2-211D-4436-9D26-3A9ED02EFEEA
    uint8_t digitalIoUuid128[] = {0xEA, 0xFE, 0x2E, 0xD0, 0x9E, 0x3A, 0x26, 0x9D, 0x36, 0x44, 0x1D, 0x21, 0xE2, 0x9A, 0xAF, 0x37};
    FLib_MemCpy(digitalIoCharUuid.uuid128, digitalIoUuid128, 16);
    
    // Find Digital IO characteristic value handle using NXP API with 128-bit UUID
    result = GattDb_FindCharValueHandleInService(
        g_gattHandles.aiosServiceHandle,
        gBleUuidType128_c,
        &digitalIoCharUuid,
        &g_gattHandles.digitalIoValueHandle
    );
    
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Failed to find Digital IO characteristic handle: ");
            QS_U32(0, result);
        QS_END()
        return;
    }

#if (BLE_EXTENDED_LOGS == 1)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Digital IO value handle found: 0x");
        QS_U16(0, g_gattHandles.digitalIoValueHandle);
    QS_END()

#endif // (BLE_EXTENDED_LOGS == 1)

    // Find Digital IO CCCD handle using the proper NXP API
    result = GattDb_FindCccdHandleForCharValueHandle(
        g_gattHandles.digitalIoValueHandle,
        &g_gattHandles.digitalIoCccdHandle
    );
    
#if (BLE_EXTENDED_LOGS == 1)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Digital IO CCCD handle result: ");
        QS_U32(0, result);
        QS_STR(" handle: 0x");
        QS_U16(0, g_gattHandles.digitalIoCccdHandle);
    QS_END()
#endif // (BLE_EXTENDED_LOGS == 1)

    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Digital IO CCCD discovery failed or returned invalid handle");
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__);
    }

    g_gattHandles.handlesDiscovered = true;
}

/*! *********************************************************************************
 * \brief  GAP Generic Event Callback
 * \param[in] pGenericEvent GAP Generic Event
 ********************************************************************************** */
static void BLE_GenericCallback(gapGenericEvent_t* pGenericEvent)
{
    if (pGenericEvent == NULL) {
        HAL_ASSERT(0, __FILE__, __LINE__); // Invalid event
        return;
    }

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE GAP Event received: type=");
        QS_U32(0, pGenericEvent->eventType);
    QS_END()
#endif // BLE_EXTENDED_LOGS

    switch (pGenericEvent->eventType) {
        case gInitializationComplete_c:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Host Initialization Complete! Initializing GATT database and advertising...");
            QS_END()
            
            // Step 1: Initialize GATT database when BLE host initialization is complete
            BLE_InitializeGattDatabase();
            
            // Step 2: Set advertising parameters after GATT DB is initialized
            BLE_SetupAdvertisingParameters();
            break;
        }

        case gAdvertisingParametersSetupComplete_c:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Advertising Parameters Setup Complete! Setting advertising data...");
            QS_END()
            
            // Step 2: Set advertising data when parameters are set
            BLE_SetupAdvertisingData();
            break;
        }

        case gAdvertisingDataSetupComplete_c:
        {
#if BLE_EXTENDED_LOGS == 1
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Advertising Data Setup Complete! Starting advertising...");
            QS_END()
#endif // BLE_EXTENDED_LOGS

            // Check if this is part of an advertising update sequence
            if (advUpdateState == ADV_UPDATE_SETTING_DATA) {
                // Step 3 of update: Start advertising with updated data
                advUpdateState = ADV_UPDATE_STARTING;
                BLE_StartAdvertisingInternal();
            } else {
                // Initial setup: Start advertising when data is set
                isBLEstackInitialized = true;
                BLE_StartAdvertisingInternal();
            }
            break;
        }

        case gPublicAddressRead_c:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Public Address Read");
            QS_END()
            break;
        }
 
        default:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Unhandled GAP event: ");
                QS_U32(0, pGenericEvent->eventType);
            QS_END()
            break;
        }
    }
}

/*! *********************************************************************************
 * \brief  Host to Controller Interface (from Host to Controller)
 * \param[in] packetType HCI packet type
 * \param[in] pPacket HCI packet
 * \param[in] packetSize HCI packet size
 * \return BLE result
 ********************************************************************************** */
static bleResult_t BLE_HostToControllerInterface(hciPacketType_t packetType, void* pPacket, uint16_t packetSize)
{
    if (pPacket == NULL || packetSize == 0) {
        return gBleInvalidParameter_c;
    }

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("HCI TX: type=");
        QS_U8(0, packetType);
        QS_STR(" size=");
        QS_U16(0, packetSize);
    QS_END()
#endif // BLE_EXTENDED_LOGS

    // Send HCI packet to controller
    return Hci_SendPacketToController(packetType, pPacket, packetSize);
}

// Initialize BLE stack and hardware
void BLE_init(void)
{
    /** Initialize memory manager */
    memStatus_t memStatus = MEM_Init();
    if (memStatus != MEM_SUCCESS_c) {
        HAL_ASSERT(0, __FILE__, __LINE__); // Memory initialization failed
    }

    TMR_Init();

    /**
     * @note Skip SecLib_Init() for basic BLE functionality without security features
     * @details What requires SecLib:
     * - BLE pairing and bonding
     * - Encryption and decryption of BLE packets
     * - Secure connections
     * - ECDH key exchange
     * - Other security-related operations
     * @note If you need security features, uncomment the SecLib_Init() line below
     */
    SecLib_Init(); // Commented out - only needed for encryption/pairing
    /**
     * @note Skip SecLib_SetExternalMultiplicationCb() for basic BLE functionality without security features
     * See NXP BLE example for App_SecLibMultCallback() implementation
     */
    // SecLib_SetExternalMultiplicationCb(App_SecLibMultCallback);

    /** RNG software initialization and PRNG initial seeding (from hardware) */
    uint8_t rngStatus = RNG_Init();
    if (rngStatus != gRngSuccess_d) {
        HAL_ASSERT(0, __FILE__, __LINE__); // RNG initialization failed
    }
    RNG_SetPseudoRandomNoSeed(NULL);

#if gAppUseNvm_d
    /* Initialize NV module only if we have registered datasets */
    if (gNVM_TABLE_entries_c > 0) {
        NVM_Status_t nvmStatus = NvModuleInit();
        if (nvmStatus != gNVM_OK_c) {
            // For debugging: you can check the exact error code
            // Common codes: gNVM_InvalidTableEntriesCount_c, gNVM_ModuleAlreadyInitialized_c
            HAL_ASSERT(0, __FILE__, __LINE__); // NVM initialization failed
        }

        // Optionally restore the test config from NVM
        (void)NvRestoreDataSet(&bmsTestConfig, FALSE);
    }
#endif

    /** Initialize QN9080 BLE Controller. Requires that MEM_Manager and SecLib to be already initialized */
    BLE_Init(gAppMaxConnections_c);

    /** Step 1: Initialize the BLE controller task */
    osaStatus_t taskStatus = Controller_TaskInit();
    if (taskStatus != osaStatus_Success) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Controller_TaskInit failed with status=");
            QS_U32(0, taskStatus);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Controller task initialization failed
    }

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Controller_TaskInit completed successfully");
    QS_END()
#endif // BLE_EXTENDED_LOGS

    /** Step 2: Initialize BLE Controller with HCI callback */
    bleResult_t controllerResult = Controller_Init(Ble_HciRecv);
    if (controllerResult != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Controller_Init failed with result=");
            QS_U32(0, controllerResult);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Controller initialization failed
    }

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Controller_Init completed successfully");
    QS_END()
#endif // BLE_EXTENDED_LOGS

    /** Step 3: Initialize BLE Host Tasks */    
    /* Check for available memory storage */
    if (!Ble_CheckMemoryStorage()) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Ble_CheckMemoryStorage failed - insufficient memory");
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Memory storage check failed
    }
    
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE memory storage check passed - calling Ble_HostTaskInit");
    QS_END()
#endif // BLE_EXTENDED_LOGS
    
    osaStatus_t hostTaskStatus = Ble_HostTaskInit();
    if (hostTaskStatus != osaStatus_Success) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Ble_HostTaskInit failed with status=");
            QS_U32(0, hostTaskStatus);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // BLE Host task initialization failed
    }
    
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Ble_HostTaskInit completed successfully");
    QS_END()
#endif // BLE_EXTENDED_LOGS

    /** Step 4: Initialize BLE Host with callbacks */    
    bleResult_t hostResult = Ble_HostInitialize(BLE_GenericCallback, BLE_HostToControllerInterface);
    if (hostResult != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Ble_HostInitialize failed with result=");
            QS_U32(0, hostResult);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Host initialization failed
    }
    
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Ble_HostInitialize completed successfully");
    QS_END()
#endif // BLE_EXTENDED_LOGS

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE initialization complete - advertising will start automatically");
        QS_STR(" when stack generates gInitializationComplete_c event");
    QS_END()
    QS_FLUSH();
#endif // BLE_EXTENDED_LOGS
}

/*! *********************************************************************************
 * \brief  Setup Advertising Parameters (called on gInitializationComplete_c)
 ********************************************************************************** */
static void BLE_SetupAdvertisingParameters(void)
{
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_SetupAdvertisingParameters: Setting up advertising parameters");
    QS_END()
    QS_FLUSH();
#endif // BLE_EXTENDED_LOGS

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Setting advertising params: min=");
        QS_U16(0, advParams.minInterval);
        QS_STR(" max=");
        QS_U16(0, advParams.maxInterval);
        QS_STR(" type=");
        QS_U8(0, advParams.advertisingType);
    QS_END()
    QS_FLUSH();
#endif // BLE_EXTENDED_LOGS

    bleResult_t result = Gap_SetAdvertisingParameters(&advParams);
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_SetAdvertisingParameters failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Critical failure
    }

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Gap_SetAdvertisingParameters completed successfully");
    QS_END()
    QS_FLUSH();
#endif // BLE_EXTENDED_LOGS
}

/*! *********************************************************************************
 * \brief  Setup Advertising Data (called on gAdvertisingParametersSetupComplete_c)
 ********************************************************************************** */
static void BLE_SetupAdvertisingData(void)
{
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_SetupAdvertisingData: Setting up advertising data (NXP style)");
    QS_END()

    // Debug: Log advertising data details (structures are statically initialized)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Advertising data setup: structures=");
        QS_U8(0, g_advData.cNumAdStructures);
        QS_STR(" device_name='QN9080_BMS' with service UUIDs and battery data");
    QS_END()
    
    // Debug: Print advertising data structures
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("ADV Structure 0 (Flags): len=");
        QS_U8(0, advScanStruct[0].length);
        QS_STR(" type=");
        QS_U8(0, advScanStruct[0].adType);
        QS_STR(" flags=");
        QS_U8(0, adData0[0]);
    QS_END()
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("ADV Structure 1 (Name): len=");
        QS_U8(0, advScanStruct[1].length);
        QS_STR(" type=");
        QS_U8(0, advScanStruct[1].adType);
        QS_STR(" name='");
        QS_STR((char*)advScanStruct[1].aData);
        QS_STR("'");
    QS_END()
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("ADV Structure 2 (UUIDs): len=");
        QS_U8(0, advScanStruct[2].length);
        QS_STR(" type=");
        QS_U8(0, advScanStruct[2].adType);
        QS_STR(" UUIDs: 0x180F, 0x1815");
    QS_END()
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("ADV Structure 3 (Service Data): len=");
        QS_U8(0, advScanStruct[3].length);
        QS_STR(" type=");
        QS_U8(0, advScanStruct[3].adType);
        QS_STR(" battery=");
        QS_U8(0, adData3[2]);
        QS_STR("%");
    QS_END()
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Scan Response: structures=");
        QS_U8(0, g_scanRspData.cNumAdStructures);
        QS_STR(" TX_power=");
        QS_U8(0, scanRspData0[0]);
        QS_STR("dBm");
    QS_END()
#endif // BLE_EXTENDED_LOGS

    // Use g_scanRspData (empty scan response) following NXP pattern
    bleResult_t result = Gap_SetAdvertisingData(&g_advData, &g_scanRspData);
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_SetAdvertisingData failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Critical failure
    }
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Gap_SetAdvertisingData completed successfully (NXP style)");
    QS_END()
    QS_FLUSH();
#endif // BLE_EXTENDED_LOGS
}

/*! *********************************************************************************
 * \brief  Start Advertising Internal (called on gAdvertisingDataSetupComplete_c)
 ********************************************************************************** */
static void BLE_StartAdvertisingInternal(void)
{
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_StartAdvertisingInternal: Starting advertising");
    QS_END()
#endif // BLE_EXTENDED_LOGS

    bleResult_t result = Gap_StartAdvertising(BLE_AdvertisingCallback, BLE_ConnectionCallback);
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_StartAdvertising failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        isAdvertising = false; // Ensure state is correct on failure
        HAL_ASSERT(0, __FILE__, __LINE__); // Critical failure
    } else {
        isAdvertising = true; // Set state on successful start
    }

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Gap_StartAdvertising completed successfully - BLE device is now advertising!");
        QS_STR(" Device should be visible as 'QN9080_BMS' on BLE scanners.");
    QS_END()
#endif // BLE_EXTENDED_LOGS
}

/*! *********************************************************************************
 * \brief  Advertising Event Callback
 * \param[in] pAdvertisingEvent Advertising Event
 ********************************************************************************** */
static void BLE_AdvertisingCallback(gapAdvertisingEvent_t* pAdvertisingEvent)
{
#if BLE_EXTENDED_LOGS == 1
    // Set the flag immediately to track that callback was called
    g_advertisingCallbackCount++;
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("*** ADVERTISING CALLBACK TRIGGERED *** Count=");
        QS_U32(0, g_advertisingCallbackCount);
    QS_END()
    
    if (pAdvertisingEvent == NULL) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("BLE Advertising Event: NULL pointer received");
        QS_END()
        return;
    }

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE Advertising Event: type=");
        QS_U32(0, pAdvertisingEvent->eventType);
        QS_STR(" (gAdvertisingStateChanged_c=");
        QS_U32(0, gAdvertisingStateChanged_c);
        QS_STR(")");
    QS_END()
#endif // BLE_EXTENDED_LOGS

    switch (pAdvertisingEvent->eventType) {
        case gAdvertisingStateChanged_c:
        {
            // Handle advertising state changes for update state machine
            if (advUpdateState == ADV_UPDATE_STOPPING) {
                // Step 1 complete: Advertising stopped, now update data
#if BLE_EXTENDED_LOGS == 1
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("*** Advertising Stopped for Data Update ***");
                QS_END()
#endif // BLE_EXTENDED_LOGS

                isAdvertising = false;
                advUpdateState = ADV_UPDATE_SETTING_DATA;
                
                // Update the advertising service data with new battery level
                adData3[2] = pendingBatteryLevel;
                
                // Set advertising data - this will trigger gAdvertisingDataSetupComplete_c
                bleResult_t result = Gap_SetAdvertisingData(&g_advData, &g_scanRspData);
                if (result != gBleSuccess_c) {
                    QS_BEGIN_ID(MAIN, 0)
                        QS_STR("Failed to set advertising data during update: ");
                        QS_U32(0, result);
                    QS_END()
                    advUpdateState = ADV_UPDATE_IDLE; // Reset state on failure
                }
            } else if (advUpdateState == ADV_UPDATE_STARTING) {
#if BLE_EXTENDED_LOGS == 1
                // Step 3 complete: Advertising started with updated data
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("*** Advertising Started with Updated Data ***");
                QS_END()
#endif // BLE_EXTENDED_LOGS

                isAdvertising = true;
                advUpdateState = ADV_UPDATE_IDLE; // Update sequence complete
                
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Advertising data update completed successfully - battery level: ");
                    QS_U8(0, pendingBatteryLevel);
                    QS_STR("%");
                QS_END()
            } else {
                // Normal advertising state change
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("*** Advertising State Changed ***");
                QS_END()
            }
            break;
        }
            
        case gAdvertisingCommandFailed_c:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("*** Advertising Command Failed *** - reason=");
                QS_U32(0, pAdvertisingEvent->eventData.failReason);
            QS_END()
            QS_FLUSH();
            HAL_ASSERT(0, __FILE__, __LINE__); // Critical failure
            break;
        }
            
        default:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Unhandled Advertising event: ");
                QS_U32(0, pAdvertisingEvent->eventType);
            QS_END()
            break;
        }
    }
}

/*! *********************************************************************************
 * \brief  Connection Event Callback
 * \param[in] peerDeviceId Device ID of the peer
 * \param[in] pConnectionEvent Connection Event
 * \return None
 ********************************************************************************** */
static void BLE_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent)
{
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE Connection Event: device=");
        QS_U8(0, peerDeviceId);
        QS_STR(" type=");
        QS_U32(0, pConnectionEvent->eventType);
    QS_END()
#endif // BLE_EXTENDED_LOGS

    switch (pConnectionEvent->eventType) {
        case gConnEvtConnected_c:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Device Connected!");
            QS_END()
            client_deviceId = peerDeviceId; // Store the connected device ID
            isAdvertising = false; // Advertising automatically stops when connected
            break;
        }
   
        case gConnEvtDisconnected_c:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Device Disconnected - reason=");
                QS_U32(0, pConnectionEvent->eventData.disconnectedEvent.reason);
                QS_STR(" - Restarting advertising...");
            QS_END()

            client_deviceId = gInvalidDeviceId_c; // Reset client device ID

            // Restart advertising after disconnect
            bleResult_t result = Gap_StartAdvertising(BLE_AdvertisingCallback, BLE_ConnectionCallback);
            if (result != gBleSuccess_c) {
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Failed to restart advertising after disconnect: ");
                    QS_U32(0, result);
                QS_END()
                QS_FLUSH();
                isAdvertising = false; // Ensure state is correct on failure
            } else {
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Advertising restarted successfully after disconnect");
                QS_END()
                isAdvertising = true; // Set state on successful restart
            }
            break;
        }
        
        case gConnEvtLeDataLengthChanged_c:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Data Length Changed: TX=");
                QS_U16(0, pConnectionEvent->eventData.leDataLengthChanged.maxTxOctets);
                QS_STR(" RX=");
                QS_U16(0, pConnectionEvent->eventData.leDataLengthChanged.maxRxOctets);
            QS_END()
            break;
        }
            
        default:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Unhandled Connection event: ");
                QS_U32(0, pConnectionEvent->eventType);
            QS_END()
            break;
        }
    }
}

// Global flag to track if advertising callback was ever called
// static volatile uint32_t g_advertisingCallbackCount = 0;

/*! *********************************************************************************
 * \brief  Decode BLE error codes for better diagnostics
 * \param[in] errorCode BLE error code to decode
 * \return String description of the error
 ********************************************************************************** */
// static const char* BLE_DecodeError(bleResult_t errorCode)
// {
//     switch (errorCode) {
//         case gBleSuccess_c: return "SUCCESS";
//         case gBleInvalidParameter_c: return "INVALID_PARAMETER";
//         case gBleOverflow_c: return "OVERFLOW";
//         // case gBleUnderflow_c: return "UNDERFLOW";  // May not exist in this stack
//         case gBleOutOfMemory_c: return "OUT_OF_MEMORY";
//         case gBleAlreadyInitialized_c: return "ALREADY_INITIALIZED";
//         case gBleOsError_c: return "OS_ERROR";
//         case gBleUnexpectedError_c: return "UNEXPECTED_ERROR";
//         case gBleInvalidState_c: return "INVALID_STATE";
//         case gBleTimerError_c: return "TIMER_ERROR";
//         case gHciUnknownHciCommand_c: return "HCI_UNKNOWN_COMMAND";
//         case gHciUnknownConnectionIdentifier_c: return "HCI_UNKNOWN_CONNECTION";
//         case gHciHardwareFailure_c: return "HCI_HARDWARE_FAILURE";
//         case gHciPageTimeout_c: return "HCI_PAGE_TIMEOUT";
//         case gHciAuthenticationFailure_c: return "HCI_AUTH_FAILURE";
//         case gHciPinOrKeyMissing_c: return "HCI_PIN_OR_KEY_MISSING";
//         case gHciMemoryCapacityExceeded_c: return "HCI_MEMORY_EXCEEDED";
//         case gHciConnectionTimeout_c: return "HCI_CONNECTION_TIMEOUT";
//         case gHciConnectionLimitExceeded_c: return "HCI_CONNECTION_LIMIT";
//         case gHciCommandDisallowed_c: return "HCI_COMMAND_DISALLOWED";
//         // Only include GAP/GATT errors if they exist in this stack
//         // case gGapAdvDataTooLong_c: return "GAP_ADV_DATA_TOO_LONG";
//         // case gGapScanRspDataTooLong_c: return "GAP_SCAN_RSP_TOO_LONG";
//         // case gGapDeviceNotBonded_c: return "GAP_DEVICE_NOT_BONDED";
//         default: return "UNKNOWN_ERROR";
//     }
// }

/*! *********************************************************************************
 * \brief  Stop BLE Advertising
 * \return BLE_qn9080_status_t Status of the operation
 ********************************************************************************** */
BLE_qn9080_status_t BLE_StopAdvertising(void)
{
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_StopAdvertising: Stopping advertising");
    QS_END()

    bleResult_t result = Gap_StopAdvertising();
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_StopAdvertising failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        return BLE_QN9080_STATUS_ERROR;
    } else {
        isAdvertising = false; // Set state on successful stop
    }
    
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Gap_StopAdvertising completed successfully");
    QS_END()
#endif  // BLE_EXTENDED_LOGS

    return BLE_QN9080_STATUS_OK;
}

/*! *********************************************************************************
 * \brief  Start Advertising Data Update Process (Asynchronous State Machine)
 * \param[in] batteryLevel New battery level to advertise
 * \return bleResult_t Result of starting the update process
 ********************************************************************************** */
static bleResult_t BLE_StartAdvertisingDataUpdate(uint8_t batteryLevel)
{
    // Check if an update is already in progress
    if (advUpdateState != ADV_UPDATE_IDLE) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Advertising update already in progress - state: ");
            QS_U8(0, advUpdateState);
        QS_END()
        return gBleInvalidState_c;
    }
    
    // Check if advertising is currently active
    if (!isAdvertising) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Advertising not active - updating data directly");
        QS_END()
        
        // Update data directly and start advertising
        adData3[2] = batteryLevel;
        advUpdateState = ADV_UPDATE_SETTING_DATA;
        return Gap_SetAdvertisingData(&g_advData, &g_scanRspData);
    }
    
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Starting advertising data update sequence for battery level: ");
        QS_U8(0, batteryLevel);
        QS_STR("%");
    QS_END()
#endif // BLE_EXTENDED_LOGS

    
    // Save the battery level for the update sequence
    pendingBatteryLevel = batteryLevel;
    
    // Step 1: Stop advertising - this will trigger gAdvertisingStateChanged_c
    advUpdateState = ADV_UPDATE_STOPPING;
    return Gap_StopAdvertising();
}

/*! *********************************************************************************
 * \brief  Initialize GATT Database (called on gInitializationComplete_c)
 ********************************************************************************** */
static void BLE_InitializeGattDatabase(void)
{
#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_InitializeGattDatabase: Initializing GATT database");
    QS_END()
#endif // BLE_EXTENDED_LOGS

    // Initialize GATT database from static definition
    bleResult_t result = GattDb_Init();
    if (!((result ==gBleSuccess_c) || (result == gBleAlreadyInitialized_c))) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("GattDb_Init failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Critical failure
    }

    // Discover GATT handles after database initialization
    _HAL_BLE_DiscoverGattHandles();

    // Register GATT server callback
    result = GattServer_RegisterCallback(BLE_GattServerCallback);
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("GattServer_RegisterCallback failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Critical failure
    }

    // Register char_digital_io_value characteristic for write notifications using discovered handle
    result = GattServer_RegisterHandlesForWriteNotifications(1, &g_gattHandles.digitalIoValueHandle);
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Failed to register digital IO value handle for write notifications: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Critical failure
    }

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("GATT Database and Server initialized successfully");
        QS_STR(" - Digital IO Value handle 0x");
        QS_U16(0, g_gattHandles.digitalIoValueHandle);
        QS_STR(" registered for write notifications");
    QS_END()
#endif // BLE_EXTENDED_LOGS
}

/*! *********************************************************************************
 * \brief  GATT Server Event Callback
 * \param[in] deviceId Device ID of the peer
 * \param[in] pServerEvent GATT Server Event
 * \return None
 ********************************************************************************** */
static void BLE_GattServerCallback(deviceId_t deviceId, gattServerEvent_t* pServerEvent)
{
    if (pServerEvent == NULL) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("GATT Server Event: NULL pointer received");
        QS_END()
        HAL_ASSERT(0, __FILE__, __LINE__); // Invalid event
    }

#if BLE_EXTENDED_LOGS == 1
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("GATT Server Event: device=");
        QS_U8(0, deviceId);
        QS_STR(" type=");
        QS_U32(0, pServerEvent->eventType);
    QS_END()
#endif // BLE_EXTENDED_LOGS

    switch (pServerEvent->eventType) {
        case gEvtMtuChanged_c:
        {
            uint16_t newMtu = pServerEvent->eventData.mtuChangedEvent.newMtu;
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("GATT MTU Changed to: ");
                QS_U16(0, newMtu);
                QS_STR(" bytes");
            QS_END()
            break;
        }
        
        case gEvtAttributeWritten_c:
        {
            uint16_t handle = pServerEvent->eventData.attributeWrittenEvent.handle;
            uint16_t valueLength = pServerEvent->eventData.attributeWrittenEvent.cValueLength;
            uint8_t* pValue = pServerEvent->eventData.attributeWrittenEvent.aValue;

#if BLE_EXTENDED_LOGS == 1
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("GATT Attribute Written: handle=0x");
                QS_U16(0, handle);
                QS_STR(" length=");
                QS_U16(0, valueLength);
                QS_STR(" value=0x");
                if (valueLength > 0) {
                    QS_U8(0, pValue[0]);
                }
            QS_END()
#endif // BLE_EXTENDED_LOGS

            // Check if this is the digital IO value characteristic
            if (handle == g_gattHandles.digitalIoValueHandle) {
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Digital IO Value characteristic written: ");
                    if (valueLength > 0) {
                        QS_STR("value=0x");
                        QS_U8(0, pValue[0]);
                    }
                QS_END()

                // Write the attribute value to the database
                if ((valueLength > 0) && (pValue != NULL)) {
                    // Store received data directly (now sending proper numeric values)
                    uint8_t dbData[4] = {0};
                    uint16_t storeLength = (valueLength > 4) ? 4 : valueLength;
                    for (uint16_t i = 0; i < storeLength; i++) {
                        dbData[i] = pValue[i];
                    }
                    
                    QS_BEGIN_ID(MAIN, 0)
                        QS_STR("Digital IO write: ");
                        QS_U16(0, valueLength);
                        QS_STR(" bytes [");
                        for (uint16_t i = 0; i < storeLength; i++) {
                            QS_U8(0, pValue[i]);
                            if (i < storeLength - 1) QS_STR(",");
                        }
                        QS_STR("] -> switch state: ");
                        QS_U8(0, pValue[0]);
                    QS_END()

                    // Write to database 
                    bleResult_t writeResult = GattDb_WriteAttribute(handle, 4, dbData);
                    if (writeResult != gBleSuccess_c) {
                        QS_BEGIN_ID(MAIN, 0)
                            QS_STR("Failed to write digital IO value to database: ");
                            QS_U32(0, writeResult);
                        QS_END()

                        // Send error response
                        GattServer_SendAttributeWrittenStatus(deviceId, handle, gAttErrCodeWriteNotPermitted_c);
                        break;
                    } else {
                        // Database write successful - store switch state and trigger notification
                        __disable_irq();
                        aiosSwitchState = pValue[0]; // Use first byte as switch state
                        isSendAiosNotification = true;
                        __enable_irq();
                    }
                }

                // Send success response
                bleResult_t statusResult = GattServer_SendAttributeWrittenStatus(deviceId, handle, gAttErrCodeNoError_c);
                if (statusResult != gBleSuccess_c) {
                    QS_BEGIN_ID(MAIN, 0)
                        QS_STR("Failed to send attribute written status: ");
                        QS_U32(0, statusResult);
                    QS_END()
                }
            } else {
                // Unknown attribute handle
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Unknown attribute handle written: 0x");
                    QS_U16(0, handle);
                    QS_STR(" (expected digital IO: 0x");
                    QS_U16(0, g_gattHandles.digitalIoValueHandle);
                    QS_STR(")");
                QS_END()
                
                // Send error response for unknown handles
                GattServer_SendAttributeWrittenStatus(deviceId, handle, gAttErrCodeWriteNotPermitted_c);
            }
            break;
        }

        case gEvtAttributeRead_c:
        {
#if BLE_EXTENDED_LOGS == 1
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("GATT Attribute Read: handle=");
                QS_U16(0, pServerEvent->eventData.attributeReadEvent.handle);
            QS_END()
#endif // BLE_EXTENDED_LOGS

            // TODO: implement attribute read handling
            // Update battery level and other characteristics when read
            // You can update characteristic values here based on handle
            // Example: if this is battery level characteristic, read actual battery level
            // and update the database value
            break;
        }
        
        case gEvtCharacteristicCccdWritten_c:
        {
            uint16_t handle = pServerEvent->eventData.charCccdWrittenEvent.handle;
            uint16_t cccdValue = pServerEvent->eventData.charCccdWrittenEvent.newCccd;

            QS_BEGIN_ID(MAIN, 0)
                QS_STR("GATT CCCD Written: handle=0x");
                QS_U16(0, handle);
                QS_STR(" cccd=0x");
                QS_U16(0, cccdValue);
            QS_END()

            // Check if this is the Battery Level CCCD using discovered handle
            if (handle == g_gattHandles.batteryLevelCccdHandle) {
                // Battery Level CCCD was written
                isBatteryServiceSubscribed = (cccdValue == gCccdNotification_c);
                
                // Save CCCD value to the device database (required by NXP stack)
                bleResult_t saveResult = Gap_SaveCccd(deviceId, handle, cccdValue);
                if (saveResult != gBleSuccess_c) {
                    QS_BEGIN_ID(MAIN, 0)
                        QS_STR("Failed to save Battery CCCD: ");
                        QS_U32(0, saveResult);
                    QS_END()
                }
                
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Battery Level CCCD updated: notifications ");
                    QS_STR(isBatteryServiceSubscribed ? "ENABLED" : "DISABLED");
                    QS_STR(" (saved to device DB)");
                QS_END()
                
                // If notifications were enabled, verify CCCD is properly saved and send battery level
                if (isBatteryServiceSubscribed && (client_deviceId != gInvalidDeviceId_c)) {
                    // Use Gap_CheckNotificationStatus to verify CCCD is properly registered
                    bool_t isNotifActive = FALSE;
                    bleResult_t checkResult = Gap_CheckNotificationStatus(deviceId, handle, &isNotifActive);
                    
                    if ((checkResult == gBleSuccess_c) && (isNotifActive == TRUE)) {
                        bleResult_t result = GattServer_SendNotification(client_deviceId, g_gattHandles.batteryLevelValueHandle);
                        (void)result; // Ignore result for now, handled in callback
#if BLE_EXTENDED_LOGS == 1
                        if (result == gBleSuccess_c) {
                            QS_BEGIN_ID(MAIN, 0)
                                QS_STR("Initial battery level notification sent: ");
                                QS_U8(0, g_batteryLevel);
                                QS_STR("%");
                            QS_END()
                        } else {
                            QS_BEGIN_ID(MAIN, 0)
                                QS_STR("Failed to send initial battery notification: ");
                                QS_U32(0, result);
                            QS_END()
                        }
#endif // BLE_EXTENDED_LOGS
                    } else {
                        QS_BEGIN_ID(MAIN, 0)
                            QS_STR("CCCD not yet active for notifications - check result: ");
                            QS_U32(0, checkResult);
                            QS_STR(" isActive: ");
                            QS_U32(0, isNotifActive);
                            QS_STR(" (will retry on next battery update)");
                        QS_END()
                    }
                }
            } 
            // Check if this is the Digital IO Value CCCD
            else if (handle == g_gattHandles.digitalIoCccdHandle) {
                // Digital IO Value CCCD was written
                bool isDigitalIoSubscribed = (cccdValue == gCccdNotification_c);
                
                // Save CCCD value to the device database (required by NXP stack)
                bleResult_t saveResult = Gap_SaveCccd(deviceId, handle, cccdValue);
                if (saveResult != gBleSuccess_c) {
                    QS_BEGIN_ID(MAIN, 0)
                        QS_STR("Failed to save Digital IO CCCD: ");
                        QS_U32(0, saveResult);
                    QS_END()
                } else {
                    isAIOServiceSubscribed = isDigitalIoSubscribed;
                }
                
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Digital IO Value CCCD updated: notifications ");
                    QS_STR(isDigitalIoSubscribed ? "ENABLED" : "DISABLED");
                    QS_STR(" (saved to device DB)");
                QS_END()
                
                // Note: Notification will be sent only when characteristic is written, not on subscription
            } 
            else {
                // Unknown CCCD handle - show all known handles for comparison
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Unknown CCCD handle: 0x");
                    QS_U16(0, handle);
                    QS_STR(" (expected - battery: 0x");
                    QS_U16(0, g_gattHandles.batteryLevelCccdHandle);
                    QS_STR(", digital IO: 0x");
                    QS_U16(0, g_gattHandles.digitalIoCccdHandle);
                    QS_STR(")");
                QS_END()
            }
            break;
        }

        case gEvtError_c:
        {
            gattServerProcedureError_t* pError = &pServerEvent->eventData.procedureError;
            
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("GATT Server Procedure Error: type=");
                QS_U32(0, pError->procedureType);
                QS_STR(" error=");
                QS_U32(0, pError->error);
                // Decode specific error codes
                if (pError->error == 2050) { // gDevDbCccdNotFound_c
                    QS_STR(" (CCCD not found in device DB)");
                }
            QS_END()

            // Log specific procedure types that failed
            switch (pError->procedureType) {
                case gSendNotification_c:
                    QS_BEGIN_ID(MAIN, 0)
                        if (pError->error == 2050) { // gDevDbCccdNotFound_c
                            QS_STR("Notification failed: CCCD not found in device DB - client may need to re-subscribe");
                        } else {
                            QS_STR("Notification procedure failed - client may have disconnected or CCCD not enabled");
                        }
                    QS_END()
                    break;
                case gSendIndication_c:
                    QS_BEGIN_ID(MAIN, 0)
                        QS_STR("Indication procedure failed");
                    QS_END()
                    break;
                case gSendAttributeWrittenStatus_c:
                    QS_BEGIN_ID(MAIN, 0)
                        QS_STR("Send attribute written status failed");
                    QS_END()
                    break;
                case gSendAttributeReadStatus_c:
                    QS_BEGIN_ID(MAIN, 0)
                        QS_STR("Send attribute read status failed");
                    QS_END()
                    break;
                default:
                    QS_BEGIN_ID(MAIN, 0)
                        QS_STR("Unknown procedure type failed");
                    QS_END()
                    break;
            }
            break;
        }
        
        default:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Unhandled GATT Server event: ");
                QS_U32(0, pServerEvent->eventType);
            QS_END()
            break;
        }
    }
}

/*! *********************************************************************************
 * \brief  Update BMS Characteristics with Current Values
 * \param[in] vbat Battery voltage level (0-100%
 * 
 * This function updates the battery level characteristic in the GATT database
 * and sends a notification if the device is connected and notifications are enabled.
 * \return BLE_qn9080_status_t Status of the operation
 ********************************************************************************** */
BLE_qn9080_status_t BLE_UpdateBMSCharacteristics(uint8_t vbat)
{
    bleResult_t result;

    g_batteryLevel = vbat; // Update the global battery level variable

    // Note: adData3[2] will be updated by the advertising state machine when needed

    // Update battery level characteristic using the discovered handle
    if ((client_deviceId != gInvalidDeviceId_c) && (isBatteryServiceSubscribed)) {
        result = GattDb_WriteAttribute(g_gattHandles.batteryLevelValueHandle, sizeof(g_batteryLevel), &g_batteryLevel);
        if (result != gBleSuccess_c) {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Failed to update battery level (handle 0x");
                QS_U16(0, g_gattHandles.batteryLevelValueHandle);
                QS_STR("): ");
                QS_U32(0, result);
            QS_END()
            return BLE_QN9080_STATUS_ERROR;
        }
    }

    // If device is connected and notifications are enabled, send notifications
    if ((client_deviceId != gInvalidDeviceId_c) && isBatteryServiceSubscribed) {
        // Verify CCCD is properly registered before sending notification
        bool_t isNotifActive = FALSE;
        bleResult_t checkResult = Gap_CheckNotificationStatus(client_deviceId, g_gattHandles.batteryLevelCccdHandle, &isNotifActive);
        
        if ((checkResult == gBleSuccess_c) && (isNotifActive == TRUE)) {
            // Send battery level notification using discovered handle
            result = GattServer_SendNotification(client_deviceId, g_gattHandles.batteryLevelValueHandle);
#if BLE_EXTENDED_LOGS == 1
            if (result == gBleSuccess_c) {
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Battery level notification sent: ");
                    QS_U8(0, g_batteryLevel);
                    QS_STR("% (handle 0x");
                    QS_U16(0, g_gattHandles.batteryLevelValueHandle);
                    QS_STR(")");
                QS_END()
            } else {
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Failed to send battery notification (handle 0x");
                    QS_U16(0, g_gattHandles.batteryLevelValueHandle);
                    QS_STR("): ");
                    QS_U32(0, result);
                QS_END()
            }
#endif // BLE_EXTENDED_LOGS
        } else {
#if BLE_EXTENDED_LOGS == 1
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("CCCD not active - skipping notification (check result: ");
                QS_U32(0, checkResult);
                QS_STR(" isActive: ");
                QS_U32(0, isNotifActive);
                QS_STR(")");
            QS_END()
#endif // BLE_EXTENDED_LOGS
        }
    }

    if ((client_deviceId != gInvalidDeviceId_c) && isAIOServiceSubscribed && isSendAiosNotification) {
        // Send notification if client is subscribed to digital IO value notifications
        bool_t isNotifActive = FALSE;
        bleResult_t checkResult = Gap_CheckNotificationStatus(client_deviceId, g_gattHandles.digitalIoCccdHandle, &isNotifActive);
        
        if ((checkResult == gBleSuccess_c) && (isNotifActive == TRUE)) {
            bleResult_t notifyResult = GattServer_SendNotification(client_deviceId, g_gattHandles.digitalIoValueHandle);
            if (notifyResult == gBleSuccess_c) {
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Digital IO Value notification sent successfully after write: ");
                    QS_U8(0, aiosSwitchState);
                QS_END()
                __disable_irq();
                isSendAiosNotification = false; // Reset flag after sending
                __enable_irq();

            } else {
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Failed to send digital IO value notification: ");
                    QS_U32(0, notifyResult);
                QS_END()
            }
        } else {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Digital IO Value successfully written - no notification (client not subscribed)");
            QS_END()
        }
    }

    // If not connected, update advertising data to reflect new battery level
    if ((client_deviceId == gInvalidDeviceId_c) && (isBLEstackInitialized == true)) {
        // Use asynchronous state machine to update advertising data
        result = BLE_StartAdvertisingDataUpdate(vbat);
        if (result != gBleSuccess_c) {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Failed to start advertising data update: ");
                QS_U32(0, result);
            QS_END()
            return BLE_QN9080_STATUS_ERROR;
        }
#if BLE_EXTENDED_LOGS == 1
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Advertising data update started (asynchronous)");
        QS_END()
#endif // BLE_EXTENDED_LOGS
    }

    return BLE_QN9080_STATUS_SUCCESS;
}

/*! *********************************************************************************
* \brief SecLib Callback
*
* \param[in] pData SecLib message data
*
* \return  none
*
********************************************************************************** */
// static void App_SecLibMultCallback(computeDhKeyParam_t *pData)
// {
//     appMsgFromHost_t *pMsgIn = NULL;

//     /* Allocate a buffer with enough space to store also the notified value*/
//     pMsgIn = MSG_Alloc(sizeof(uint32_t) + sizeof(secLibMsgData_t));

//     if (pMsgIn == NULL)
//     {
//         return;
//     }

//     pMsgIn->msgType = (uint32_t)gAppSecLibMultiplyMsg_c;
//     pMsgIn->msgData.secLibMsgData.pData = pData;

//     /* Put message in the Host Stack to App queue */
//     (void)MSG_Queue(&mHostAppInputQueue, pMsgIn);

//     /* Signal application */
//     (void)OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);
// }

/*! *********************************************************************************
* @}
********************************************************************************** */

/*! *********************************************************************************
* \brief    NVM Application Layer Functions required by BLE host library
********************************************************************************** */

/* NVM Dataset identifiers */
#if gAppUseNvm_d
#define nvmId_BondingHeaderId_c          0x4011
#define nvmId_BondingDataDynamicId_c     0x4012
#define nvmId_BondingDataStaticId_c      0x4013
#define nvmId_BondingDataDeviceInfoId_c  0x4014
#define nvmId_BondingDataDescriptorId_c  0x4015
#endif

#if gAppUseNvm_d
#if gUnmirroredFeatureSet_d == TRUE
static bleBondIdentityHeaderBlob_t*  aBondingHeader[gMaxBondedDevices_c];
static bleBondDataDynamicBlob_t*     aBondingDataDynamic[gMaxBondedDevices_c];
static bleBondDataStaticBlob_t*      aBondingDataStatic[gMaxBondedDevices_c];
static bleBondDataDeviceInfoBlob_t*  aBondingDataDeviceInfo[gMaxBondedDevices_c];
static bleBondDataDescriptorBlob_t* aBondingDataDescriptor[gMaxBondedDevices_c * gcGapMaximumSavedCccds_c];

NVM_RegisterDataSet(aBondingHeader, gMaxBondedDevices_c, gBleBondIdentityHeaderSize_c, nvmId_BondingHeaderId_c, (uint16_t)gNVM_NotMirroredInRamAutoRestore_c);
NVM_RegisterDataSet(aBondingDataDynamic, gMaxBondedDevices_c, gBleBondDataDynamicSize_c, nvmId_BondingDataDynamicId_c, (uint16_t)gNVM_NotMirroredInRamAutoRestore_c);
NVM_RegisterDataSet(aBondingDataStatic, gMaxBondedDevices_c, gBleBondDataStaticSize_c, nvmId_BondingDataStaticId_c, (uint16_t)gNVM_NotMirroredInRamAutoRestore_c);
NVM_RegisterDataSet(aBondingDataDeviceInfo, gMaxBondedDevices_c, gBleBondDataDeviceInfoSize_c, nvmId_BondingDataDeviceInfoId_c, (uint16_t)gNVM_NotMirroredInRamAutoRestore_c);
NVM_RegisterDataSet(aBondingDataDescriptor, gMaxBondedDevices_c * gcGapMaximumSavedCccds_c, gBleBondDataDescriptorSize_c, nvmId_BondingDataDescriptorId_c, (uint16_t)gNVM_NotMirroredInRamAutoRestore_c);
#else /* mirrored dataset */
static bleBondIdentityHeaderBlob_t  aBondingHeader[gMaxBondedDevices_c];
static bleBondDataDynamicBlob_t     aBondingDataDynamic[gMaxBondedDevices_c];
static bleBondDataStaticBlob_t      aBondingDataStatic[gMaxBondedDevices_c];
static bleBondDataDeviceInfoBlob_t  aBondingDataDeviceInfo[gMaxBondedDevices_c];
static bleBondDataDescriptorBlob_t  aBondingDataDescriptor[gMaxBondedDevices_c * gcGapMaximumSavedCccds_c];
/* register datasets */
NVM_RegisterDataSet(aBondingHeader, gMaxBondedDevices_c, gBleBondIdentityHeaderSize_c, nvmId_BondingHeaderId_c, (uint16_t)gNVM_MirroredInRam_c);
NVM_RegisterDataSet(aBondingDataDynamic, gMaxBondedDevices_c, gBleBondDataDynamicSize_c, nvmId_BondingDataDynamicId_c, (uint16_t)gNVM_MirroredInRam_c);
NVM_RegisterDataSet(aBondingDataStatic, gMaxBondedDevices_c, gBleBondDataStaticSize_c, nvmId_BondingDataStaticId_c, (uint16_t)gNVM_MirroredInRam_c);
NVM_RegisterDataSet(aBondingDataDeviceInfo, gMaxBondedDevices_c, gBleBondDataDeviceInfoSize_c, nvmId_BondingDataDeviceInfoId_c, (uint16_t)gNVM_MirroredInRam_c);
NVM_RegisterDataSet(aBondingDataDescriptor, gMaxBondedDevices_c * gcGapMaximumSavedCccds_c, gBleBondDataDescriptorSize_c, nvmId_BondingDataDescriptorId_c, (uint16_t)gNVM_MirroredInRam_c);
#endif
#else
static bleBondDataBlob_t          maBondDataBlobs[gMaxBondedDevices_c] = {{{{0}}}};
#endif

/*! *********************************************************************************
* \brief  Performs NVM read operation
*
* \param[in] mEntryIdx            Bonded device index
* \param[in] pBondHeader          Bond Identity Header
* \param[in] pBondDataDynamic     Bond Data Dynamic
* \param[in] pBondDataStatic      Bond Data Static
* \param[in] pBondDataDeviceInfo  Bond Data Device Info
* \param[in] pBondDataDescriptor  Bond Data Descriptor
* \param[in] mDescriptorIndex     Descriptor Index
*
* \return  none
*
********************************************************************************** */
void App_NvmRead
(
    uint8_t  mEntryIdx,
    void*    pBondHeader,
    void*    pBondDataDynamic,
    void*    pBondDataStatic,
    void*    pBondDataDeviceInfo,
    void*    pBondDataDescriptor,
    uint8_t  mDescriptorIndex
)
{
    if(mEntryIdx >= (uint8_t)gMaxBondedDevices_c)
    {
          return;
    }
#if gAppUseNvm_d
    uint8_t  idx = 0;
#if gUnmirroredFeatureSet_d == TRUE
    uint32_t mSize = 0;
    void**   ppNvmData = NULL;
    void*    pRamData = NULL;
#endif

#if gUnmirroredFeatureSet_d == TRUE
    for(idx = 0; idx < 5U; idx++)
    {
        ppNvmData = NULL;
        switch(*(uint8_t*)&idx)
        {
        case 0:
            if(pBondHeader != NULL)
            {
                ppNvmData = (void**)&aBondingHeader[mEntryIdx];
                pRamData  = pBondHeader;
                mSize     = gBleBondIdentityHeaderSize_c;
            }
            break;
        case 1:
            if(pBondDataDynamic != NULL)
            {
                ppNvmData = (void**)&aBondingDataDynamic[mEntryIdx];
                pRamData  = pBondDataDynamic;
                mSize     = gBleBondDataDynamicSize_c;
            }
            break;
        case 2:
            if(pBondDataStatic != NULL)
            {
                ppNvmData = (void**)&aBondingDataStatic[mEntryIdx];
                pRamData  = pBondDataStatic;
                mSize     = gBleBondDataStaticSize_c;
            }
            break;
        case 3:
            if(pBondDataDeviceInfo != NULL)
            {
                ppNvmData = (void**)&aBondingDataDeviceInfo[mEntryIdx];
                pRamData  = pBondDataDeviceInfo;
                mSize     = gBleBondDataDeviceInfoSize_c;
            }
            break;
        case 4:
            if(pBondDataDescriptor != NULL)
            {
                if(mDescriptorIndex < gcGapMaximumSavedCccds_c)
                {
                    ppNvmData = (void**)&aBondingDataDescriptor[mEntryIdx * gcGapMaximumSavedCccds_c + mDescriptorIndex];
                    pRamData  = pBondDataDescriptor;
                    mSize     = gBleBondDataDescriptorSize_c;
                }
            }
            break;
        default:
            ; /* No action required */
            break;
        }

        /* if ppNvmData is not NULL the same holds for pRamData */
        if((NULL != ppNvmData) && (NULL != *ppNvmData))
        {
            FLib_MemCpy(pRamData, *ppNvmData, mSize);
        }
    }
#else // gMirroredFeatureSet_d
    for(idx = 0; idx < 5; idx++)
    {
        switch(idx)
        {
        case 0:
            if(pBondHeader != NULL)
            {
                if(gNVM_OK_c == NvRestoreDataSet((void*)&aBondingHeader[mEntryIdx], FALSE))
                {
                  FLib_MemCpy(pBondHeader, (void*)&aBondingHeader[mEntryIdx], gBleBondIdentityHeaderSize_c);
                }
            }
            break;
        case 1:
            if(pBondDataDynamic != NULL)
            {
                if(gNVM_OK_c == NvRestoreDataSet((void*)&aBondingDataDynamic[mEntryIdx], FALSE))
                {
                  FLib_MemCpy(pBondDataDynamic, (void*)&aBondingDataDynamic[mEntryIdx], gBleBondDataDynamicSize_c);
                }
            }
            break;
        case 2:
            if(pBondDataStatic != NULL)
            {
                if(gNVM_OK_c == NvRestoreDataSet((void*)&aBondingDataStatic[mEntryIdx], FALSE))
                {
                  FLib_MemCpy(pBondDataStatic, (void*)&aBondingDataStatic[mEntryIdx], gBleBondDataStaticSize_c);
                }
            }
            break;
        case 3:
            if(pBondDataDeviceInfo != NULL)
            {
                if(gNVM_OK_c == NvRestoreDataSet((void*)&aBondingDataDeviceInfo[mEntryIdx], FALSE))
                {
                  FLib_MemCpy(pBondDataDeviceInfo, (void*)&aBondingDataDeviceInfo[mEntryIdx], gBleBondDataDeviceInfoSize_c);
                }
            }
            break;
        case 4:
            if(pBondDataDescriptor != NULL)
            {
                if(mDescriptorIndex < gcGapMaximumSavedCccds_c)
                {
                    if(gNVM_OK_c == NvRestoreDataSet((void*)&aBondingDataDescriptor[mEntryIdx * gcGapMaximumSavedCccds_c + mDescriptorIndex], FALSE))
                    {
                      FLib_MemCpy(pBondDataDescriptor, (void*)&aBondingDataDescriptor[mEntryIdx * gcGapMaximumSavedCccds_c + mDescriptorIndex], gBleBondDataDescriptorSize_c);
                    }
                }
            }
            break;
        default:
            ; /* No action required */
            break;
        }
    }
#endif

#else

    if(pBondHeader != NULL)
    {
        FLib_MemCpy(pBondHeader, &maBondDataBlobs[mEntryIdx].bondHeader, gBleBondIdentityHeaderSize_c);
    }

    if(pBondDataDynamic != NULL)
    {
        FLib_MemCpy(pBondDataDynamic,
                    (uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobDynamic,
                    gBleBondDataDynamicSize_c
                        );
    }

    if(pBondDataStatic != NULL)
    {
        FLib_MemCpy(pBondDataStatic,
                    (uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobStatic,
                    gBleBondDataStaticSize_c
                        );
    }

    if(pBondDataDeviceInfo != NULL)
    {
        FLib_MemCpy(pBondDataDeviceInfo,
                    (uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobDeviceInfo,
                    gBleBondDataDeviceInfoSize_c
                        );
    }

    if(pBondDataDescriptor != NULL && mDescriptorIndex < gcGapMaximumSavedCccds_c)
    {
        FLib_MemCpy(pBondDataDescriptor,
                    (uint8_t*)&(maBondDataBlobs[mEntryIdx].bondDataDescriptors[mDescriptorIndex]),
                    gBleBondDataDescriptorSize_c
                        );
    }

#endif
}

/*! *********************************************************************************
* \brief  Performs NVM write operation
*
* \param[in] mEntryIdx            Bonded device index
* \param[in] pBondHeader          Bond Identity Header
* \param[in] pBondDataDynamic     Bond Data Dynamic
* \param[in] pBondDataStatic      Bond Data Static
* \param[in] pBondDataDeviceInfo  Bond Data Device Info
* \param[in] pBondDataDescriptor  Bond Data Descriptor
* \param[in] mDescriptorIndex     Descriptor Index
*
* \return    none
*
********************************************************************************** */
void App_NvmWrite
(
    uint8_t  mEntryIdx,
    void*    pBondHeader,
    void*    pBondDataDynamic,
    void*    pBondDataStatic,
    void*    pBondDataDeviceInfo,
    void*    pBondDataDescriptor,
    uint8_t  mDescriptorIndex
)
{
    if(mEntryIdx >= (uint8_t)gMaxBondedDevices_c)
    {
          return;
    }
#if gAppUseNvm_d
    uint8_t  idx   = 0;

#if gUnmirroredFeatureSet_d == TRUE
    uint32_t mSize = 0;
    void**   ppNvmData = NULL;
    void*    pRamData = NULL;
#endif

#if gUnmirroredFeatureSet_d == TRUE

    for(idx = 0; idx < 5U; idx++)
    {
        ppNvmData = NULL;
        switch(*(uint8_t*)&idx)
        {
        case 0:
            if(pBondHeader != NULL)
            {
                ppNvmData = (void**)&aBondingHeader[mEntryIdx];
                pRamData  = pBondHeader;
                mSize     = gBleBondIdentityHeaderSize_c;
            }
            break;
        case 1:
            if(pBondDataDynamic != NULL)
            {
                ppNvmData = (void**)&aBondingDataDynamic[mEntryIdx];
                pRamData  = pBondDataDynamic;
                mSize     = gBleBondDataDynamicSize_c;
            }
            break;
        case 2:
            if(pBondDataStatic != NULL)
            {
                ppNvmData = (void**)&aBondingDataStatic[mEntryIdx];
                pRamData  = pBondDataStatic;
                mSize     = gBleBondDataStaticSize_c;
            }
            break;
        case 3:
            if(pBondDataDeviceInfo != NULL)
            {
                ppNvmData = (void**)&aBondingDataDeviceInfo[mEntryIdx];
                pRamData  = pBondDataDeviceInfo;
                mSize     = gBleBondDataDeviceInfoSize_c;
            }
            break;
        case 4:
            if(pBondDataDescriptor != NULL)
            {
                if(mDescriptorIndex < gcGapMaximumSavedCccds_c)
                {
                    ppNvmData = (void**)&aBondingDataDescriptor[mEntryIdx * gcGapMaximumSavedCccds_c + mDescriptorIndex];
                    pRamData  = pBondDataDescriptor;
                    mSize     = gBleBondDataDescriptorSize_c;
                }
            }
            break;
        default:
            ; /* No action required */
            break;
        }

        if(ppNvmData != NULL)
        {
            if(gNVM_OK_c == NvMoveToRam(ppNvmData))
            {
                FLib_MemCpy(*ppNvmData, pRamData, mSize);
                (void)NvSaveOnIdle(ppNvmData, FALSE);
            }
            else
            {
                *ppNvmData = pRamData;
                (void)NvSyncSave(ppNvmData, FALSE);
            }
        }
    }
#else // gMirroredFeatureSet_d

    for(idx = 0; idx < 5; idx++)
    {
        switch(idx)
        {
        case 0:
            if(pBondHeader != NULL)
            {
                FLib_MemCpy((void*)&aBondingHeader[mEntryIdx], pBondHeader, gBleBondIdentityHeaderSize_c);
                (void)NvSaveOnIdle((void*)&aBondingHeader[mEntryIdx], FALSE);
            }
            break;
        case 1:
            if(pBondDataDynamic != NULL)
            {
                FLib_MemCpy((void*)&aBondingDataDynamic[mEntryIdx], pBondDataDynamic, gBleBondDataDynamicSize_c);
                (void)NvSaveOnIdle((void*)&aBondingDataDynamic[mEntryIdx], FALSE);
            }
            break;
        case 2:
            if(pBondDataStatic != NULL)
            {
                FLib_MemCpy((void*)&aBondingDataStatic[mEntryIdx], pBondDataStatic, gBleBondDataStaticSize_c);
                (void)NvSaveOnIdle((void*)&aBondingDataStatic[mEntryIdx], FALSE);
            }
            break;
        case 3:
            if(pBondDataDeviceInfo != NULL)
            {
                FLib_MemCpy((void*)&aBondingDataDeviceInfo[mEntryIdx], pBondDataDeviceInfo, gBleBondDataDeviceInfoSize_c);
                (void)NvSaveOnIdle((void*)&aBondingDataDeviceInfo[mEntryIdx], FALSE);
            }
            break;
        case 4:
            if(pBondDataDescriptor != NULL)
            {
                if(mDescriptorIndex < gcGapMaximumSavedCccds_c)
                {
                    FLib_MemCpy((void*)&aBondingDataDescriptor[mEntryIdx * gcGapMaximumSavedCccds_c + mDescriptorIndex], pBondDataDescriptor, gBleBondDataDescriptorSize_c);
                    (void)NvSaveOnIdle((void*)&aBondingDataDescriptor[mEntryIdx * gcGapMaximumSavedCccds_c + mDescriptorIndex], FALSE);
                }
            }
            break;
        default:
            ; /* No action required */
            break;
        }
    }

#endif //gUnmirroredFeatureSet_d

#else

    if(pBondHeader != NULL)
    {
        FLib_MemCpy(&maBondDataBlobs[mEntryIdx].bondHeader, pBondHeader, gBleBondIdentityHeaderSize_c);
    }

    if(pBondDataDynamic != NULL)
    {
        FLib_MemCpy((uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobDynamic,
                    pBondDataDynamic,
                    gBleBondDataDynamicSize_c
                        );
    }

    if(pBondDataStatic != NULL)
    {
        FLib_MemCpy((uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobStatic,
                    pBondDataStatic,
                    gBleBondDataStaticSize_c
                        );
    }

    if(pBondDataDeviceInfo != NULL)
    {
        FLib_MemCpy((uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobDeviceInfo,
                    pBondDataDeviceInfo,
                    gBleBondDataDeviceInfoSize_c
                        );
    }

    if(pBondDataDescriptor != NULL && mDescriptorIndex != gcGapMaximumSavedCccds_c)
    {
        FLib_MemCpy((uint8_t*)&(maBondDataBlobs[mEntryIdx].bondDataDescriptors[mDescriptorIndex]),
                    pBondDataDescriptor,
                    gBleBondDataDescriptorSize_c
                        );
    }

#endif
}

/*! *********************************************************************************
* \brief  Performs NVM erase operation
*
* \param[in] mEntryIdx Bonded device index
*
* \return  none
*
********************************************************************************** */
void App_NvmErase(uint8_t mEntryIdx)
{
    if(mEntryIdx >= (uint8_t)gMaxBondedDevices_c)
    {
          return;
    }
#if gAppUseNvm_d
#if gUnmirroredFeatureSet_d == TRUE
    (void)NvErase((void**)&aBondingHeader[mEntryIdx]);
    (void)NvErase((void**)&aBondingDataDynamic[mEntryIdx]);
    (void)NvErase((void**)&aBondingDataStatic[mEntryIdx]);
    (void)NvErase((void**)&aBondingDataDeviceInfo[mEntryIdx]);

    uint32_t mDescIdx;

    for(mDescIdx = ((uint32_t)mEntryIdx * gcGapMaximumSavedCccds_c);
        mDescIdx < ((uint32_t)mEntryIdx + 1U) * gcGapMaximumSavedCccds_c; mDescIdx++)
    {
        (void)NvErase((void**)&aBondingDataDescriptor[mDescIdx]);
    }
#else // mirrored
    FLib_MemSet(&aBondingHeader[mEntryIdx], 0, gBleBondIdentityHeaderSize_c);
    NvSaveOnIdle((void*)&aBondingHeader[mEntryIdx], FALSE);
    FLib_MemSet(&aBondingDataDynamic[mEntryIdx], 0, gBleBondDataDynamicSize_c);
    NvSaveOnIdle((void*)&aBondingDataDynamic[mEntryIdx], FALSE);
    FLib_MemSet(&aBondingDataStatic[mEntryIdx], 0, gBleBondDataStaticSize_c);
    NvSaveOnIdle((void*)&aBondingDataStatic[mEntryIdx], FALSE);
    FLib_MemSet(&aBondingDataDeviceInfo[mEntryIdx], 0, gBleBondDataDeviceInfoSize_c);
    NvSaveOnIdle((void*)&aBondingDataDeviceInfo[mEntryIdx], FALSE);

    uint32_t mDescIdx;

    for(mDescIdx = ((uint32_t)mEntryIdx * gcGapMaximumSavedCccds_c);
        mDescIdx < ((uint32_t)mEntryIdx + 1U) * gcGapMaximumSavedCccds_c; mDescIdx++)
    {
        FLib_MemSet(&aBondingDataDescriptor[mDescIdx], 0, gBleBondDataDescriptorSize_c);
        NvSaveOnIdle((void*)&aBondingDataDescriptor[mDescIdx], FALSE);
    }
#endif
#else
    FLib_MemSet(&maBondDataBlobs[mEntryIdx], 0, sizeof(bleBondDataBlob_t));
#endif
}

/* [] END OF FILE */
