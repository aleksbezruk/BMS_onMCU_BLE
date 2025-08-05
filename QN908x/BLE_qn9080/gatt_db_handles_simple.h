/**
 * @file gatt_db_handles_simple.h
 * @brief Simple GATT Database Handle Definitions
 * @note This file provides handle constants that correlate with gatt_db.h
 * @details For Test configuration, we define handle constants manually to match the GATT database structure
 * @version 0.1.0
 */

#ifndef GATT_DB_HANDLES_SIMPLE_H
#define GATT_DB_HANDLES_SIMPLE_H

#include <stdint.h>

/*! *********************************************************************************
 * GATT Database Handle Constants
 * These handles correspond to the attributes defined in gatt_db.h
 * 
 * GATT Database Structure:
 * - service_gap (GAP Service)
 *   - char_device_name
 *     - value_device_name         <- Handle ~0x0003
 *   - char_appearance  
 *     - value_appearance         <- Handle ~0x0005
 *   - char_ppcp
 *     - value_ppcp              <- Handle ~0x0007
 * 
 * - service_battery (Battery Service) 
 *   - char_battery_level
 *     - value_battery_level     <- Handle ~0x000A (this is what we need)
 *     - desc_bat_level         <- Handle ~0x000B
 *     - cccd_battery_level     <- Handle ~0x000C
 ********************************************************************************** */

// GAP Service handles
#define HANDLE_GAP_SERVICE              0x0001
#define HANDLE_DEVICE_NAME_CHAR         0x0002
#define HANDLE_DEVICE_NAME_VALUE        0x0003
#define HANDLE_APPEARANCE_CHAR          0x0004
#define HANDLE_APPEARANCE_VALUE         0x0005
#define HANDLE_PPCP_CHAR                0x0006
#define HANDLE_PPCP_VALUE               0x0007

// Battery Service handles  
#define HANDLE_BATTERY_SERVICE          0x0008
#define HANDLE_BATTERY_LEVEL_CHAR       0x0009
#define HANDLE_BATTERY_LEVEL_VALUE      0x000A  // This correlates with value_battery_level
#define HANDLE_BATTERY_LEVEL_DESC       0x000B  // This correlates with desc_bat_level
#define HANDLE_BATTERY_LEVEL_CCCD       0x000C  // This correlates with cccd_battery_level

/*! *********************************************************************************
 * Correlation with gatt_db.h:
 * 
 * value_battery_level  -> HANDLE_BATTERY_LEVEL_VALUE  (0x000A)
 * desc_bat_level      -> HANDLE_BATTERY_LEVEL_DESC   (0x000B)  
 * cccd_battery_level  -> HANDLE_BATTERY_LEVEL_CCCD   (0x000C)
 * 
 * These constants should be used instead of the auto-generated enum values
 * when the macro system is not properly generating the handles.
 ********************************************************************************** */

// Define the correlation constants for easier code readability
#define value_battery_level     HANDLE_BATTERY_LEVEL_VALUE
#define desc_bat_level         HANDLE_BATTERY_LEVEL_DESC  
#define cccd_battery_level     HANDLE_BATTERY_LEVEL_CCCD

#endif /* GATT_DB_HANDLES_SIMPLE_H */
