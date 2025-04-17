/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2017 , NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_CALIB_H_
#define _FSL_CALIB_H_

#include "fsl_common.h"

/*!
 * @addtogroup calibration
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief QN9080 calibration version 2.0.0. */
#define FSL_CALIB_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief  System Calibration
 */
void CALIB_SystemCalib(void);

/*!
 * @brief  PLL Calibration
 */
void CALIB_CalibPLL48M(void);

/*!
 * @brief  Main bangap Calibration
 */
void CALIB_CalibMainBangap(void);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FSL_CALIB_H_ */
