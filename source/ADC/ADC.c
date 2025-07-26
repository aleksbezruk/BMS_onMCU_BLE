/**
 * @file  ADC.c
 *
 * @brief Analog to digital converter module functions definition
 * 
 * @details ## **Details** 
 *           ### 1. Hardware details: <br>
 *               1.1 For PSOC63: BAT cell4 -> P10_0 sarmux[0] <br>
 *               1.2 For PSOC63: BAT cell3 -> P10_1 sarmux[1] <br>
 *               1.3 For PSOC63: BAT cell2 -> P10_2 sarmux[2] <br>
 *               1.4 For PSOC63: BAT cell1 -> P10_3 sarmux[3] <br>
 *               1.5 For PSOC63: temperature sensor connected to ADC <br>
 *               1.6 For PSOC63: One 12-bit SAR (successive approximation register) ADC <br>
 *               1.7 For PSOC63: Reference voltage -> VDDA = 2.5V referenced form VDD <br>
 *           ### 2. Software details for PSOC63:
 *               2.1 Use Cypress HAL library: <br> 
 *                   - mtb-hal-cat1/release-v2.6.1/source/cyhal_adc_sar.c ;
 *                   - mtb-hal-cat1/release-v2.6.1/include/cyhal_adc.h ;
 *                   - mtb-hal-cat1/release-v2.6.1/include_pvt/cyhal_analog_common.h . <br>
 *               2.2 Cypress ModToolBox is used to config ADC peripheral (pins, clocks, etc) .
 *
 * @version 0.5.0
 */

#include "ADC.h"
#include "qspyHelper.h"
#include "MAIN.h"
#include "bms_events.h"

// RTOS includes
#include "OSAL.h"

// HAL
#include "hal.h"
#include "hal_adc.h"

// =======================
// Defines
// =======================
/** 
 * ADC error in [%]. 
 * 1. Maybe variable from sample to sample
 * 2. Maybe needed external precise VREF to improve accuracy 
 */
#define ADC_ERR 2.0f
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

/**
 * Convert ADC value to mV using conversion ratio
 * @note val is in [mV]
 */
#define ADC_CONV_BY_RATIO(val, ratio)   (int16_t) ( (float) val * ratio )

/**
 * Number of ADC measurements to average
 * @note Averaging using 5 samples.
 */
#define ADC_NUM_MEAS 5U

// RTOS task defines
#define ADC_TASK_INTERVAL   30000U  /**< ms */
#define ADC_TASK_STACK_SIZE 1024U   /**< bytes, aligned to 8 bytes */

// =======================
// Private data
// =======================
/** ADC task */
OSAL_TASK_DEFINE(adcTask);

#if !defined(BMS_DISABLE_OSAL_STATIC_ALL)
/** 
 *  @note In stack words because stack pointer should be aligned to 
 *  8 bytes boundary per the RTOS requirements.
 */
static uint64_t adcTaskStack_[ADC_TASK_STACK_SIZE/8U];
#else
/*! For some ports (e.g. QN908x) the OSAL static allocation is not supported */
#define adcTaskStack_ NULL
#endif // !BMS_DISABLE_OSAL_STATIC_ALL

/*! ADC peripheral status flag */
static volatile bool _adcEnabled = true;

// =======================
// Functions prototype
// =======================
static void adcTask_(OSAL_arg_t arg);
static int16_t calcBankAvgVolt_(HAL_ADC_channel_t chnl, float convRatio, int16_t* pAdcInVolt);
static ADC_status_t ADC_init_periph(void);

// =======================
// Code
// =======================
/**
 * @brief Init ADC peripheral
 * 
 * @param None
 * 
 * @retval See \ref ADC_status_t
 */
ADC_status_t ADC_init_periph(void)
{
    ADC_status_t status = ADC_STATUS_OK;

    if (HAL_ADC_init() != HAL_ADC_SUCCESS) {
        status = ADC_STATUS_FAIL;
    }

    return status;
}

/**
 * @brief De-init ADC peripheral
 * 
 * @param None
 * 
 * @retval None
 * 
 */
