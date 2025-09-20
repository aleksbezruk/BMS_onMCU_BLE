/**
 * @file  hal_adc.c
 *
 * @brief Implementation of ADC functions.
 *
 * @note PSOC63
 * 
 * @version 0.6.0
 */
#include "cy_pdl.h"
#include "cyhal.h"
#include "cyhal_adc.h"
#include "cycfg.h"

#include "hal.h"
#include "hal_adc.h"

// =======================
// Defines
// =======================
#define HAL_ADC_NUM_CHNLS   4U
#define HAL_ADC_CONVERT_UV_TO_MV(uv)    (int16_t)(((uv) + 500) / 1000)

// =======================
// Functions prototype
// =======================


// =======================
// Private data
// =======================
static cyhal_adc_t adc_;               // allocate some memory for HAL ADC
static cyhal_adc_channel_t channel0_;  // allocate memory for channel0 (BAT cell1)
static cyhal_adc_channel_t channel1_;  // allocate memory for channel1 (BAT cell2)
static cyhal_adc_channel_t channel2_;  // allocate memory for channel2 (BAT cell3)
static cyhal_adc_channel_t channel3_;  // allocate memory for channel3 (BAT cell4)
static cyhal_adc_channel_t* channels_[HAL_ADC_NUM_CHNLS] = {
    &channel0_,
    &channel1_,
    &channel2_,
    &channel3_
};
static uint8_t num_channels_ = HAL_ADC_NUM_CHNLS;  // allocate memory to keep count

// =======================
// Code
// =======================
/**
 * @brief Initialize ADC peripheral
 * @details This function initializes the ADC hardware using the Cypress HAL library.
 * @param None
 * @retval See \ref HAL_ADC_status_t
 */
HAL_ADC_status_t HAL_ADC_init(void)
{
    HAL_ADC_status_t status = HAL_ADC_SUCCESS;

    // Initialize ADC hardware
    if (cyhal_adc_init_cfg(&adc_, channels_, &num_channels_, &pass_0_sar_0_hal_config) != CY_RSLT_SUCCESS) {
        status = HAL_ADC_FAIL;
    }

    return status;
}

/**
 * @brief De-initialize ADC peripheral
 * @details This function de-initializes the ADC hardware and releases any resources used by it.
 * @param None
 * @retval None
 */
void HAL_ADC_deinit(void)
{
    // De-initialize ADC hardware
    cyhal_adc_free(&adc_);
}

/**
 * @brief Read ADC channel value
 * @details This function reads the ADC value (in mV) from the specified channel using the Cypress HAL library.
 * @param channel The ADC channel to read.
 * @retval The ADC value in millivolts (mV).
 */
int32_t HAL_ADC_read(HAL_ADC_channel_t channel)
{
    // Validate channel index
    HAL_ASSERT(((uint8_t)channel < HAL_ADC_NUM_CHNLS), __FILE__, __LINE__);

    int32_t uv = cyhal_adc_read_uv(channels_[channel]);
    if (uv < 0) {
        // Error occurred during ADC read, handle as needed (e.g., return error code or 0)
        HAL_ASSERT(false, __FILE__, __LINE__);
        return 0;  // Return 0 or an error code as appropriate
    }

    // Convert microvolts to millivolts
    return HAL_ADC_CONVERT_UV_TO_MV(uv);
}

/* [] END OF FILE */
