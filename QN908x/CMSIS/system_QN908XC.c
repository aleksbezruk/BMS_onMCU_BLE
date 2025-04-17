/*
** ###################################################################
**     Processors:          QN9080C
**                          QN9083C
**
**     Compilers:           Keil ARM C/C++ Compiler
**                          GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          MCUXpresso Compiler
**
**     Reference manual:    QN908X User manual Rev.1.0 21 Mar 2017
**     Version:             rev. 1.0, 2017-03-27
**     Build:               b170328
**
**     Abstract:
**         Provides a system configuration function and a global variable that
**         contains the system frequency. It configures the device and initializes
**         the oscillator (PLL) that is part of the microcontroller device.
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2017 NXP
**     All rights reserved.
**     
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2017-03-27)
**         Initial version.
**
** ###################################################################
*/

/*!
 * @file QN908XC
 * @version 1.0
 * @date 2017-03-27
 * @brief Device specific configuration file for QN908XC (implementation file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#include <stdint.h>
#include "fsl_device_registers.h"

extern void *__Vectors;


/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */

uint32_t SystemCoreClock = DEFAULT_SYSTEM_CLOCK;

/* ----------------------------------------------------------------------------
   -- SystemInit()
   ---------------------------------------------------------------------------- */

void SystemInit (void) {
#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
  SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));    /* set CP10, CP11 Full Access */
#endif /* ((__FPU_PRESENT == 1) && (__FPU_USED == 1)) */

    SCB->VTOR = (uint32_t)&__Vectors;

    /* Disable watchdog timer */
    WDT->LOCK = 0x1ACCE551;
    WDT->CTRL = 0;
    WDT->LOCK = 0;

    /* Speed up the startup of XTAL by decrease load cap */
    SYSCON->XTAL_CTRL = (SYSCON->XTAL_CTRL &
                         ~(SYSCON_XTAL_CTRL_XTAL_SU_CA_REG_MASK | SYSCON_XTAL_CTRL_XTAL_SU_CB_REG_MASK |
                           SYSCON_XTAL_CTRL_XTAL_XCUR_BOOST_REG_MASK)) |
                        SYSCON_XTAL_CTRL_XTAL_SU_CA_REG(0U) | SYSCON_XTAL_CTRL_XTAL_SU_CB_REG(0U) |
                        SYSCON_XTAL_CTRL_XTAL_XCUR_BOOST_REG_MASK;

    /* Change crystal load cap to (0.35 * 0x08 + 5) = 7.8pF for DK board, and half of default voltage */
    SYSCON->ANA_CTRL0 = (SYSCON->ANA_CTRL0 & ~(SYSCON_ANA_CTRL0_XTAL_LOAD_CAP_MASK | SYSCON_ANA_CTRL0_XTAL_AMP_MASK)) |
                        SYSCON_ANA_CTRL0_XTAL_LOAD_CAP(8U) | SYSCON_ANA_CTRL0_XTAL_AMP(2U);

    /* Adjust mem & pmu voltage */
    SYSCON->ANA_CTRL1 = (SYSCON->ANA_CTRL1 &
                         ~(SYSCON_ANA_CTRL1_VDD_PMU_SET_PDM_MASK | SYSCON_ANA_CTRL1_VDD_PMU_SET_MASK |
                           SYSCON_ANA_CTRL1_VDD_MEM_SET_PDM_MASK | SYSCON_ANA_CTRL1_VDD_MEM_SET_MASK |
                           SYSCON_ANA_CTRL1_VDD_PMU_SET_ULTRA_LOW_MASK | SYSCON_ANA_CTRL1_IV_VREG11_SET_MASK |
                           SYSCON_ANA_CTRL1_DVREG11_SET_DIG_MASK)) |
                        SYSCON_ANA_CTRL1_VDD_PMU_SET_PDM(3U) | SYSCON_ANA_CTRL1_VDD_PMU_SET(3U) |
                        SYSCON_ANA_CTRL1_VDD_MEM_SET_PDM(3U) | SYSCON_ANA_CTRL1_VDD_MEM_SET(3U) |
                        SYSCON_ANA_CTRL1_VDD_PMU_SET_ULTRA_LOW(0U) | SYSCON_ANA_CTRL1_IV_VREG11_SET(2U) |
                        SYSCON_ANA_CTRL1_DVREG11_SET_DIG(2U);

    /* Flatten RX sensitivity across 2402-2480M */
    CALIB->RRF1 = (CALIB->RRF1 & ~(CALIB_RRF1_RRF_RX_INCAP1_MASK | CALIB_RRF1_RRF_LOAD_CAP_MASK)) |
                  CALIB_RRF1_RRF_RX_INCAP1(4U) | CALIB_RRF1_RRF_LOAD_CAP(6U);

    /* Sub module clock setting:
     * enable Data Path 16/8MHz clock(some of the flash operations need this, too)
     * enable BiV clock include RTC BiV register  */
    SYSCON->CLK_EN = SYSCON_CLK_EN_CLK_DP_EN_MASK | SYSCON_CLK_EN_CLK_BIV_EN_MASK;

    /* Workaround: only use bjt comparator to save buck power.
     * fix for buck's frequency (tune tmos to adjust efficiency of buck)
     * increase buck constant on time to get better RX and TX performance */
    SYSCON->BUCK = (SYSCON->BUCK & ~(SYSCON_BUCK_BUCK_TMOS_MASK | SYSCON_BUCK_BUCK_ISEL_MASK)) |
                   SYSCON_BUCK_BUCK_TMOS(0x0EU) | SYSCON_BUCK_BUCK_ISEL(0x2U);

