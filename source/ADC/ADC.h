/**
 *
 * @file  ADC.h
 *
 * @brief Analog to digital converter module functions declaration.
 *
 * @version 0.7.0
 */

#ifndef ADC_MODULE_H
#define ADC_MODULE_H

#include <stdbool.h>

// =======================
// Defines
// =======================
#define ADC_BMS_FULL_VBAT_MIN   (int16_t) 11200  // mV
#define ADC_BMS_FULL_VBAT_MAX   (int16_t) 16800  // mV
#define ASC_BMS_BANK_VOLTAGE_MIN (int16_t) 2800   // mV
#define ASC_BMS_BANK_VOLTAGE_MAX (int16_t) 4200   // mV
#define ADC_BMS_CALC_PERCENT(vbat) ( (100u * ((vbat) - ADC_BMS_FULL_VBAT_MIN)) / (ADC_BMS_FULL_VBAT_MAX - ADC_BMS_FULL_VBAT_MIN) )

/** ADC status */
typedef enum {
    ADC_STATUS_OK,
    ADC_STATUS_FAIL
} ADC_status_t;

// =======================
// Code
// =======================
ADC_status_t ADC_init(void);
void ADC_deinit(void);
void ADC_setState(bool on_off);

#endif  //ADC_MODULE_H

/* [] END OF FILE */
