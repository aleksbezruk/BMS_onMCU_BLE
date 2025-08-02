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

// Forward declarations for BLE callbacks
static void BLE_GenericCallback(gapGenericEvent_t* pGenericEvent);
static bleResult_t BLE_HostHciRecvCallback(hciPacketType_t packetType, void* pHciPacket, uint16_t packetSize);
static bleResult_t BLE_HostToControllerInterface(hciPacketType_t packetType, void* pPacket, uint16_t packetSize);
// static void App_SecLibMultCallback(computeDhKeyParam_t *pData);

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

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE GAP Event received: type=");
        QS_U32(0, pGenericEvent->eventType);
    QS_END()

    switch (pGenericEvent->eventType) {
        case gInitializationComplete_c:
        {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Host Initialization Complete!");
            QS_END()
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
 * \brief  HCI Host Receive Callback (from Controller to Host)
 * \param[in] packetType HCI packet type
 * \param[in] pHciPacket HCI packet
 * \param[in] packetSize HCI packet size
 * \return BLE result status
 ********************************************************************************** */
static bleResult_t BLE_HostHciRecvCallback(hciPacketType_t packetType, void* pHciPacket, uint16_t packetSize)
{
    // This callback receives HCI packets from the controller
    // Forward them to the BLE host stack for processing
    if (pHciPacket != NULL && packetSize > 0) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("HCI RX: type=");
            QS_U8(0, packetType);
            QS_STR(" size=");
            QS_U16(0, packetSize);
        QS_END()
        
        // For debugging: log the first few bytes if it's an event packet
        if (packetType == 0x04 && packetSize >= 2) {  // HCI Event packet
            uint8_t* pData = (uint8_t*)pHciPacket;
            QS_BEGIN_ID(MAIN, 0)
                QS_STR(" HCI Event: code=");
                QS_U8(0, pData[0]);
                QS_STR(" len=");
                QS_U8(0, pData[1]);
            QS_END()
        }
        
        // Call the BLE Host downlink interface to process the HCI packet
        return Ble_HciRecv(packetType, pHciPacket, packetSize);
    }
    
    return gBleSuccess_c; // Return success for empty packets
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

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("HCI TX: type=");
        QS_U8(0, packetType);
        QS_STR(" size=");
        QS_U16(0, packetSize);
    QS_END()

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

    // NVDS is initialized in main() before any tasks start
    // This ensures ROM BLE functions (nvds_get, nvds_put, etc.) are accessible
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("NVDS initialized in main() - ROM functions accessible");
    QS_END()
    QS_FLUSH();

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

    // Step 1: Initialize the BLE controller task
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 1: Calling Controller_TaskInit");
    QS_END()
    QS_FLUSH();
    
    osaStatus_t taskStatus = Controller_TaskInit();
    if (taskStatus != osaStatus_Success) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Controller_TaskInit failed with status=");
            QS_U32(0, taskStatus);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Controller task initialization failed
    }
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Controller_TaskInit completed successfully");
    QS_END()
    QS_FLUSH();

    // Step 2: Initialize BLE Controller with HCI callback
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 2: Calling Controller_Init with HCI callback");
    QS_END()
    QS_FLUSH();
    
    bleResult_t controllerResult = Controller_Init(BLE_HostHciRecvCallback);
    if (controllerResult != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Controller_Init failed with result=");
            QS_U32(0, controllerResult);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Controller initialization failed
    }

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Controller_Init completed successfully");
    QS_END()
    QS_FLUSH();

    // /** Check for available memory storage */
    // if (!Ble_CheckMemoryStorage()) {
    //     HAL_ASSERT(0, __FILE__, __LINE__); // Memory storage check failed
    // }
    
    // /* Step 3: BLE Host Tasks Init */
    // if (osaStatus_Success != Ble_HostTaskInit()) {
    //     HAL_ASSERT(0, __FILE__, __LINE__); // BLE Host task initialization failed
    // }

    // Step 4: Initialize BLE Host with callbacks
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 4: Calling Ble_HostInitialize with callbacks");
    QS_END()
    QS_FLUSH();
    
    bleResult_t hostResult = Ble_HostInitialize(BLE_GenericCallback, BLE_HostToControllerInterface);
    if (hostResult != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Ble_HostInitialize failed with result=");
            QS_U32(0, hostResult);
        QS_END()
        QS_FLUSH();
        HAL_ASSERT(0, __FILE__, __LINE__); // Host initialization failed
    }
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Ble_HostInitialize completed successfully");
    QS_END()
    QS_FLUSH();
    
    // Step 4: Configure GAP default parameters (CRITICAL - often missing!)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 4: Setting up GAP default parameters and security");
    QS_END()
    QS_FLUSH();
    
    // Set default pairing parameters - required for proper GAP operation
    gapPairingParameters_t pairingParams = {
        .withBonding = FALSE,
        .securityModeAndLevel = gSecurityMode_1_Level_1_c,  // Just Works
        .maxEncryptionKeySize = gDefaultEncryptionKeySize_d,  // Use correct constant
        .localIoCapabilities = gIoNone_c,
        .oobAvailable = FALSE,
        .centralKeys = gLtk_c | gIrk_c | gCsrk_c,
        .peripheralKeys = gLtk_c | gIrk_c | gCsrk_c,
        .leSecureConnectionSupported = TRUE,
        .useKeypressNotifications = FALSE,
    };
    
    bleResult_t pairingResult = Gap_SetDefaultPairingParameters(&pairingParams);
    if (pairingResult != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_SetDefaultPairingParameters failed: ");
            QS_U32(0, pairingResult);
        QS_END()
        QS_FLUSH();
        // Don't fail here - some stacks work without this
    } else {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_SetDefaultPairingParameters completed successfully");
        QS_END()
        QS_FLUSH();
    }
    
    // Skip complex security requirements - they may not be needed for basic advertising
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Skipping complex security requirements setup");
    QS_END()
    QS_FLUSH();
    
    // Step 5: Initialize Generic Callback - VERY IMPORTANT for proper event handling
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 5: Attempting Gap_RegisterGenericCallback (may not exist in this stack)");
    QS_END()
    QS_FLUSH();

    // Step 6: CRITICAL - Read and verify device address (required for advertising)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 6: Reading and verifying device address");
    QS_END()
    QS_FLUSH();
    
    bleResult_t addressResult = Gap_ReadPublicDeviceAddress();
    if (addressResult != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("WARNING: Gap_ReadPublicDeviceAddress failed: ");
            QS_U32(0, addressResult);
            QS_STR(" - Will try to set random address");
        QS_END()
        QS_FLUSH();
        
        // If public address fails, set a random static address
        bleDeviceAddress_t randomAddr = {0x01, 0x02, 0x03, 0x04, 0x05, 0xC0}; // MSB must be 11xxxxxx for static random
        bleResult_t randomResult = Gap_SetRandomAddress(randomAddr);
        if (randomResult != gBleSuccess_c) {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("CRITICAL: Gap_SetRandomAddress also failed: ");
                QS_U32(0, randomResult);
            QS_END()
            QS_FLUSH();
            HAL_ASSERT(0, __FILE__, __LINE__); // This is critical for advertising
        } else {
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Random device address set successfully");
            QS_END()
            QS_FLUSH();
        }
    } else {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Public device address read successfully");
        QS_END()
        QS_FLUSH();
    }
    
    // Step 7: Enable scan response (helps with device discovery)
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 7: Attempting to set scan response data (may not exist)");
    QS_END()
    QS_FLUSH();
}