#if defined(CPU_QN9080C)
    /* Use differential lo clock, tx&rx: close loop mode */
    CALIB->LO1 = (CALIB->LO1 &
                  ~(CALIB_LO1_DIV_DIFF_CLK_LO_DIS_MASK | CALIB_LO1_TX_PLLPFD_EN_MASK | CALIB_LO1_RX_PLLPFD_EN_MASK)) |
                 CALIB_LO1_DIV_DIFF_CLK_LO_DIS(0x0U) | CALIB_LO1_TX_PLLPFD_EN(0x1U) | CALIB_LO1_RX_PLLPFD_EN(0x1U);
#else /* CPU_QN9083C */
    CALIB->LO0 |= CALIB_LO0_VCO_DSM_INT_EN_MASK;
    /* Use differential lo clock, tx: close loop mode, rx: open loop mode */
    CALIB->LO1 = (CALIB->LO1 &
                  ~(CALIB_LO1_DIV_DIFF_CLK_LO_DIS_MASK | CALIB_LO1_TX_PLLPFD_EN_MASK | CALIB_LO1_RX_PLLPFD_EN_MASK)) |
                 CALIB_LO1_DIV_DIFF_CLK_LO_DIS(0x0U) | CALIB_LO1_TX_PLLPFD_EN(0x1U) | CALIB_LO1_RX_PLLPFD_EN(0x0U);
#endif

    /* AA_ERROR */
    BLEDP->DP_AA_ERROR_CTRL = 0x0000000EU;

    /* Bypass hop calibration delay, then it is a little earlier than tx/rx */
    CALIB->CAL_DLY |= CALIB_CAL_DLY_HOP_DLY_BP_MASK;

    /* Power down the analog part at rx state when the air signal is over */
    SYSCON->PMU_CTRL2 = (SYSCON->PMU_CTRL2 & ~SYSCON_PMU_CTRL2_RX_EN_SEL_MASK) | SYSCON_PMU_CTRL2_RX_EN_SEL(0x0U);

    /* Wait for XTAL ready */
    while (!(SYSCON->SYS_MODE_CTRL & SYSCON_SYS_MODE_CTRL_XTAL_RDY_MASK))
    {
    }
}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */

void SystemCoreClockUpdate (void) {

    switch ((SYSCON->CLK_CTRL & SYSCON_CLK_CTRL_SYS_CLK_SEL_MASK) >> SYSCON_CLK_CTRL_SYS_CLK_SEL_SHIFT)
    {
        case 0x00:
            SystemCoreClock =
                (SYSCON->CLK_CTRL & SYSCON_CLK_CTRL_CLK_OSC32M_DIV_MASK) ? CLK_OSC_32MHZ / 2 : CLK_OSC_32MHZ;
            break;
        case 0x01:
            SystemCoreClock = ((SYSCON->CLK_CTRL & SYSCON_CLK_CTRL_CLK_XTAL_SEL_MASK) &&
                               (!(SYSCON->XTAL_CTRL & SYSCON_XTAL_CTRL_XTAL_DIV_MASK))) ?
                                  CLK_XTAL_32MHZ :
                                  CLK_XTAL_16MHZ;
            break;
        case 0x02:
            SystemCoreClock = (SYSCON->CLK_CTRL & SYSCON_CLK_CTRL_CLK_32K_SEL_MASK) ? CLK_RCO_32KHZ : CLK_XTAL_32KHZ;
            break;
        default:
            break;
    }
    SystemCoreClock = SystemCoreClock / (((SYSCON->CLK_CTRL & SYSCON_CLK_CTRL_AHB_DIV_MASK) >> SYSCON_CLK_CTRL_AHB_DIV_SHIFT) + 1);
}
