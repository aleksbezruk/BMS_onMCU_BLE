/**
 * @file gatt_db.h
 * @brief GATT Database definitions for BMS application
 * @note Static GATT database definition using NXP macros
 * @details Contains GAP, Battery, BMS, and Device Information services
 * @version 0.1.0
 */

#ifndef GATT_DB_H
#define GATT_DB_H

#include "gatt_database.h"

// Required constants - use conditional definitions to avoid redefinition warnings
#ifndef MANUFACTURER_NAME
#define MANUFACTURER_NAME "NXP Semiconductors"
#endif

#ifndef BOARD_NAME
#define BOARD_NAME "QN9080CDK"
#endif

#endif /* GATT_DB_H */

// GATT Database Definition - Outside of header guard to allow multiple inclusions
PRIMARY_SERVICE(service_gap, gBleSig_GenericAccessProfile_d)
    CHARACTERISTIC(char_device_name, gBleSig_GapDeviceName_d, (gGattCharPropRead_c | gGattCharPropWrite_c) )
            VALUE(value_device_name, gBleSig_GapDeviceName_d, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), 11, "QN9080_BMS")
    CHARACTERISTIC(char_appearance, gBleSig_GapAppearance_d, (gGattCharPropRead_c) )
            VALUE(value_appearance, gBleSig_GapAppearance_d, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
    CHARACTERISTIC(char_ppcp, gBleSig_GapPpcp_d, (gGattCharPropRead_c) )
            VALUE(value_ppcp, gBleSig_GapPpcp_d, (gPermissionFlagReadable_c), 8, 0x0A, 0x00, 0x10, 0x00, 0x64, 0x00, 0xE2, 0x04)

PRIMARY_SERVICE(service_battery, gBleSig_BatteryService_d)
    CHARACTERISTIC(char_battery_level, gBleSig_BatteryLevel_d, (gGattCharPropNotify_c | gGattCharPropRead_c))
        VALUE(value_battery_level, gBleSig_BatteryLevel_d, (gPermissionFlagReadable_c), 1, 0x5A)
        DESCRIPTOR(desc_bat_level, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x04, 0x00, 0xAD, 0x27, 0x01, 0x01, 0x00)
        CCCD(cccd_battery_level)
