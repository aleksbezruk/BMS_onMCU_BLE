/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2017 , NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_device_registers.h"
#include "fsl_calibration.h"
#include "fsl_power.h"
#include "clock_config.h" /* for BOARD_XTAL0_CLK_HZ */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
void CALIB_CalibMainBangap(void)
{
    uint32_t bangap = 0;
    bangap = *((uint32_t *)FSL_FEATURE_FLASH_ADDR_OF_MAINBANDGAP_VOL);

    if ((bangap & 0xF) != 0xF)
    {
        SYSCON->ANA_CTRL1 &= ~SYSCON_ANA_CTRL1_IV_BG_SEL_MASK;
        SYSCON->ANA_CTRL1 |= (bangap << SYSCON_ANA_CTRL1_IV_BG_SEL_SHIFT);
    }
}

void CALIB_CalibPLL48M(void)
{
    uint32_t code;

    /* clear int */
    CALIB->INT_RAW = CALIB_INT_RAW_REF_CAL_DONE_INT_MASK;

    /* start calib */
    CALIB->START = CALIB_START_REF_CLB_START_MASK;
    /* wait until done */
    while (!(CALIB->INT_RAW & CALIB_INT_RAW_REF_CAL_DONE_INT_MASK))
    {
    }

    /* write code to cfg */
    code = CALIB->RCO_RC_REF_OSC_CODE & CALIB_RCO_RC_REF_OSC_CFG_PLL48_ENREF_CFG_MASK;
    CALIB->RCO_RC_REF_OSC_CFG &=
        (~(CALIB_RCO_RC_REF_OSC_CFG_REF_CAL_DIS_MASK | CALIB_RCO_RC_REF_OSC_CFG_PLL48_ENREF_CFG_MASK));
    CALIB->RCO_RC_REF_OSC_CFG |= (CALIB_RCO_RC_REF_OSC_CFG_REF_CAL_DIS_MASK | code);

    /* clear int */
    CALIB->INT_RAW = CALIB_INT_RAW_REF_CAL_DONE_INT_MASK;
}

/*!
 * @brief Calibration of RC.
 *
 * @param None.
 */
static void CALIB_CalibRC(void)
{
    uint32_t code;
    uint32_t i;

    /* power on for rc calib */
    SYSCON->PMU_CTRL2 &= ~SYSCON_PMU_CTRL2_RC_CAL_DIS_MASK;

    /* 1->0 to start rc calib */
    CALIB->RCO_RC_REF_OSC_CFG = (CALIB->RCO_RC_REF_OSC_CFG | CALIB_RCO_RC_REF_OSC_CFG_RC_CAL_REQ_MASK);
    CALIB->RCO_RC_REF_OSC_CFG = (CALIB->RCO_RC_REF_OSC_CFG & ~CALIB_RCO_RC_REF_OSC_CFG_RC_CAL_REQ_MASK);

    /* wait > 128us at 16M AHB(calibration needs AHB to be set to 16MHz).
     * only rc calib need delay before done, other calibs do not need. */
    for (i = 0U; i < 500U; ++i)
    {
        __asm("NOP");
    }

    /* wait until done */
    while (!(CALIB->DONE & CALIB_DONE_RC_CAL_DONE_MASK))
    {
    }

    /* write code to cfg, and disable rc calib */
    code = CALIB->RCO_RC_REF_OSC_CODE & CALIB_RCO_RC_REF_OSC_CFG_CAU_RC_CAL_REG_IN_MASK;
    CALIB->RCO_RC_REF_OSC_CFG =
        (CALIB->RCO_RC_REF_OSC_CFG &
         ~(CALIB_RCO_RC_REF_OSC_CFG_CAU_RC_CAL_REG_IN_MASK | CALIB_RCO_RC_REF_OSC_CFG_CAU_RC_CAL_DIS_MASK)) |
        code | CALIB_RCO_RC_REF_OSC_CFG_CAU_RC_CAL_DIS(1U);

    /* power off for rc calib */
    SYSCON->PMU_CTRL2 |= SYSCON_PMU_CTRL2_RC_CAL_DIS_MASK;
}

/*!
 * @brief Calibration of external High Frequency(16M or 32M) Crystal.
 *
 * Calibrates the XTAL's current for lower power consumption. Has nothing to do with XTAL's precision.
 *
 * @param None.
 */