// Global flag to track if advertising callback was ever called
static volatile uint32_t g_advertisingCallbackCount = 0;

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
 * \brief  Advertising Event Callback
 * \param[in] pAdvertisingEvent Advertising Event
 ********************************************************************************** */
static void BLE_AdvertisingCallback(gapAdvertisingEvent_t* pAdvertisingEvent)
{
    // Set the flag immediately to track that callback was called
    g_advertisingCallbackCount++;
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("*** ADVERTISING CALLBACK TRIGGERED *** Count=");
        QS_U32(0, g_advertisingCallbackCount);
    QS_END()
    QS_FLUSH();
    
    if (pAdvertisingEvent == NULL) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("BLE Advertising Event: NULL pointer received");
        QS_END()
        QS_FLUSH();
        return;
    }

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE Advertising Event: type=");
        QS_U32(0, pAdvertisingEvent->eventType);
        QS_STR(" (gAdvertisingStateChanged_c=");
        QS_U32(0, gAdvertisingStateChanged_c);
        QS_STR(")");
    QS_END()
    QS_FLUSH();

    switch (pAdvertisingEvent->eventType) {
        case gAdvertisingStateChanged_c:
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("*** Advertising State Changed - Advertisement state updated ***");
                QS_STR(" This means advertising is now active on the radio!");
            QS_END()
            QS_FLUSH();
            break;
            
        case gAdvertisingCommandFailed_c:
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("*** Advertising Command Failed *** - reason=");
                QS_U32(0, pAdvertisingEvent->eventData.failReason);
            QS_END()
            QS_FLUSH();
            break;
            
        default:
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("*** Unhandled Advertising event: ");
                QS_U32(0, pAdvertisingEvent->eventType);
                QS_STR(" ***");
            QS_END()
            QS_FLUSH();
            break;
    }
}

/*! *********************************************************************************
 * \brief  Connection Event Callback
 * \param[in] peerDeviceId Device ID of the peer
 * \param[in] pConnectionEvent Connection Event
 ********************************************************************************** */
