/**
 * @file BLE_qn9080.c
 * @brief Source file for BLE_qn9080 module
 * @note The BLE_qn9080 module is used to evaluate and bring up the BLE functionality
 *       on the QN9080 platform.
 * @details The module uses the QN9080's BLE stack and its APIs to manage advertisements and connections.
 * @version 0.1.0
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

// =====================
// Defines
// =====================
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
    }
};

static gapAdvertisingData_t g_advData = {
    .cNumAdStructures = sizeof(advScanStruct) / sizeof(gapAdStructure_t),
    .aAdStructures = (gapAdStructure_t*)advScanStruct
};

static gapScanResponseData_t g_scanRspData = {
    .cNumAdStructures = 0,
    .aAdStructures = NULL
};

#if BLE_EXTENDED_LOGS == 1
static volatile uint32_t g_advertisingCallbackCount = 0;
#endif // BLE_EXTENDED_LOGS

// Forward declarations for BLE callbacks
static bleResult_t BLE_HostToControllerInterface(hciPacketType_t packetType, void* pPacket, uint16_t packetSize);
// static void App_SecLibMultCallback(computeDhKeyParam_t *pData);

// Forward declarations for advertising functions
static void BLE_SetupAdvertisingParameters(void);
static void BLE_SetupAdvertisingData(void);
static void BLE_StartAdvertisingInternal(void);
static void BLE_AdvertisingCallback(gapAdvertisingEvent_t* pAdvertisingEvent);
static void BLE_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent);

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
                QS_STR("BLE Host Initialization Complete! Setting up advertising parameters...");
            QS_END()
            
            // Step 1: Set advertising parameters when initialization is complete
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
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Advertising Data Setup Complete! Starting advertising...");
            QS_END()
            
            // Step 3: Start advertising when data is set
            BLE_StartAdvertisingInternal();
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

    // /** Step 5: Configure GAP default parameters (CRITICAL - often missing!) */    
    // // Set default pairing parameters - required for proper GAP operation
    // gapPairingParameters_t pairingParams = {
    //     .withBonding = FALSE,
    //     .securityModeAndLevel = gSecurityMode_1_Level_1_c,  // Just Works
    //     .maxEncryptionKeySize = gDefaultEncryptionKeySize_d,  // Use correct constant
    //     .localIoCapabilities = gIoNone_c,
    //     .oobAvailable = FALSE,
    //     .centralKeys = gLtk_c | gIrk_c | gCsrk_c,
    //     .peripheralKeys = gLtk_c | gIrk_c | gCsrk_c,
    //     .leSecureConnectionSupported = TRUE,
    //     .useKeypressNotifications = FALSE,
    // };
    
    // bleResult_t pairingResult = Gap_SetDefaultPairingParameters(&pairingParams);
    // if (pairingResult != gBleSuccess_c) {
    //     QS_BEGIN_ID(MAIN, 0)
    //         QS_STR("Gap_SetDefaultPairingParameters failed: ");
    //         QS_U32(0, pairingResult);
    //     QS_END()
    //     QS_FLUSH();
    //     // Don't fail here - some stacks work without this
    // } else {
    //     QS_BEGIN_ID(MAIN, 0)
    //         QS_STR("Gap_SetDefaultPairingParameters completed successfully");
    //     QS_END()
    //     QS_FLUSH();
    // }

    // /** Step 6: CRITICAL - Read and verify device address (required for advertising) */
    // bleResult_t addressResult = Gap_ReadPublicDeviceAddress();
    // if (addressResult != gBleSuccess_c) {
    //     QS_BEGIN_ID(MAIN, 0)
    //         QS_STR("WARNING: Gap_ReadPublicDeviceAddress failed: ");
    //         QS_U32(0, addressResult);
    //         QS_STR(" - Will try to set random address");
    //     QS_END()
    //     QS_FLUSH();
        
    //     // If public address fails, set a random static address
    //     bleDeviceAddress_t randomAddr = {0x01, 0x02, 0x03, 0x04, 0x05, 0xC0}; // MSB must be 11xxxxxx for static random
    //     bleResult_t randomResult = Gap_SetRandomAddress(randomAddr);
    //     if (randomResult != gBleSuccess_c) {
    //         QS_BEGIN_ID(MAIN, 0)
    //             QS_STR("CRITICAL: Gap_SetRandomAddress also failed: ");
    //             QS_U32(0, randomResult);
    //         QS_END()
    //         QS_FLUSH();
    //         HAL_ASSERT(0, __FILE__, __LINE__); // This is critical for advertising
    //     } else {
    //         QS_BEGIN_ID(MAIN, 0)
    //             QS_STR("Random device address set successfully");
    //         QS_END()
    //         QS_FLUSH();
    //     }
    // } else {
    //     QS_BEGIN_ID(MAIN, 0)
    //         QS_STR("Public device address read successfully");
    //     QS_END()
    //     QS_FLUSH();
    // }

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
        QS_STR(" device_name='QN9080_BMS'");
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
        HAL_ASSERT(0, __FILE__, __LINE__); // Critical failure
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
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("*** Advertising State Changed ***");
            QS_END()
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
                QS_STR("*** Unhandled Advertising event: ");
                QS_U32(0, pAdvertisingEvent->eventType);
                QS_STR(" ***");
            QS_END()
            break;
        }
    }
}

/*! *********************************************************************************
 * \brief  Connection Event Callback
 * \param[in] peerDeviceId Device ID of the peer
 * \param[in] pConnectionEvent Connection Event
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
            break;
        }
            
        case gConnEvtDisconnected_c:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Device Disconnected - reason=");
                QS_U32(0, pConnectionEvent->eventData.disconnectedEvent.reason);
                QS_STR(" - Restarting advertising...");
            QS_END()
            
            // Restart advertising after disconnect
            bleResult_t result = Gap_StartAdvertising(BLE_AdvertisingCallback, BLE_ConnectionCallback);
            if (result != gBleSuccess_c) {
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Failed to restart advertising after disconnect: ");
                    QS_U32(0, result);
                QS_END()
                QS_FLUSH();
            } else {
                QS_BEGIN_ID(MAIN, 0)
                    QS_STR("Advertising restarted successfully after disconnect");
                QS_END()
            }
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
    QS_FLUSH();

    bleResult_t result = Gap_StopAdvertising();
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_StopAdvertising failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        return BLE_QN9080_STATUS_ERROR;
    }

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE Advertising stopped successfully");
    QS_END()
    QS_FLUSH();

    return BLE_QN9080_STATUS_OK;
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

/* [] END OF FILE */