static void CALIB_CalibXTAL(void)
{
    uint32_t code;

    /* power on OSC32M for XTAL calib */
    POWER_WritePmuCtrl1(SYSCON, SYSCON_PMU_CTRL1_OSC32M_DIS_MASK, SYSCON_PMU_CTRL1_OSC32M_DIS(0U));

    /* switch to OSC32M, and divide it to 16M */
    SYSCON->CLK_CTRL = (SYSCON->CLK_CTRL & ~(SYSCON_CLK_CTRL_SYS_CLK_SEL_MASK | SYSCON_CLK_CTRL_CLK_OSC32M_DIV_MASK)) |
                       SYSCON_CLK_CTRL_SYS_CLK_SEL(0U) | SYSCON_CLK_CTRL_CLK_OSC32M_DIV(1U);

    CALIB->CTRL = (CALIB->CTRL & ~(CALIB_CTRL_XTL_PO_TIM_MASK | CALIB_CTRL_XTL_CAL_TIM_MASK)) |
                  CALIB_CTRL_XTL_PO_TIM(3U)     /* XTL_PO_TIM_4MS */
                  | CALIB_CTRL_XTL_CAL_TIM(2U); /* XTL_CAL_TIM_10MS */

    CALIB->INT_RAW = CALIB_INT_RAW_XTL_CAL_DONE_INT_MASK;
    CALIB->START = CALIB_START_XTL_CLB_START_MASK;

    while (!(CALIB->INT_RAW & CALIB_INT_RAW_XTL_CAL_DONE_INT_MASK))
    {
    }

    code = CALIB->XTL_CODE & CALIB_XTL_CFG_XTL_XICTRL_CFG_MASK;
    CALIB->XTL_CFG = (CALIB->XTL_CFG & ~(CALIB_XTL_CFG_XTL_XICTRL_CFG_MASK | CALIB_XTL_CFG_XTL_CAL_DIS_MASK)) |
                     CALIB_XTL_CFG_XTL_CAL_DIS(1U) | code;

    CALIB->INT_RAW = CALIB_INT_RAW_XTL_CAL_DONE_INT_MASK;

    /* switch back to XTAL */
    SYSCON->CLK_CTRL = (SYSCON->CLK_CTRL & ~SYSCON_CLK_CTRL_SYS_CLK_SEL_MASK) | SYSCON_CLK_CTRL_SYS_CLK_SEL(1U);
}

/*!
 * @brief Calibration of internal High Frequency(32M) OSC.
 *
 * Calibrates the OSC32M to make it more precise.
 *
 * @param None.
 */
#define OSC_CAL_OV_TIME (0x20U)
static void CALIB_CalibOSC32M(void)
{
    int i, j;
    uint32_t code;
    uint32_t osc_cur = 0;

    /* power on OSC32M */
    POWER_WritePmuCtrl1(SYSCON, SYSCON_PMU_CTRL1_OSC32M_DIS_MASK, SYSCON_PMU_CTRL1_OSC32M_DIS(0U));

    for (i = 0x4; i >= 0; i--)
    {
        /* clear int */
        CALIB->INT_RAW = CALIB_INT_RAW_OSC_CAL_DONE_INT_MASK;

        /* disable the code */
        CALIB->RCO_RC_REF_OSC_CFG |= CALIB_RCO_RC_REF_OSC_CFG_OSC_CAL_DIS_MASK;

        osc_cur |= (1U << i);
        code = CALIB->RCO_RC_REF_OSC_CFG & ~CALIB_RCO_RC_REF_OSC_CFG_CAU_OSC_CUR_CFG_MASK;
        code |= (osc_cur << CALIB_RCO_RC_REF_OSC_CFG_CAU_OSC_CUR_CFG_SHIFT);
        CALIB->RCO_RC_REF_OSC_CFG = code;

        /* start calib */
        CALIB->START = CALIB_START_OSC_CLB_START_MASK;
        /* wait until done */
        j = 0;
        while (1)
        {
            /* calibration done, or time is up, need to increase code */
            if ((CALIB->INT_RAW & CALIB_INT_RAW_OSC_CAL_DONE_INT_MASK) || (j == OSC_CAL_OV_TIME))
            {
                break;
            }
            else
            {
                j++;
            }
        }

        /* clear disable of the code */
        CALIB->RCO_RC_REF_OSC_CFG &= ~CALIB_RCO_RC_REF_OSC_CFG_OSC_CAL_DIS_MASK;
        /* clear int */
        CALIB->INT_RAW = CALIB_INT_RAW_OSC_CAL_DONE_INT_MASK;
        /* check the code status */
        code = CALIB->RCO_RC_REF_OSC_CODE & CALIB_RCO_RC_REF_OSC_CODE_CAU_OSC_CUR_MASK;

        /* code is large, current bit need to change to 0 */
        if ((!((code >> CALIB_RCO_RC_REF_OSC_CODE_CAU_OSC_CUR_SHIFT) & 0x10U)))
        {
            osc_cur &= ~(1U << i);
        }
    }
    CALIB->RCO_RC_REF_OSC_CFG |= CALIB_RCO_RC_REF_OSC_CFG_OSC_CAL_DIS_MASK;

    /* power off OSC32M */
    POWER_WritePmuCtrl1(SYSCON, SYSCON_PMU_CTRL1_OSC32M_DIS_MASK, SYSCON_PMU_CTRL1_OSC32M_DIS(1U));
}

