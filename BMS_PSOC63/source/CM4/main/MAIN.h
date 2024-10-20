/**
 * @file  MAIN.h
 *
 * @brief Main task data & API definition.
 *
 * @version 0.1.0
 */

#ifndef MAIN_MODULE_H
#define MAIN_MODULE_H

#include "bms_events.h"

////////////////////////////
/// API
////////////////////////////
void MAIN_post_evt(Evt_adc_data_t* evt);

#endif // MAIN_MODULE_H

/* [] END OF FILE */
