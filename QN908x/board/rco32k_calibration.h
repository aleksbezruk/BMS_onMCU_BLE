/*! *********************************************************************************
* Copyright 2018, 2022 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#ifndef _RCO32K_CALIBRATION_H_
#define _RCO32K_CALIBRATION_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define RCO32K_1ST_EDGE (1U)
#define RCO32K_17TH_EDGE (17U)

/* RCO calibration interval in miliseconds
 * Can be changed as needed.
 * Smaller interval = slightly higher power consumption but will offer RCO stability and lower ppm when temperature varies
 * Larger  interval = lower power consumption but RCO will not be as stable in varying temperature (can be used safely if temperature is stable)
 */
#ifndef RCO32K_CALIBRATION_INTERVAL
#define RCO32K_CALIBRATION_INTERVAL (1000U)
#endif

/* RCO startup calibration interval in miliseconds.
 * Performed 2 times. Can be changed as needed.
 */
#define RCO32K_STARTUP_CALIBRATION_INTERVAL (50U)

/* Perform the calibration on Idle Task.
 * If this macro is set to 0, the calibration will be performed on calibration timer callback.
 */
#define CFG_CALIBRATION_ON_IDLE_TASK    (1U)

/*!
 * @brief RCO32K calibration environment
 *
 * This structure holds the environment for calibrating RCO32K.
 */
typedef struct _rco32k_calib_env
{
    /* application defined */
    CTIMER_Type *ctimer;    /*!< stores which ctimer is used for calibrating rco32k */
    uint32_t    ctimer_irq; /*!< stores the irq associated to the ctimer used */
    uint32_t    interval;   /*!< the intervel between 2 calibrations, configured by application */

    /* internal use, application can ignore them */
    int32_t     hwCodeFlag; /*!< 1 to increase, -1 to decrease value in CAU_RCO_CAP_CFG */
    uint32_t    firstVal;   /*!< captured counter value at 1st 32k edge */
    uint32_t    lastVal;    /*!< captured counter value at last 32k edge */
    uint8_t     cnt;        /*!< the counter used to record 32k cycle count */
    uint32_t    total_calibrations_executed;        /*!< the total number of system calibrations done since power-up */
} rco32k_calib_env_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

void RCO32K_InitSwCalib(uint32_t interval);
void RCO32K_Calibrate(void);
void RCO32K_StopSwCalibration(void);
void RCO32K_UpdateSwCalibInterval(uint32_t interval);

#if defined(__cplusplus)
}
#endif

#endif /* _RCO32K_CALIBRATION_H_ */
