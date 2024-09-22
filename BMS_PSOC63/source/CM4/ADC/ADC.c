/******************************************************************************
* @file  ADC.c
*
* @brief Analog to digital converter module functions definition.
* @details 
*           1. Hardware details:
*               1.1 BAT cell4 -> P10_0 sarmux[0] .
*               1.2 BAT cell3 -> P10_1 sarmux[1] .
*               1.3 BAT cell2 -> P10_2 sarmux[2] .
*               1.4 BAT cell1 -> P10_3 sarmux[3] .
*               1.5 temperature sensor connected to ADC.
*               1.6 One 12-bit SAR (successive approximation register) ADC
*               1.7 Reference voltage -> VDDA = 2.5V referenced form VDD
*           2. Software details:
*               2.1 Use Cypress HAL library: 
*                   - mtb-hal-cat1/release-v2.6.1/source/cyhal_adc_sar.c ;
*                   - mtb-hal-cat1/release-v2.6.1/include/cyhal_adc.h ;
*                   - mtb-hal-cat1/release-v2.6.1/include_pvt/cyhal_analog_common.h .
*               2.2 Cypress ModToolBox is used to define ADC peripheral (pins, clocks, etc) .
* @version 0.1.0
*/

#include "cyhal_adc.h"
#include "cycfg.h"
#include "ADC.h"
#include "qspyHelper.h"

// RTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"

/*******************************/
/*** Defines */
/******************************/
#define ADC_NUM_CHNLS   4U
#define ADC_CONVERT_UV_TO_MV(uv)    (uv / (int32_t) 1000)
/** 
 * ADC error in [%]. 
 * 1. Maybe variable from sample to sample
 * 2. Maybe needed external precise VREF to improve accuracy 
 */
#define ADC_ERR 4.0f
#define ADC_MEAS_COMPENSATE(val)    (int32_t)((float) val * (1.0f - ADC_ERR/100.0f))  // val in [mV] 

#define ADC_TASK_STACK_SIZE 400U   // bytes, aligned to 8 bytes

/*******************************/
/*** Private data */
/******************************/
static cyhal_adc_t adc_;    // allocate some memory for HAL ADC
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
 *  8 bytes per the RTOS requirements.
 */
static uint64_t adcTaskStack_[ADC_TASK_STACK_SIZE/8U];

/*******************************/
/*** Functions prototype */
/******************************/
static void adcTask_(cy_thread_arg_t arg);

/*******************************/
/*** Code */
/******************************/
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

static void adcTask_(cy_thread_arg_t arg)
{
    (void) arg;
    int32_t uv, mv;

    while(1) {
        (void)cy_rtos_delay_milliseconds(1000U);    // the API always returns SUCCESS becaause of hardcode

        /** Measure Bat Cell4 */
        uv = cyhal_adc_read_uv(&channel3_);
        mv = ADC_CONVERT_UV_TO_MV(uv);
        mv = ADC_MEAS_COMPENSATE(mv);

        QS_BEGIN_ID(ADC, 0 /*prio/ID for local Filters*/)
            QS_STR("Bat_Cell4 = ");
            QS_I32(0, mv); 
        QS_END()

        /** Measure Bat Cell3 */
        uv = cyhal_adc_read_uv(&channel2_);
        mv = ADC_CONVERT_UV_TO_MV(uv);
        mv = ADC_MEAS_COMPENSATE(mv);

        QS_BEGIN_ID(ADC, 0 /*prio/ID for local Filters*/)
            QS_STR("Bat_Cell3 = ");
            QS_I32(0, mv); 
        QS_END()

        /** Measure Bat Cell2 */
        uv = cyhal_adc_read_uv(&channel1_);
        mv = ADC_CONVERT_UV_TO_MV(uv);
        mv = ADC_MEAS_COMPENSATE(mv);

        QS_BEGIN_ID(ADC, 0 /*prio/ID for local Filters*/)
            QS_STR("Bat_Cell2 = ");
            QS_I32(0, mv); 
        QS_END()

        /** Measure Bat Cell1 */
        uv = cyhal_adc_read_uv(&channel0_);
        mv = ADC_CONVERT_UV_TO_MV(uv);
        mv = ADC_MEAS_COMPENSATE(mv);

        QS_BEGIN_ID(ADC, 0 /*prio/ID for local Filters*/)
            QS_STR("Bat_Cell1 = ");
            QS_I32(0, mv); 
        QS_END()
    }
}

/* [] END OF FILE */