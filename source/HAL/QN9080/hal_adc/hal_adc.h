/** 
 * @file  hal_adc.h
 *
 * @brief Definition of adc functions:
 *          1. Init ADC peripheral:
 *              - enable clock;
 *              - config channels.
 *          2. Read ADC data for specified channel
 *
 * @note QN9080
 * 
 * @version 0.5.0
 */

#ifndef HAL_ADC_H_
#define HAL_ADC_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup HAL_ADC_Defines ADC Defines
 * @{
 * @brief Definitions and types for the ADC HAL
 */

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

/**
 * @defgroup HAL_ADC_API ADC API
 * @brief ADC HAL API functions
 * @{
 */
/**
 * @brief Deinitializes the ADC peripheral and releases resources.
 */
void HAL_ADC_deinit(void);
/**
 * @brief Initializes the ADC peripheral.
 * 
 * This function initializes the ADC hardware, enabling the clock and configuring the channels.
 * 
 * @return HAL_ADC_status_t - status of the initialization.
 */
HAL_ADC_status_t HAL_ADC_init(void);
/**
 * @brief Reads the ADC value from the specified channel.
 * 
 * This function reads the ADC value (in mV) from the specified channel using the NXP QN9080 HAL.
 * 
 * @param channel The ADC channel to read.
 * @return The ADC value in millivolts (mV).
 */
int32_t HAL_ADC_read(HAL_ADC_channel_t channel);

/** @} */
/** @} */

/** @} */

#endif  // HAL_ADC_H_

/* [] END OF FILE */
