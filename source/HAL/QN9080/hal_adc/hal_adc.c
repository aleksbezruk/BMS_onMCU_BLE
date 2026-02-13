/**
 * @file  hal_adc.c
 *
 * @brief Implementation of ADC functions.
 *
 * @note QN9080
 * 
 * @version 0.6.0
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
#define ADC_REFERENCE_VCC 1U // Define if VCC is used as reference voltage
#define ADC_REFERENCE_BANDGAP 2U // Define if Bandgap is used as reference voltage

/**
 * @note VCC is the default reference voltage source for ADC.
 *       It can be changed to other sources like bandgap or external reference.
 *       VCC = 3.0V measured on QN9080 DK board.
 */
#define ADC_REFERENCE_SOURCE  ADC_REFERENCE_BANDGAP
// #define ADC_REFERENCE_SOURCE  ADC_REFERENCE_VCC  // uncomment to use VCC as reference

/**
 * @note ADC gain setting.
 *       kADC_Gain1 means no gain, which is suitable for most applications (if Vref is VBAT).
 *       In case of Bandgap reference the Gain should be 0.5
 */
// #define ADC_GAIN kADC_Gain1 // uncomment to use the ADC gain setting, 1 means no gain (for Vref = VBAT, which is typically 3.0V on QN9080 DK board)
#define ADC_GAIN kADC_Gain0P5 // 0.5 gain enabled (required for bandgap reference because the bandgap voltage is typically half of VCC, so the gain compensates for the lower reference voltage)

/**
 * @note Full battery voltage in mV.
 *       3000u is a typical default value for QN9080 DK board.
 *       Adjust this value as needed for your hardware or make it configurable.
 */
#ifndef ADC_BATTERY_FULL_VOLTAGE
#define ADC_BATTERY_FULL_VOLTAGE 3000u // Default: 3000 mV
#endif  // ADC_BATTERY_FULL_VOLTAGE

#define ADC_BMS_BANK1_CHANNEL 4u
#define ADC_BMS_BANK2_CHANNEL 5u
#define ADC_BMS_BANK3_CHANNEL 8u
#define ADC_BMS_BANK4_CHANNEL 9u

#define ADC_CFG_IDX 0u // Index of the ADC configuration register to use

#define HAL_ADC_NUM_CHNLS   4U

/* ===================
 * Functions prototype
 * =================== */
/**
 * @brief Get the ADC channel number from HAL_ADC_channel_t enum.
 *
 * @param channel The HAL_ADC_channel_t enum value.
 * @return The corresponding ADC channel number.
 */
static uint8_t _get_adc_channel(HAL_ADC_channel_t channel);

/* ===================
 * Private data
 * =================== */
static uint32_t adc_vinn;
#if defined(ADC_REFERENCE_SOURCE) && (ADC_REFERENCE_SOURCE == ADC_REFERENCE_BANDGAP)
static float adcBandgap;
#endif // ADC_REFERENCE_SOURCE

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

    /** Configure ADC */
    ADC_GetDefaultConfig(&adcConfigStruct);
#if defined(ADC_REFERENCE_SOURCE) && (ADC_REFERENCE_SOURCE == ADC_REFERENCE_VCC)
    adcConfigStruct.refSource = kADC_RefSourceVccWithDriver; // Set reference source to VCC
#elif defined(ADC_REFERENCE_SOURCE) && (ADC_REFERENCE_SOURCE == ADC_REFERENCE_BANDGAP)
    adcConfigStruct.refSource = kADC_RefSourceBandgapWithDriver; // Set reference source to bandgap
#else
    #error "ADC_REFERENCE_SOURCE must be defined as either ADC_REFERENCE_VCC or ADC_REFERENCE_BANDGAP"
#endif // ADC_REFERENCE_SOURCE 

    adcConfigStruct.channelEnable = 0; // Default does not enable any channel
    adcConfigStruct.channelConfig = 0; // Default channel configuration
    adcConfigStruct.triggerSource = kADC_TriggerSelectSoftware;
    adcConfigStruct.convMode = kADC_ConvModeSingle;
    adcConfigStruct.clock = kADC_Clock2M; // Set ADC clock to 2MHz
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
    adcSdConfigStruct.gain = ADC_GAIN; // Set ADC gain as defined by ADC_GAIN
    ADC_SetSdConfig(ADC, ADC_CFG_IDX, (const adc_sd_config_t *)&adcSdConfigStruct);

#if defined(ADC_REFERENCE_SOURCE) && (ADC_REFERENCE_SOURCE == ADC_REFERENCE_BANDGAP)
    /** Bandgap calibration */
    adcBandgap = ADC_GetBandgapCalibrationResult(ADC, ADC_CFG_IDX);
#endif // ADC_REFERENCE_SOURCE

    /** Calibration VINN value */
    adc_vinn = ADC_GetVinnCalibrationResult(ADC, &adcConfigStruct);

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
    HAL_ASSERT(((uint8_t)channel < HAL_ADC_NUM_CHNLS), __FILE__, __LINE__);

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

    /** Get the conversion result */
    adcConvResult = ADC_GetConversionResult(ADC);

    /** Convert to mV */
    uint32_t ref_voltage_mv = 0;
    float gain_adjust = 1.0f;

#if defined(ADC_REFERENCE_SOURCE) && (ADC_REFERENCE_SOURCE == ADC_REFERENCE_VCC)
    ref_voltage_mv = ADC_BATTERY_FULL_VOLTAGE;
#elif defined(ADC_REFERENCE_SOURCE) && (ADC_REFERENCE_SOURCE == ADC_REFERENCE_BANDGAP)
    ref_voltage_mv = (uint32_t)adcBandgap;
    gain_adjust = 2.0f; // Adjust for gain of 0.5
#else
    #error "ADC_REFERENCE_SOURCE must be defined as either ADC_REFERENCE_VCC or ADC_REFERENCE_BANDGAP"
#endif // ADC_REFERENCE_SOURCE

    int32_t adc_value = 0;  // mV
    adc_value = ADC_ConversionResult2Mv(
        ADC,
        ch,
        ADC_CFG_IDX,
        ref_voltage_mv,
        adc_vinn,
        adcConvResult
    );
    adc_value = (int32_t)(adc_value * gain_adjust);

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
 * @return The corresponding ADC channel number. Returns 0xFF if the channel is invalid.
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
            HAL_ASSERT(0, __FILE__, __LINE__); // Invalid channel
            return 0xFF; // Invalid channel
        }
    }
}

/** [] END OF FILE */