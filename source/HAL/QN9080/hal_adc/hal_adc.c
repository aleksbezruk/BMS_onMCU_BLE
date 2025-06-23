/**
 * @file  hal_adc.c
 *
 * @brief Implementation of ADC functions.
 *
 * @note QN9080
 * 
 * @version 0.5.0
 */
#include "fsl_device_registers.h"
#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_adc.h"
#include "fsl_power.h"

#include "hal.h"
#include "hal_adc.h"

/* ===================
 * Defines
 * =================== */
/**
 * @note VCC is the default reference voltage source for ADC.
 *       It can be changed to other sources like bandgap or external reference.
 *       VCC = 3.0V measured on QN9080 DK board.
 */
#define ADC_REFERENCE_SOURCE kADC_RefSourceVccWithDriver
#define ADC_BATTERY_FULL_VOLTAGE 3000u // Full battery voltage in mV

#define ADC_BMS_BANK1_CHANNEL 4u
#define ADC_BMS_BANK2_CHANNEL 5u
#define ADC_BMS_BANK3_CHANNEL 8u
#define ADC_BMS_BANK4_CHANNEL 9u

#define ADC_CFG_IDX 0u // Index of the ADC configuration register to use

#define HAL_ADC_NUM_CHNLS   4U

/* ===================
 * Functions prototype
 * =================== */
static uint8_t _get_adc_channel(HAL_ADC_channel_t channel);

/* ===================
 * Private data
 * =================== */
static uint32_t mAdcVinn;

/* ===================
 * Code
 * =================== */
/**
 * @brief Initialize ADC peripheral.
 *
 * This function initializes the ADC hardware, enabling the clock and configuring the channels.
 *
 * @param None
 * 
 * @note Use an example from  board.c file as a reference for the actual implementation.
 * 
 * @note AN12232 QN908x ADC Configuration Guide:
 * If PGA_GAIN =1 and ADC_GAIN=1, then the formula for single mode is:
 * (Vadcx – Vinn) / Vref = RegData / 2^22 => Vadcx = (RegData / 2^22) * Vref + Vinn
 * Vadcx: Voltage of analog input single from pin
 * Vinn: Voltage common input, it can be chosen from Vref, 1/2Vref, 3/4Vref and GND.
 * Vref: Reference voltage, it can be chosen from internal bandgap, VCC and external
 * reference on pin PA07. The internal bandgap voltage store on flash information
 * address(0x210B07F4).
 * RegData: Read from DATA register.
 *
 * @return HAL_ADC_status_t - status of the initialization.
 */
HAL_ADC_status_t HAL_ADC_init(void) 
{
    adc_config_t adcConfigStruct;

    /** Power on the ADC */
    POWER_EnableADC(true);

    /** Enable the ADC clock */
    CLOCK_EnableClock(kCLOCK_Adc);

    /** Initialize ADC configuration structure with default values & init hardware */
    ADC_GetDefaultConfig(&adcConfigStruct);
    adcConfigStruct.channelEnable = 0; // Default does not enable any channel
    adcConfigStruct.channelConfig = 0; // Default channel configuration
    adcConfigStruct.triggerSource = kADC_TriggerSelectSoftware;
    adcConfigStruct.convMode = kADC_ConvModeSingle;
    adcConfigStruct.clock = kADC_Clock2M; // Set ADC clock to 2MHz
    adcConfigStruct.refSource = ADC_REFERENCE_SOURCE; // Set reference source to VCC
    adcConfigStruct.dataFormat = kADC_DataFormat1WithIdx; // Set data format
    ADC_Init(ADC, &adcConfigStruct);

    /** Initialize ADC Sigma Delta (SD) */
    adc_sd_config_t adcSdConfigStruct;
    ADC_GetSdDefaultConfig(&adcSdConfigStruct);
    adcSdConfigStruct.pgaGain = kADC_PgaGain1; // Set PGA gain to 1
    adcSdConfigStruct.refGain = kADC_RefGain1; // Set reference gain to 1
    adcSdConfigStruct.vinnSelect = kADC_VinnSelectAvss; // Set VINN to AVSS/GND
    adcSdConfigStruct.downSample = kADC_DownSample256; // Set down sample rate to 256
    adcSdConfigStruct.adjustDirection = kADC_PgaAdjustMoveDown; // Set PGA adjust direction
    adcSdConfigStruct.adjustValue = 0; // No adjustment value
    adcSdConfigStruct.vcmSelect = kADC_VcmVoltage1D16; // Set VCM to 1.16V. The value doesn't matter for single-ended channels
    adcSdConfigStruct.gain = kADC_Gain1; // Set ADC gain to 1
    ADC_SetSdConfig(ADC, ADC_CFG_IDX, (const adc_sd_config_t *)&adcSdConfigStruct);

    /** Calibration VINN value */
    mAdcVinn = ADC_GetVinnCalibrationResult(ADC, &adcConfigStruct);

    /** Enable chopper to improve noise performance */
    ADC_PgaChopperEnable(ADC, true);

    /** Enable ADC */
    ADC_Enable(ADC, true);

    return HAL_ADC_SUCCESS;
}

