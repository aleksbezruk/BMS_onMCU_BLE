/**
 * @file  ADC.h
 *
 * @brief Analog to digital converter module functions declaration.
 *
 * @version 0.1.0
 */

#ifndef ADC_MODULE_H
#define ADC_MODULE_H

///////////////////////
// Defines
///////////////////////
#define ADC_BMS_FULL_VBAT_MIN   (int16_t) 11200  // mV
#define ADC_BMS_FULL_VBAT_MAX   (int16_t) 16800  // mV

/** ADC status */
typedef enum {
    ADC_STATUS_OK,
    ADC_STATUS_FAIL
} ADC_status_t;

///////////////////////
// Code
///////////////////////
ADC_status_t ADC_init(void);

#endif  //ADC_MODULE_H

/* [] END OF FILE */
