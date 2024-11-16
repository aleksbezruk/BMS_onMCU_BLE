/**
 * @file  ADC.c
 *
 * @brief Analog to digital converter module functions definition
 * 
 * @details ## **Details** 
 *           ### 1. Hardware details: <br>
 *               1.1 BAT cell4 -> P10_0 sarmux[0] <br>
 *               1.2 BAT cell3 -> P10_1 sarmux[1] <br>
 *               1.3 BAT cell2 -> P10_2 sarmux[2] <br>
 *               1.4 BAT cell1 -> P10_3 sarmux[3] <br>
 *               1.5 temperature sensor connected to ADC <br>
 *               1.6 One 12-bit SAR (successive approximation register) ADC <br>
 *               1.7 Reference voltage -> VDDA = 2.5V referenced form VDD <br>
 *           ### 2. Software details:
 *               2.1 Use Cypress HAL library: <br> 
 *                   - mtb-hal-cat1/release-v2.6.1/source/cyhal_adc_sar.c ;
 *                   - mtb-hal-cat1/release-v2.6.1/include/cyhal_adc.h ;
 *                   - mtb-hal-cat1/release-v2.6.1/include_pvt/cyhal_analog_common.h . <br>
 *               2.2 Cypress ModToolBox is used to config ADC peripheral (pins, clocks, etc) .
 * 
 * @version 0.1.0
 */

#include "cyhal_adc.h"
#include "cycfg.h"
#include "ADC.h"
#include "qspyHelper.h"
#include "MAIN.h"
#include "bms_events.h"

// RTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"

///////////////////////
// Defines
///////////////////////
#define ADC_NUM_CHNLS   4U
#define ADC_CONVERT_UV_TO_MV(uv)    (int16_t) (uv / 1000)
/** 
 * ADC error in [%]. 
 * 1. Maybe variable from sample to sample
 * 2. Maybe needed external precise VREF to improve accuracy 
 */
#define ADC_ERR 0
#define ADC_MEAS_COMPENSATE_ERR(val)    (int16_t)((float) val * (1.0f - ADC_ERR/100.0f))  // val in [mV] 
/**
 * Bank1
 * R1 = 1.2 MOhm, R2 = 1.2 MOhm
 */
#define ADC_BANK1_CONV_RATIO    2.0f
/**
 * Bank2
 * R1 = 1.2 MOhm, R2 = 390 kOhm
 */
#define ADC_BANK2_CONV_RATIO    4.07692308f
/**
 * Bank3
 * R1 = 2.4 MOhm, R2 = 390 kOhm
 */
#define ADC_BANK3_CONV_RATIO    7.153843402f
/**
 * Bank4
 * R1 = 2.4 MOhm, R2 = 390 kOhm
 */
#define ADC_BANK4_CONV_RATIO    7.153843402f
/**
 * mV
 */
#define ADC_BANK_VOLT_CALC(v_neg, v_pos)    (int16_t) ( v_pos - v_neg )
#define ADC_CONV_BY_RATIO(val, ratio)   (int16_t) ( (float) val * ratio )
#define ADC_NUM_MEAS 10U

#define ADC_TASK_INTERVAL   10000U
#define ADC_TASK_STACK_SIZE 560U   /**< bytes, aligned to 8 bytes */

///////////////////////
// Private data
///////////////////////
static cyhal_adc_t adc_;               // allocate some memory for HAL ADC
static cyhal_adc_channel_t channel0_;  // allocate memory for channel0 (BAT cell1)
static cyhal_adc_channel_t channel1_;  // allocate memory for channel0 (BAT cell2)
static cyhal_adc_channel_t channel2_;  // allocate memory for channel2 (BAT cell3)
static cyhal_adc_channel_t channel3_;  // allocate memory for channel3 (BAT cell4)
static cyhal_adc_channel_t* channels_[ADC_NUM_CHNLS] = {
    &channel0_,
    &channel1_,
    &channel2_,
    &channel3_
};
static uint8_t num_channels_ = ADC_NUM_CHNLS;  // allocate memory to keep count

static cy_thread_t adcTaskHandle_;
/** 
 *  In stack words because stack pointer should be aligned to 
 *  8 bytes boundaru per the RTOS requirements.
 */
static uint64_t adcTaskStack_[ADC_TASK_STACK_SIZE/8U];

///////////////////////
// Functions prototype
///////////////////////
static void adcTask_(cy_thread_arg_t arg);
static int16_t calcBankAvgVolt_(cyhal_adc_channel_t* chnl, float convRatio);

///////////////////////
// Code
///////////////////////
/**
 * @brief Init ADC peripheral and create RTOS task
 * 
 * @param None
 * 
 * @retval See \ref ADC_status_t
 * 
 */
