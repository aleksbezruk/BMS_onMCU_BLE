/**
 * @file BLE_qn9080.c
 * @brief Source file for BLE_qn9080 module
 * @note The BLE_qn9080 module is used to evaluate and bring up the BLE functionality
 *       on the QN9080 platform.
 * @details The module uses the QN9080's BLE stack and its APIs to manage advertisements and connections.
 * @version 0.1.0
 */

#include "BLE_qn9080.h"

// FSL includes
#include "MemManager.h"
#include "SecLib.h"
#include "RNG_Interface.h"
#if gAppUseNvm_d
#include "NVM_Interface.h"
#endif // gAppUseNvm_d

// HAL includes
#include "hal.h"
#include "hal_led.h"

// QSPY for debugging
#include "qs.h"
#include "MAIN.h"

// NXP BLE includes
#include "controller_interface.h"
#include "ble_controller_task_config.h"
#include "ble_general.h"

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

/*! *********************************************************************************
 * \brief  GAP Generic Event Callback
 * \param[in] pGenericEvent GAP Generic Event
 ********************************************************************************** */
static void BLE_GenericCallback(gapGenericEvent_t* pGenericEvent)
{
    if (pGenericEvent == NULL) {
        return;
    }

    QS_BEGIN_ID(MAIN, 0)
        QS_STR("BLE GAP Event received: type=");
        QS_U32(0, pGenericEvent->eventType);
    QS_END()
    QS_FLUSH();

    switch (pGenericEvent->eventType) {
        case gInitializationComplete_c:
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Host Initialization Complete!");
            QS_END()
            QS_FLUSH();
            break;
            
        case gPublicAddressRead_c:
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Public Address Read");
            QS_END()
            QS_FLUSH();
            break;
            
        default:
            QS_BEGIN_ID(MAIN, 0)
                QS_STR("BLE Unhandled GAP event: ");
                QS_U32(0, pGenericEvent->eventType);
            QS_END()
            QS_FLUSH();
            break;
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
        QS_FLUSH();
        
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
    QS_FLUSH();

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
    // SecLib_Init(); // Commented out - only needed for encryption/pairing
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
    HAL_LED_red_On();  // Turn on red LED before BLE_Init
    __asm("DSB"); // Data Synchronization Barrier
    __asm("ISB"); // Instruction Synchronization Barrier
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Starting proper NXP BLE stack initialization sequence");
    QS_END()
    QS_FLUSH();
    
    // Step 1: Initialize BLE Controller with HCI callback
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 1: Calling Controller_Init with HCI callback");
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
    
    // Step 2: Initialize BLE Host with callbacks
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 2: Calling Ble_HostInitialize with callbacks");
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
    
    // Step 3: Initialize the BLE controller task
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("Step 3: Calling Controller_TaskInit");
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
    
    QS_BEGIN_ID(MAIN, 0)
        QS_STR("NXP BLE stack initialization sequence completed successfully!");
        QS_STR(" Ready for connections=");
        QS_U8(0, gAppMaxConnections_c);
    QS_END()
    QS_FLUSH();
    
    HAL_LED_red_Off(); // Turn off red LED after successful BLE_Init
}

/* [] END OF FILE */