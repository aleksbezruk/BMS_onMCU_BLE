#ifndef GATT_UUID128_H
#define GATT_UUID128_H

#include "ble_general.h"

/* 
* Declare all custom 128-bit UUIDs here
* Definitions are in gatt_uuid128_defs.c to avoid multiple definition errors
*/

// Declare 128-bit UUIDs for PSOC63 compatibility
extern uint8_t uuid_char_automation_io_digital_io[16];
extern uint8_t uuid_char_automation_io_analog_full_vbat[16];
extern uint8_t uuid_char_automation_io_analog_vbank1[16];
extern uint8_t uuid_char_automation_io_analog_vbank2[16];
extern uint8_t uuid_char_automation_io_analog_vbank3[16];
extern uint8_t uuid_char_automation_io_analog_vbank4[16];

#endif // GATT_UUID128_H
