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

PRIMARY_SERVICE(service_automation_io, gBleSig_AutomationIo_d)
    // Digital IO Value - Switch states with notification support (matches PSOC63 index 5)
    CHARACTERISTIC(char_digital_io_value, gBleSig_Digital_d, (gGattCharPropRead_c | gGattCharPropWrite_c | gGattCharPropNotify_c))
        VALUE(value_digital_io_value, gBleSig_Digital_d, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), 1, 0x00)
        DESCRIPTOR(desc_digital_io_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00)
        DESCRIPTOR(desc_digital_io_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 17, "Switch Control")
        CCCD(cccd_digital_io_value)
    
    // Analog Input - Full Battery Voltage (matches PSOC63 index 8)
    CHARACTERISTIC(char_analog_full_battery, gBleSig_Analog_d, (gGattCharPropRead_c))
        VALUE(value_analog_full_battery, gBleSig_Analog_d, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_full_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x06, 0x00, 0x72, 0x27, 0x01, 0x00, 0x00)
        DESCRIPTOR(desc_analog_full_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 12, "Full Battery")
    
    // Analog Input - Bank 1 Voltage (matches PSOC63 index 10)
    CHARACTERISTIC(char_analog_bank1, gBleSig_Analog_d, (gGattCharPropRead_c))
        VALUE(value_analog_bank1, gBleSig_Analog_d, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank1_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x06, 0x00, 0x72, 0x27, 0x01, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank1_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 11, "Bank 1 Volt")
    
    // Analog Input - Bank 2 Voltage (matches PSOC63 index 12)
    CHARACTERISTIC(char_analog_bank2, gBleSig_Analog_d, (gGattCharPropRead_c))
        VALUE(value_analog_bank2, gBleSig_Analog_d, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank2_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x06, 0x00, 0x72, 0x27, 0x01, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank2_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 11, "Bank 2 Volt")
    
    // Analog Input - Bank 3 Voltage (matches PSOC63 index 14)
    CHARACTERISTIC(char_analog_bank3, gBleSig_Analog_d, (gGattCharPropRead_c))
        VALUE(value_analog_bank3, gBleSig_Analog_d, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank3_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x06, 0x00, 0x72, 0x27, 0x01, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank3_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 11, "Bank 3 Volt")
    
    // Analog Input - Bank 4 Voltage (matches PSOC63 index 16)
    CHARACTERISTIC(char_analog_bank4, gBleSig_Analog_d, (gGattCharPropRead_c))
        VALUE(value_analog_bank4, gBleSig_Analog_d, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank4_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x06, 0x00, 0x72, 0x27, 0x01, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank4_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 11, "Bank 4 Volt")