ADC_status_t ADC_init(void)
{
    ADC_status_t status = ADC_STATUS_OK;
    cy_rslt_t result;

    /** Init ADC periph */
    result = cyhal_adc_init_cfg(
        &adc_, 
        channels_, 
        &num_channels_,
        &pass_0_sar_0_hal_config
    );

    /** Check operation status */
    if (result != CY_RSLT_SUCCESS) {
        status = ADC_STATUS_FAIL;
    }

    /** Create RTOS task */
    if (status == ADC_STATUS_OK) {
        result = cy_rtos_thread_create(
            &adcTaskHandle_, 
            adcTask_,
            "adcTask", 
            adcTaskStack_,          // should be aligned to 8 bytes
            ADC_TASK_STACK_SIZE,    // in bytes
            CY_RTOS_PRIORITY_LOW,   // prio
            NULL                    // no args
        );
        if (result != CY_RSLT_SUCCESS) {
            status = ADC_STATUS_FAIL;
        }
    }

    return status;
}

/**
 * @brief ADC task handler
 * 
 * @param[in] arg task input argument
 * 
 * @retval None (never returns because endless task)
 * 
 */
static void adcTask_(cy_thread_arg_t arg)
{
    (void) arg;
    Evt_adc_data_t adcEvt;
    int16_t temp_mv;

    while(1) {
        (void)cy_rtos_delay_milliseconds(ADC_TASK_INTERVAL);    // the API always returns SUCCESS becaause of hardcode

        /** Measure Bat Cell1 */
        temp_mv = calcBankAvgVolt_(
            &channel0_,
            ADC_BANK1_CONV_RATIO
        );
        adcEvt.bank1_mv = temp_mv;

        QS_BEGIN_ID(ADC, 0 /*prio/ID for local Filters*/)
            QS_STR("Bat_Cell1 = ");
            QS_I16(0, adcEvt.bank1_mv); 
        QS_END()

        /** Measure Bat Cell2 */
        temp_mv = calcBankAvgVolt_(
            &channel1_,
            ADC_BANK2_CONV_RATIO
        );
        adcEvt.bank2_mv = ADC_BANK_VOLT_CALC(adcEvt.bank1_mv, temp_mv);

        QS_BEGIN_ID(ADC, 0 /*prio/ID for local Filters*/)
            QS_STR("Bat_Cell2 = ");
            QS_I16(0, adcEvt.bank2_mv); 
        QS_END()

        /** Measure Bat Cell3 */
        temp_mv = calcBankAvgVolt_(
            &channel2_,
            ADC_BANK3_CONV_RATIO
        );
        adcEvt.bank3_mv = ADC_BANK_VOLT_CALC((adcEvt.bank1_mv + adcEvt.bank2_mv), temp_mv);

        QS_BEGIN_ID(ADC, 0 /*prio/ID for local Filters*/)
            QS_STR("Bat_Cell3 = ");
            QS_I16(0, adcEvt.bank3_mv); 
        QS_END()

        /** Measure Bat Cell4 */
        temp_mv = calcBankAvgVolt_(
            &channel3_,
            ADC_BANK4_CONV_RATIO
        );
        adcEvt.full_mv = temp_mv;
        adcEvt.bank4_mv = ADC_BANK_VOLT_CALC((adcEvt.bank1_mv + adcEvt.bank2_mv + adcEvt.bank3_mv), temp_mv);

        QS_BEGIN_ID(ADC, 0 /*prio/ID for local Filters*/)
            QS_STR("Bat_Cell4 = ");
            QS_I16(0, adcEvt.bank4_mv); 
        QS_END()

        MAIN_post_evt((Main_evt_t*) &adcEvt, EVT_ADC);
    }
}

/**
 * @brief Calculate average Bank voltage
 * 
 * @note Avearging using 10 samples.
 * 
 * @param[in] chnl - ADC chnl
 *
 * @param[in] convRatio - ADC conertion ratio
 * @retval Bank voltage in mV
 * 
 */
static int16_t calcBankAvgVolt_(cyhal_adc_channel_t* chnl, float convRatio)
{
    int32_t uv;
    int16_t mv;
    float adcMeasSum = 0;
    uint8_t i;
    int16_t retVal;

    /** ADC measurements */
    for(i = 0; i < ADC_NUM_MEAS; i++) {
        uv = cyhal_adc_read_uv(chnl);
        mv = ADC_CONVERT_UV_TO_MV(uv);
        mv = ADC_MEAS_COMPENSATE_ERR(mv);
        mv = ADC_CONV_BY_RATIO(mv, convRatio);
        adcMeasSum += mv;
    }

    /** Averaging & return */
    retVal = adcMeasSum / ADC_NUM_MEAS;
    return retVal;
}

/* [] END OF FILE */