/*!
 * @brief Calibration of internal Low Frequency(32.000K) RCO.
 *
 * Calibrates the RCO32K to make it more precise.
 *
 * @param None.
 */
static void CALIB_CalibRCO32K(void)
{
    uint32_t code;

    /* clear int */
    CALIB->INT_RAW = CALIB_INT_RAW_RCO_CAL_DONE_INT_MASK;
    /* power on RCO32K */
    POWER_WritePmuCtrl1(SYSCON, SYSCON_PMU_CTRL1_RCO32K_DIS_MASK, SYSCON_PMU_CTRL1_RCO32K_DIS(0U));

    /* switch to RCO32K */
    SYSCON->CLK_CTRL = (SYSCON->CLK_CTRL & ~SYSCON_CLK_CTRL_CLK_32K_SEL_MASK) | SYSCON_CLK_CTRL_CLK_32K_SEL(1U);

    CALIB->RCO_RC_REF_OSC_CFG &= ~CALIB_RCO_RC_REF_OSC_CFG_RCO_CAL_DIS_MASK;

    /* start calib */
    CALIB->START = CALIB_START_RCO_CLB_START(1U);
    /* wait until done */
    while (!(CALIB->INT_RAW & CALIB_INT_RAW_RCO_CAL_DONE_INT_MASK))
    {
    }

    /* write code to cfg */
    code = CALIB->RCO_RC_REF_OSC_CODE & CALIB_RCO_RC_REF_OSC_CFG_CAU_RCO_CAP_CFG_MASK;
    CALIB->RCO_RC_REF_OSC_CFG =
        (CALIB->RCO_RC_REF_OSC_CFG &
         ~(CALIB_RCO_RC_REF_OSC_CFG_CAU_RCO_CAP_CFG_MASK | CALIB_RCO_RC_REF_OSC_CFG_RCO_CAL_DIS_MASK)) |
        code | CALIB_RCO_RC_REF_OSC_CFG_RCO_CAL_DIS(1U);

    /* clear int */
    CALIB->INT_RAW = CALIB_INT_RAW_RCO_CAL_DONE_INT_MASK;
}

