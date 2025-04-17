/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_rng.h"
#include "fsl_adc.h"

/*******************************************************************************
 * Definitions
 *****************************************************************************/
#define RNG_ADC_CHANNEL (15U)
#define RNG_ADC_CFG_IDX (0U)
#define RNG_ADC_TRIGGER (kADC_TriggerSelectRNG)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

#ifdef _APP_PREINCLUDE_H_
void RNG_Drv_Init(RNG_Type *base)
#else
void RNG_Init(RNG_Type *base)
#endif
{
    adc_config_t adcConfigStruct;
    adc_sd_config_t adcSdConfigStruct;

    /**
     * Initial ADC
     */
    ADC_GetDefaultConfig(&adcConfigStruct);
    adcConfigStruct.channelEnable = (1U << RNG_ADC_CHANNEL);
    adcConfigStruct.channelConfig = (RNG_ADC_CFG_IDX << RNG_ADC_CHANNEL);
    adcConfigStruct.triggerSource = RNG_ADC_TRIGGER;
    adcConfigStruct.convMode = kADC_ConvModeBurst;
    adcConfigStruct.clock = kADC_Clock2M;
    ADC_Init(ADC, &adcConfigStruct);

    /* Initial ADC Sigma Delta(SD) configuration */
    ADC_GetSdDefaultConfig(&adcSdConfigStruct);
    ADC_SetSdConfig(ADC, RNG_ADC_CFG_IDX, &adcSdConfigStruct);

    /* Enable ADC */
    ADC_Enable(ADC, true);

    /* Fix data width to 8bits */
    base->CTRL = (base->CTRL & ~RNG_CTRL_NUM_MASK) | RNG_CTRL_NUM(0);
}

void RNG_Deinit(RNG_Type *base)
{
    ADC_Deinit(ADC);
}

status_t RNG_GetRandomData(RNG_Type *base, void *data, size_t dataSize)
{
    uint8_t *data_p = (uint8_t *)data;
    status_t result = kStatus_Success;

    if (base && data && dataSize)
    {
        while (dataSize--)
        {
            /* RNG start */
            RNG_Start(base);
            /* Wait for RNG not busy */
            while (RNG_GetStatusFlags(base) & kRNG_BusyFlag)
            {
            }
            /* Get random number */
            *data_p++ = (uint8_t)RNG_GetRandomNumber(base);
        }
    }
    else
    {
        result = kStatus_InvalidArgument;
    }

    return result;
}
