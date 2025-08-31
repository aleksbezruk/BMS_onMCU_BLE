/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2017 , NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_rf.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static const uint8_t rf_tx_gain_tab[kTxPowerInvalid] = {
        [kTxPowerMinimum]    = 0x01, /*!<  <-30dBm */
        [kTxPowerMinus30dBm] = 0x02, /*!< -30 dBm */
        [kTxPowerMinus25dBm] = 0x05, /*!< -25 dBm */
        [kTxPowerMinus20dBm] = 0x09, /*!< -20 dBm */
        [kTxPowerMinus18dBm] = 0x0B, /*!< -18 dBm */
        [kTxPowerMinus16dBm] = 0x0E, /*!< -16 dBm */
        [kTxPowerMinus14dBm] = 0x11, /*!< -14 dBm */
        [kTxPowerMinus12dBm] = 0x16, /*!< -12 dBm */
        [kTxPowerMinus10dBm] = 0x1D, /*!< -10 dBm */
        [kTxPowerMinus9dBm]  = 0x20, /*!<  -9 dBm */
        [kTxPowerMinus8dBm]  = 0x25, /*!<  -8 dBm */
        [kTxPowerMinus7dBm]  = 0x2B, /*!<  -7 dBm */
        [kTxPowerMinus6dBm]  = 0x31, /*!<  -6 dBm */
        [kTxPowerMinus5dBm]  = 0x3A, /*!<  -5 dBm */
        [kTxPowerMinus4dBm]  = 0x41, /*!<  -4 dBm */
        [kTxPowerMinus3dBm]  = 0x4D, /*!<  -3 dBm */
        [kTxPowerMinus2dBm]  = 0x5D, /*!<  -2 dBm */
        [kTxPowerMinus1dBm]  = 0x70, /*!<  -1 dBm */
        [kTxPower0dBm] = 0x89,       /*!<   0 dBm */
        [kTxPower1dBm] = 0xB3,       /*!<   1 dBm */
        [kTxPower2dBm] = 0xFB,       /*!<   2 dBm */
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

void RF_SetTxPowerLevel(SYSCON_Type *base, tx_power_t txpwr)
{
    base->ANA_CTRL0 = (base->ANA_CTRL0 & ~SYSCON_ANA_CTRL0_PA_POWER_MASK) | SYSCON_ANA_CTRL0_PA_POWER(rf_tx_gain_tab[txpwr]);
}

tx_power_t RF_GetTxPowerLevel(SYSCON_Type *base)
{
    uint8_t cnt;
    tx_power_t level = kTxPowerInvalid;
    uint32_t val = base->ANA_CTRL0 & SYSCON_ANA_CTRL0_PA_POWER_MASK;

    for (cnt = 0U; cnt < kTxPowerInvalid; cnt++)
    {
        if (rf_tx_gain_tab[cnt] == val)
        {
            level = (tx_power_t)cnt;
            break;
        }
    }

    return level;
}

void RF_ConfigRxMode(SYSCON_Type *base, rx_mode_t rm)
{
    switch (rm)
    {
        case kRxModeHighEfficiency:
            BLEDP->DP_TOP_SYSTEM_CTRL = (BLEDP->DP_TOP_SYSTEM_CTRL & ~BLEDP_DP_TOP_SYSTEM_CTRL_DET_MODE_MASK);
            /* reduce LNA current to 50% */
            CALIB->RRF1 = (CALIB->RRF1 & ~CALIB_RRF1_RRF_BM_LNA_MASK) | CALIB_RRF1_RRF_BM_LNA(0x0U);
            /* reduce PPF current to 50% */
            /* CALIB->PLL48_PPF = (CALIB->PLL48_PPF & ~CALIB_PLL48_PPF_PPF_BM_MASK) | CALIB_PLL48_PPF_PPF_BM(0x0U);*/
            break;
        case kRxModeBalanced:
            break;
        case kRxModeHighPerformance:
        default:
            /* LNA current */
            CALIB->RRF1 = (CALIB->RRF1 & ~CALIB_RRF1_RRF_BM_LNA_MASK) | CALIB_RRF1_RRF_BM_LNA(0x3U);
            /* PPF current */
            CALIB->PLL48_PPF = (CALIB->PLL48_PPF & ~CALIB_PLL48_PPF_PPF_BM_MASK) | CALIB_PLL48_PPF_PPF_BM(0x3U);
            break;
    }
}
