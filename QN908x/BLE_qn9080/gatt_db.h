/**
 * @file gatt_db.h
 * @brief GATT Database definitions for BMS application
 * @note Static GATT database definition using NXP macros
 * @details Contains GAP, Battery, BMS, and Device Information services
 * @version 0.7.0
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
        VALUE(value_appearance, gBleSig_GapAppearance_d, (gPermissionFlagReadable_c), 2, 0xC1, 0x03)
    CHARACTERISTIC(char_ppcp, gBleSig_GapPpcp_d, (gGattCharPropRead_c) )
        VALUE(value_ppcp, gBleSig_GapPpcp_d, (gPermissionFlagReadable_c), 8, 0x0A, 0x00, 0x10, 0x00, 0x64, 0x00, 0xE2, 0x04)

PRIMARY_SERVICE(service_battery, gBleSig_BatteryService_d)
    CHARACTERISTIC(char_battery_level, gBleSig_BatteryLevel_d, (gGattCharPropNotify_c | gGattCharPropRead_c))
        VALUE(value_battery_level, gBleSig_BatteryLevel_d, (gPermissionFlagReadable_c), 1, 0x5A)
        DESCRIPTOR(desc_bat_level, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x04, 0x00, 0xAD, 0x27, 0x01, 0x01, 0x00)
        CCCD(cccd_battery_level)

PRIMARY_SERVICE(service_automation_io, gBleSig_AutomationIo_d)
    /* Digital IO Characteristic with PSOC63 128-bit UUID */
    CHARACTERISTIC_UUID128(char_digital_io_value, uuid_char_automation_io_digital_io, (gGattCharPropRead_c | gGattCharPropWrite_c | gGattCharPropNotify_c))
        VALUE_UUID128(value_digital_io_value, uuid_char_automation_io_digital_io, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), 4, 0x00, 0x00, 0x00, 0x00)
        DESCRIPTOR(desc_digital_io_number, 0x2909, (gPermissionFlagReadable_c), 7, 0x04, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00)
        DESCRIPTOR(desc_digital_io_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 15, "Switch Control")
        CCCD(cccd_digital_io_value)

    /* Analog Full Battery Characteristic with PSOC63 128-bit UUID */  
    CHARACTERISTIC_UUID128(char_analog_full_battery, uuid_char_automation_io_analog_full_vbat, (gGattCharPropRead_c))
        VALUE_UUID128(value_analog_full_battery, uuid_char_automation_io_analog_full_vbat, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_full_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x0E, 0x00, 0x28, 0x27, 0x01, 0x01, 0x00)
        DESCRIPTOR(desc_analog_full_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 13, "Full Battery")

    /* Additional Analog Bank Characteristics with PSOC63 128-bit UUIDs */
    CHARACTERISTIC_UUID128(char_analog_bank1, uuid_char_automation_io_analog_vbank1, (gGattCharPropRead_c))
        VALUE_UUID128(value_analog_bank1, uuid_char_automation_io_analog_vbank1, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank1_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x0E, 0x00, 0x28, 0x27, 0x01, 0x02, 0x00)
        DESCRIPTOR(desc_analog_bank1_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 12, "Bank 1 Volt")

    CHARACTERISTIC_UUID128(char_analog_bank2, uuid_char_automation_io_analog_vbank2, (gGattCharPropRead_c))
        VALUE_UUID128(value_analog_bank2, uuid_char_automation_io_analog_vbank2, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank2_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x0E, 0x00, 0x28, 0x27, 0x01, 0x03, 0x00)
        DESCRIPTOR(desc_analog_bank2_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 12, "Bank 2 Volt")

    CHARACTERISTIC_UUID128(char_analog_bank3, uuid_char_automation_io_analog_vbank3, (gGattCharPropRead_c))
        VALUE_UUID128(value_analog_bank3, uuid_char_automation_io_analog_vbank3, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank3_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x0E, 0x00, 0x28, 0x27, 0x01, 0x04, 0x00)
        DESCRIPTOR(desc_analog_bank3_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 12, "Bank 3 Volt")

    CHARACTERISTIC_UUID128(char_analog_bank4, uuid_char_automation_io_analog_vbank4, (gGattCharPropRead_c))
        VALUE_UUID128(value_analog_bank4, uuid_char_automation_io_analog_vbank4, (gPermissionFlagReadable_c), 2, 0x00, 0x00)
        DESCRIPTOR(desc_analog_bank4_format, gBleSig_CharPresFormatDescriptor_d, (gPermissionFlagReadable_c), 7, 0x0E, 0x00, 0x28, 0x27, 0x01, 0x05, 0x00)
        DESCRIPTOR(desc_analog_bank4_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 12, "Bank 4 Volt")

    /* PCBA test and trim 128-bit UUID */
    CHARACTERISTIC_UUID128(char_pcba_test_trim_value, uuid_char_pcba_test_trim, (gGattCharPropRead_c | gGattCharPropWrite_c | gGattCharPropNotify_c))
        VALUE_UUID128(value_pcba_test_trim, uuid_char_pcba_test_trim, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), 4, 0x00, 0x00, 0x00, 0x00)
        DESCRIPTOR(desc_pcba_trim_number, 0x2909, (gPermissionFlagReadable_c), 7, 0x04, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00)
        DESCRIPTOR(desc_pcba_trim_user, gBleSig_CharUserDescriptor_d, (gPermissionFlagReadable_c), 10, "PCBA trim")
        CCCD(cccd_pcba_trim_value)

PRIMARY_SERVICE(service_device_information, gBleSig_DeviceInformationService_d)
    CHARACTERISTIC(char_manuf_name, gBleSig_ManufacturerNameString_d, (gGattCharPropRead_c) )
        VALUE(value_manuf_name, gBleSig_ManufacturerNameString_d, (gPermissionFlagReadable_c), sizeof(MANUFACTURER_NAME) - 1, MANUFACTURER_NAME)
    CHARACTERISTIC(char_model_nb, gBleSig_ModelNumberString_d, (gGattCharPropRead_c) )
        VALUE(value_model_nb, gBleSig_ModelNumberString_d, (gPermissionFlagReadable_c), sizeof(BOARD_NAME) - 1, BOARD_NAME)
