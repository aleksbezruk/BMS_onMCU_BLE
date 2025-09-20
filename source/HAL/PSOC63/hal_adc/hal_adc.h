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
 * @version 0.6.0
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

/*! ADC channel */
typedef enum {
    HAL_ADC_CHANNEL_0,
    HAL_ADC_CHANNEL_1,
    HAL_ADC_CHANNEL_2,
    HAL_ADC_CHANNEL_3
} HAL_ADC_channel_t;

// =======================
// API
// =======================
HAL_ADC_status_t HAL_ADC_init(void);
int32_t HAL_ADC_read(HAL_ADC_channel_t channel);
/**
 * @brief Deinitializes the ADC peripheral and releases any resources.
 *
 * This function should be called to properly shut down the ADC hardware
 * and disable its clock when ADC operations are no longer needed.
 */
void HAL_ADC_deinit(void);

#endif // HAL_ADC_H_

/* [] END OF FILE */
