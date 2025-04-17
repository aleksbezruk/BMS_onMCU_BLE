/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2017 , NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_RF_H_
#define _FSL_RF_H_

#include "fsl_common.h"

/*! @addtogroup rf */
/*! @{ */

/*! @file */

/*******************************************************************************
 * Definitions
 *****************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief QN9080 radio frequency version 2.0.0. */
#define FSL_QN9080_RADIO_FREQUENCY_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*!
 * @brief  Output power at SMA socket, depends on off-chip RF circuit on PCB.
 */
typedef enum _tx_power
{
    kTxPowerMinimum = 0U,    /*!< <-30dBm */
    kTxPowerMinus30dBm,      /*!< -30 dBm */
    kTxPowerMinus25dBm,      /*!< -25 dBm */
    kTxPowerMinus20dBm,      /*!< -20 dBm */
    kTxPowerMinus18dBm,      /*!< -18 dBm */
    kTxPowerMinus16dBm,      /*!< -16 dBm */
    kTxPowerMinus14dBm,      /*!< -14 dBm */
    kTxPowerMinus12dBm,      /*!< -12 dBm */
    kTxPowerMinus10dBm,      /*!< -10 dBm */
    kTxPowerMinus9dBm,       /*!< -9  dBm */
    kTxPowerMinus8dBm,       /*!< -8  dBm */
    kTxPowerMinus7dBm,       /*!< -7  dBm */
    kTxPowerMinus6dBm,       /*!< -6  dBm */
    kTxPowerMinus5dBm,       /*!< -5  dBm */
    kTxPowerMinus4dBm,       /*!< -4  dBm */
    kTxPowerMinus3dBm,       /*!< -3  dBm */
    kTxPowerMinus2dBm,       /*!< -2  dBm */
    kTxPowerMinus1dBm,       /*!< -1  dBm */
    kTxPower0dBm,            /*!<  0  dBm */
    kTxPower1dBm,            /*!<  1  dBm */
    kTxPower2dBm,            /*!<  2  dBm */
    kTxPowerInvalid
} tx_power_t;

/*!
 * @brief  Rx mode of the chip.
 */
typedef enum _tx_mode
{
    kRxModeHighPerformance = 0U, /*!< High performance mode, but the power consumption is high. */
    kRxModeBalanced,             /*!< Balanced mode, a trade-off between performance and power consumpiton. */
    kRxModeHighEfficiency,       /*!< High efficiency mode, but rx performance will not be so good. */
} rx_mode_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @brief  Set Tx Power setting
 * @param   txpwr  tx power level defined by enum @tx_power_t
 */
void RF_SetTxPowerLevel(SYSCON_Type *base, tx_power_t txpwr);

/*!
 * @brief  Get Tx Power setting value
 * @return Tx Power setting value defined by enum @tx_power_t
 */
tx_power_t RF_GetTxPowerLevel(SYSCON_Type *base);

/*!
 * @brief  Set Rx mode
 * @param  Rx mode defined by enum @rx_mode_t
 */
void RF_ConfigRxMode(SYSCON_Type *base, rx_mode_t rm);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @} */

#endif /* _FSL_RF_H_ */