void ADC_deinit(void)
{
    HAL_ADC_deinit();
}

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

    /** Init ADC periph */
    status = ADC_init_periph();

    /** Create RTOS task */
    if (status == ADC_STATUS_OK) {
        OSAL_Status_t osal_status = OSAL_SUCCESS;
        OSAL_TASK_CREATE(
            OSAL_TASK_GET_HANDLE(adcTask),
            adcTask_,
            "adcTask",
            adcTaskStack_,          // should be aligned to 8 bytes
            ADC_TASK_STACK_SIZE,    // in bytes
            OSAL_ADC_TASK_PRIORITY, // prio
            NULL,                   // no args
            osal_status
        );
        if (osal_status != OSAL_SUCCESS) {
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
static void adcTask_(OSAL_arg_t arg)
{
    (void) arg;
    Evt_adc_data_t adcEvt;
    int16_t b1_v, b2_v, b3_v, b4_v, bankAdcIn;
    ADC_status_t status;

    while(1) {
        OSAL_TASK_DELAY(ADC_TASK_INTERVAL);

        /** Enable ADC if disabled (during Deep Sleep) */
        if (!_adcEnabled) {
            status = ADC_init_periph();
            _adcEnabled = true;
            if (status != ADC_STATUS_OK) {
                HAL_ASSERT(0, __FILE__, __LINE__);
            }
        }

        /** Measure Bat Cell1 */
        b1_v = calcBankAvgVolt_(
            HAL_ADC_CHANNEL_0,
            ADC_BANK1_CONV_RATIO,
            &bankAdcIn
        );
        adcEvt.bank1_mv = b1_v;

        QS_BEGIN_ID(ADC_RCD, 0 /*prio/ID for local Filters*/)
            QS_STR("Cell1_raw = ");
            QS_I16(0, bankAdcIn); 
        QS_END()

        /** Measure Bat Cell2 */
        b2_v = calcBankAvgVolt_(
            HAL_ADC_CHANNEL_1,
            ADC_BANK2_CONV_RATIO,
            &bankAdcIn
        );
        adcEvt.bank2_mv = ADC_BANK_VOLT_CALC(b1_v, b2_v);

        QS_BEGIN_ID(ADC_RCD, 0 /*prio/ID for local Filters*/)
            QS_STR("Cell2_raw = ");
            QS_I16(0, bankAdcIn); 
        QS_END()

        /** Measure Bat Cell3 */
        b3_v = calcBankAvgVolt_(
            HAL_ADC_CHANNEL_2,
            ADC_BANK3_CONV_RATIO,
            &bankAdcIn
        );
        adcEvt.bank3_mv = ADC_BANK_VOLT_CALC(b2_v, b3_v);

        QS_BEGIN_ID(ADC_RCD, 0 /*prio/ID for local Filters*/)
            QS_STR("Cell3_raw = ");
            QS_I16(0, bankAdcIn); 
        QS_END()

        /** Measure Bat Cell4 */
        b4_v = calcBankAvgVolt_(
            HAL_ADC_CHANNEL_3,
            ADC_BANK4_CONV_RATIO,
            &bankAdcIn
        );
        adcEvt.full_mv = b4_v;
        adcEvt.bank4_mv = ADC_BANK_VOLT_CALC(b3_v, b4_v);

        QS_BEGIN_ID(ADC_RCD, 0 /*prio/ID for local Filters*/)
            QS_STR("Cell4_raw = ");
            QS_I16(0, bankAdcIn); 
        QS_END()

        MAIN_post_evt((Main_evt_t*) &adcEvt, EVT_ADC);
    }
}

/**
 * @brief Calculate average Bank voltage
 * 
 * @note Averaging using 5 samples.
 * 
 * @param[in] chnl - ADC chnl
 *
 * @param[in] convRatio - ADC conversion ratio
 * 
 * @param[out] pAdcInVolt - ADC measured input/raw voltage
 * 
 * @retval Bank voltage in mV
 *
 */
static int16_t calcBankAvgVolt_(HAL_ADC_channel_t chnl, float convRatio, int16_t* pAdcInVolt)
{
    int16_t mv;
    float avgAdcIn = 0;
    uint8_t i;
    int16_t retVal;
    float adcMeasSum = 0;

    /** ADC measurements */
    for(i = 0; i < ADC_NUM_MEAS; i++) {
        mv = HAL_ADC_read(chnl);
        avgAdcIn += mv;
        mv = ADC_MEAS_COMPENSATE_ERR(mv);
        mv = ADC_CONV_BY_RATIO(mv, convRatio);
        adcMeasSum += mv;
    }

    /** Averaging & return */
    *pAdcInVolt = avgAdcIn / ADC_NUM_MEAS;
    retVal = adcMeasSum / ADC_NUM_MEAS;
    return retVal;
}

/**
 * @brief Set ADC state
 * 
 * @param[in] on_off - true to enable ADC, false to disable
 * 
 * @retval None
 */
void ADC_setState(bool on_off)
{
    _adcEnabled = on_off;
}

/* [] END OF FILE */