/**
 * @brief Read ADC channel value
 * @details This function reads the ADC value (in mV) from the specified channel using the NXP QN9080 HAL.
 * @param channel The ADC channel to read.
 * @retval The ADC value in millivolts (mV).
 */
int32_t HAL_ADC_read(HAL_ADC_channel_t channel) 
{
    /** Check if the channel is valid */
    HAL_ASSERT((uint8_t)channel < HAL_ADC_NUM_CHNLS);
 
    int32_t adc_value = 0;  // mV
    uint32_t adcConvResult = 0;

    /** Get ADC channel */
    uint8_t ch = _get_adc_channel(channel);

    /** Enable ADC channel */
    ADC->CH_SEL = (1U << ch);

    /** Select channel configuration to be used */
    ADC->CH_CFG = ADC_CFG_IDX; // Use the selected configuration index

    /** Single conversion, software triggered */
    ADC->CTRL |= kADC_ConvModeSingle | ADC_CTRL_TRIGGER(kADC_TriggerSelectSoftware);

    /** Start ADC conversion */
    ADC_DoSoftwareTrigger(ADC);

    /** Wait for ADC conversion to complete */
    while (!(ADC_GetStatusFlags(ADC) & kADC_DataReadyFlag)) {}

    /** Get ADC conversion result */
    adcConvResult = ADC_GetConversionResult(ADC);

    /** Convert the result to mV */
    adc_value = ADC_ConversionResult2Mv(
        ADC,
        ch,
        ADC_CFG_IDX,
        ADC_BATTERY_FULL_VOLTAGE, // Reference voltage in mV
        mAdcVinn,   // VINN calibration value
        adcConvResult
    );

    /** Disable ADC channel */
    ADC->CH_SEL = 0; // Disable all channels

    return adc_value;
}

/**
 * @brief Deinitialize ADC peripheral.
 *
 * This function deinitializes the ADC hardware, disabling the clock.
 *
 * @param None
 * 
 * @return None
 */
void HAL_ADC_deinit(void) 
{
    /** Deinitialize ADC hardware */
    ADC_Deinit(ADC);

    /** Disable ADC clock */
    CLOCK_DisableClock(kCLOCK_Adc);

    /** Power off the ADC */
    POWER_EnableADC(false);
}

/* ============================
 * Private (Auxiliary) Functions
 * ============================ */
/**
 * @brief Get the ADC channel number from HAL_ADC_channel_t enum.
 *
 * @param channel The HAL_ADC_channel_t enum value.
 * @return The corresponding ADC channel number.
 */
static uint8_t _get_adc_channel(HAL_ADC_channel_t channel)
{
    switch (channel)
    {
        case HAL_ADC_CHANNEL_0: 
        {
            return ADC_BMS_BANK1_CHANNEL;
        }
        case HAL_ADC_CHANNEL_1: 
        {
            return ADC_BMS_BANK2_CHANNEL;
        }
        case HAL_ADC_CHANNEL_2: 
        {
            return ADC_BMS_BANK3_CHANNEL;
        }
        case HAL_ADC_CHANNEL_3: 
        {
            return ADC_BMS_BANK4_CHANNEL;
        }
        default: 
        {
            HAL_ASSERT(false); // Invalid channel
            return 0xFF; // Invalid channel
        }
    }
}

/** [] END OF FILE */