static void CALIB_PowerOn(void)
{
    uint32_t code;

    /* clear int */
    CALIB->INT_RAW = CALIB_INT_RAW_PO_CAL_DONE_INT_MASK;
    /* start calib */
    CALIB->START = CALIB_START_PO_CLB_START(1U);
    /* wait until done */
    while (!(CALIB->INT_RAW & CALIB_INT_RAW_PO_CAL_DONE_INT_MASK))
    {
    }

    /* clear int */
    CALIB->INT_RAW = CALIB_INT_RAW_PO_CAL_DONE_INT_MASK;

    code = CALIB->VCOA_KVCO2M_CODE & CALIB_VCOA_KVCO2M_CFG_KCALF2M_CFG_MASK;
    CALIB->VCOA_KVCO2M_CFG = (CALIB->VCOA_KVCO2M_CFG & ~CALIB_VCOA_KVCO2M_CFG_KCALF2M_CFG_MASK) | code;

    CALIB->VCOF_KVCO_CFG =
        CALIB->VCOF_KVCO_PO_CODE | CALIB_VCOF_KVCO_CFG_VCOF_CAL_DIS(1U) | CALIB_VCOF_KVCO_CFG_KVCO_DIS(1U);

    BLEDP->DP_TOP_SYSTEM_CTRL = (BLEDP->DP_TOP_SYSTEM_CTRL &
                                 ~(BLEDP_DP_TOP_SYSTEM_CTRL_TX_REQ_MASK | BLEDP_DP_TOP_SYSTEM_CTRL_RX_REQ_MASK |
                                   BLEDP_DP_TOP_SYSTEM_CTRL_TX_EN_SEL_MASK | BLEDP_DP_TOP_SYSTEM_CTRL_RX_EN_SEL_MASK)) |
                                BLEDP_DP_TOP_SYSTEM_CTRL_TX_REQ(0U) | BLEDP_DP_TOP_SYSTEM_CTRL_RX_REQ(1U) |
                                BLEDP_DP_TOP_SYSTEM_CTRL_TX_EN_SEL(1U) | BLEDP_DP_TOP_SYSTEM_CTRL_RX_EN_SEL(1U);

    /* start calib */
    CALIB->VCOA_KVCO2M_CFG = (CALIB->VCOA_KVCO2M_CFG | CALIB_VCOA_KVCO2M_CFG_VCOA_CAL_REQ_MASK);
    CALIB->VCOA_KVCO2M_CFG = (CALIB->VCOA_KVCO2M_CFG & ~CALIB_VCOA_KVCO2M_CFG_VCOA_CAL_REQ_MASK);

    /* wait until done */
    while (!(CALIB->DONE & CALIB_DONE_VCOA_CAL_DONE_MASK))
    {
    }

    BLEDP->DP_TOP_SYSTEM_CTRL = (BLEDP->DP_TOP_SYSTEM_CTRL &
                                 ~(BLEDP_DP_TOP_SYSTEM_CTRL_TX_REQ_MASK | BLEDP_DP_TOP_SYSTEM_CTRL_RX_REQ_MASK |
                                   BLEDP_DP_TOP_SYSTEM_CTRL_TX_EN_SEL_MASK | BLEDP_DP_TOP_SYSTEM_CTRL_RX_EN_SEL_MASK)) |
                                BLEDP_DP_TOP_SYSTEM_CTRL_TX_REQ(1U) | BLEDP_DP_TOP_SYSTEM_CTRL_RX_REQ(0U) |
                                BLEDP_DP_TOP_SYSTEM_CTRL_TX_EN_SEL(1U) | BLEDP_DP_TOP_SYSTEM_CTRL_RX_EN_SEL(1U);

    /* start calib */
    CALIB->VCOA_KVCO2M_CFG = (CALIB->VCOA_KVCO2M_CFG | CALIB_VCOA_KVCO2M_CFG_VCOA_CAL_REQ_MASK);
    CALIB->VCOA_KVCO2M_CFG = (CALIB->VCOA_KVCO2M_CFG & ~CALIB_VCOA_KVCO2M_CFG_VCOA_CAL_REQ_MASK);

    /* wait until done */
    while (!(CALIB->DONE & CALIB_DONE_VCOA_CAL_DONE_MASK))
    {
    }

    CALIB->VCOA_KVCO2M_CFG = CALIB->VCOA_KVCO2M_CODE | CALIB_VCOA_KVCO2M_CFG_VCOA_CAL_DIS(1U);

    BLEDP->DP_TOP_SYSTEM_CTRL = (BLEDP->DP_TOP_SYSTEM_CTRL &
                                 ~(BLEDP_DP_TOP_SYSTEM_CTRL_TX_REQ_MASK | BLEDP_DP_TOP_SYSTEM_CTRL_RX_REQ_MASK |
                                   BLEDP_DP_TOP_SYSTEM_CTRL_TX_EN_SEL_MASK | BLEDP_DP_TOP_SYSTEM_CTRL_RX_EN_SEL_MASK)) |
                                BLEDP_DP_TOP_SYSTEM_CTRL_TX_REQ(0U) | BLEDP_DP_TOP_SYSTEM_CTRL_RX_REQ(0U) |
                                BLEDP_DP_TOP_SYSTEM_CTRL_TX_EN_SEL(0U) | BLEDP_DP_TOP_SYSTEM_CTRL_RX_EN_SEL(0U);

    CALIB->VCO_MOD_CFG = CALIB_VCO_MOD_CFG_IMR_MASK;
}

/*!
 * @brief Calibration at system startup.
 *
 * Do a series of calibrations at system startup.
 * Note: bus clock(i.e. AHB clock) must be set to 16M.
 *
 * @param None.
 */
void CALIB_SystemCalib(void)
{
    CLOCK_AttachClk((BOARD_XTAL0_CLK_HZ == CLK_XTAL_32MHZ) ? k32M_to_XTAL_CLK : k16M_to_XTAL_CLK);

    /* Configure AHB clock, AHBCLK = SYSCLK/(div+1) */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, (BOARD_XTAL0_CLK_HZ == CLK_XTAL_32MHZ) ? 1U : 0U);

    /* SYSCLK comes from XTAL */
    CLOCK_AttachClk(kXTAL_to_SYS_CLK);

    /* Enable ble clock and calibration clock */
    SYSCON->CLK_EN = SYSCON_CLK_EN_CLK_BLE_EN_MASK | SYSCON_CLK_EN_CLK_CAL_EN_MASK;

    CALIB_CalibRC();
    CALIB_CalibXTAL();
    CALIB_CalibOSC32M();
    CALIB_CalibRCO32K();
    CALIB_PowerOn();
    CALIB_CalibMainBangap();
}