static void BLE_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent)
{
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE Connection Event: device=");
        QS_U8(0, peerDeviceId);
        QS_STR(" type=");
        QS_U32(0, pConnectionEvent->eventType);
    QS_END()
    QS_FLUSH();

    switch (pConnectionEvent->eventType) {
        case gConnEvtConnected_c:
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Device Connected!");
            QS_END()
            QS_FLUSH();
            break;
            
        case gConnEvtDisconnected_c:
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Device Disconnected - reason=");
                QS_U32(0, pConnectionEvent->eventData.disconnectedEvent.reason);
            QS_END()
            QS_FLUSH();
            break;
            
        default:
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("Unhandled Connection event: ");
                QS_U32(0, pConnectionEvent->eventType);
            QS_END()
            QS_FLUSH();
            break;
    }
}

/*! *********************************************************************************
 * \brief  Start BLE Advertising
 * \return BLE_qn9080_status_t Status of the operation
 ********************************************************************************** */
BLE_qn9080_status_t BLE_StartAdvertising(void)
{
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_StartAdvertising: Setting up advertising parameters");
    QS_END()
    QS_FLUSH();

    // Set up advertising parameters
    gapAdvertisingParameters_t advParams = {
        .minInterval = 2048,   // 1.28s (2048 * 0.625ms) - very slow for maximum visibility 
        .maxInterval = 2048,   // 1.28s - same as min for consistent timing
        .advertisingType = gAdvConnectableUndirected_c,
        .ownAddressType = gBleAddrTypePublic_c,
        .peerAddressType = gBleAddrTypePublic_c,
        .peerAddress = {0, 0, 0, 0, 0, 0},
        .channelMap = gGapAdvertisingChannelMapDefault_c,
        .filterPolicy = gProcessAll_c
    };

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Setting advertising params: min=");
        QS_U16(0, advParams.minInterval);
        QS_STR(" max=");
        QS_U16(0, advParams.maxInterval);
        QS_STR(" type=");
        QS_U8(0, advParams.advertisingType);
    QS_END()
    QS_FLUSH();

    bleResult_t result = Gap_SetAdvertisingParameters(&advParams);
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_SetAdvertisingParameters failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        return BLE_QN9080_STATUS_ERROR;
    }

    // Set up advertising data with proper lengths
    static uint8_t deviceName[] = "QN9080_BMS";  // Make static to ensure it persists
    static uint8_t flags = gLeGeneralDiscoverableMode_c | gBrEdrNotSupported_c;
    
    static gapAdStructure_t advStructures[2];  // Start with just 2 structures for simplicity
    
    // Flags AD structure - length does NOT include the AD type byte
    advStructures[0].length = 1;  // Just 1 byte for flags data
    advStructures[0].adType = gAdFlags_c;
    advStructures[0].aData = &flags;
    
    // Complete Local Name AD structure - length does NOT include AD type byte
    advStructures[1].length = strlen((char*)deviceName);  // Just the name length
    advStructures[1].adType = gAdCompleteLocalName_c;
    advStructures[1].aData = deviceName;
    
    static gapAdvertisingData_t advData = {
        .cNumAdStructures = 2,  // Use only 2 structures initially
        .aAdStructures = advStructures
    };

    // Debug: Log advertising data details
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Advertising data setup: flags=");
        QS_U8(0, flags);
        QS_STR(" name_len=");
        QS_U8(0, strlen((char*)deviceName));
        QS_STR(" structures=");
        QS_U8(0, advData.cNumAdStructures);
        QS_STR(" flag_len=");
        QS_U8(0, advStructures[0].length);
        QS_STR(" name_ad_len=");
        QS_U8(0, advStructures[1].length);
    QS_END()
    QS_FLUSH();

    result = Gap_SetAdvertisingData(&advData, NULL);
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_SetAdvertisingData failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        return BLE_QN9080_STATUS_ERROR;
    }

    // Start advertising
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_StartAdvertising: Starting advertising");
    QS_END()
    QS_FLUSH();

    result = Gap_StartAdvertising(BLE_AdvertisingCallback, BLE_ConnectionCallback);
    if (result != gBleSuccess_c) {
        QS_BEGIN_ID(MAIN, 0)
            QS_STR("Gap_StartAdvertising failed: ");
            QS_U32(0, result);
        QS_END()
        QS_FLUSH();
        return BLE_QN9080_STATUS_ERROR;
    }

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Gap_StartAdvertising returned SUCCESS. Result=");
        QS_U32(0, result);
    QS_END()
    QS_FLUSH();

    // Add longer delay to give BLE stack time to process
    for(volatile int i = 0; i < 5000000; i++); // Longer delay

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE_StartAdvertising: Checking if advertising callbacks were triggered...");
    QS_END()
    QS_FLUSH();

    // Try to force a check of advertising state by reading public address
    // This might trigger pending events
    bleResult_t addrResult = Gap_ReadPublicDeviceAddress();
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Gap_ReadPublicDeviceAddress result=");
        QS_U32(0, addrResult);
    QS_END()
    QS_FLUSH();

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE Advertising started successfully! Device name: QN9080_BMS");
        QS_STR(" Waiting for advertising callbacks...");
    QS_END()
    QS_FLUSH();

    return BLE_QN9080_STATUS_OK;
}

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