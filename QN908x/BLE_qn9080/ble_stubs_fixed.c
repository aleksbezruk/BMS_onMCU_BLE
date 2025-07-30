/**
 * @file ble_stubs.c
 * @brief Stub implementations for missing BLE library dependencies
 * @note These stubs provide minimal implementations to satisfy linker requirements
 * @version 0.1.0
 */

#include "EmbeddedTypes.h"
#include "ble_general.h"

// Task queue globals - these are expected by the BLE library
void* gHci2Host_TaskQueue = NULL;
uint8_t gHost_TaskEvent = 0;
void* gApp2Host_TaskQueue = NULL;
bool_t gHostInitResetController = FALSE;

// GATT database stubs
typedef struct {
    uint16_t handle;
    uint16_t uuid;
    uint8_t permissions;
    uint16_t maxValueLength;
    uint16_t valueLength;
    uint8_t* pValue;
} gattDbAttribute_t;

const gattDbAttribute_t gattDatabase[1] = {{0}};
const uint16_t gGattDbAttributeCount_c = 0;

bleResult_t GattDb_Init(void)
{
    // Return success - no actual database initialization needed
    return gBleSuccess_c;
}

uint16_t GattDb_GetIndexOfHandle(uint16_t handle)
{
    // Return invalid index - no database
    (void)handle;
    return 0xFFFF; // gGattDbInvalidHandleIndex_d equivalent
}

// NVM stubs for device database
bleResult_t App_NvmRead(uint16_t nvmId, void* pData, uint16_t dataSize, uint16_t* pBytesRead)
{
    // Simulate empty NVM - no bonding data
    (void)nvmId;
    (void)pData;
    (void)dataSize;
    if (pBytesRead != NULL) {
        *pBytesRead = 0;
    }
    return gBleUnavailable_c; // Return error to indicate no data
}

bleResult_t App_NvmWrite(uint16_t nvmId, void* pData, uint16_t dataSize)
{
    // Simulate successful write but don't actually store anything
    (void)nvmId;
    (void)pData;
    (void)dataSize;
    return gBleSuccess_c;
}

bleResult_t App_NvmErase(uint16_t nvmId)
{
    // Simulate successful erase
    (void)nvmId;
    return gBleSuccess_c;
}
