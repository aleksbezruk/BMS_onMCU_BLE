/**
 * @file  hal_adc.h
 *
 * @brief Definition of adc functions:
 *          1. Init ADC peripheral:
 *              - enable clock;
 *              - config channels.
 *          2. Read ADC data for specified channel
 *
 * @note PSOC63
 * 
 * @version 0.5.0
 */

#ifndef HAL_ADC_H_
#define HAL_ADC_H_

#include <stdint.h>
#include <stdbool.h>

// =======================
// Defines
// =======================

/*! ADC status */
typedef enum {
    HAL_ADC_SUCCESS,
    HAL_ADC_FAIL
} HAL_ADC_status_t;

// =======================
// API
// =======================
HAL_ADC_status_t HAL_ADC_init(void);
HAL_ADC_status_t HAL_ADC_read(uint32_t channel, uint16_t *data);

#endif // HAL_ADC_H_

/* [] END OF FILE */
