/*! *********************************************************************************
* Copyright 2013-2015, Freescale Semiconductor, Inc.
* Copyright 2016-2018 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#ifndef _CLOCK_CONFIG_H_
#define _CLOCK_CONFIG_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Configure high frequency XTAL according to board design: 16MHz, 32MHz or 0U (no XTAL crystal on board) */
#define BOARD_XTAL0_CLK_HZ 32000000U

/* Configure low frequency XTAL according to board design: 32768U or 0U (no 32K crystal on board) */
#define BOARD_XTAL1_CLK_HZ 32768U

/*******************************************************************************
 ************************ BOARD_InitBootClocks function ************************
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief This function executes default configuration of clocks.
 *
 */
void BOARD_InitBootClocks(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*******************************************************************************
 ********************* Configuration BOARD_BootClockLSRUN **********************
 ******************************************************************************/
/*******************************************************************************
 * Definitions for BOARD_BootClockLSRUN configuration
 ******************************************************************************/
/* In the BOARD_BOOTCLOCKLSRUN clock configuration  AHB clock frequency = 8000000U */

/*******************************************************************************
 * API for BOARD_BootClockLSRUN configuration
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief This function executes configuration of clocks.
 *
 */
void BOARD_BootClockLSRUN(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*******************************************************************************
 ********************** Configuration BOARD_BootClockRUN ***********************
 ******************************************************************************/
/*******************************************************************************
 * Definitions for BOARD_BootClockRUN configuration
 ******************************************************************************/
/* In the BOARD_BOOTCLOCKRUN clock configuration  AHB clock frequency = 16000000U */

/*******************************************************************************
 * API for BOARD_BootClockRUN configuration
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief This function executes configuration of clocks.
 *
 */
void BOARD_BootClockRUN(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*******************************************************************************
 ********************* Configuration BOARD_BootClockHSRUN **********************
 ******************************************************************************/
/*******************************************************************************
 * Definitions for BOARD_BootClockHSRUN configuration
 ******************************************************************************/
/* In the BOARD_BOOTCLOCKHSRUN clock configuration  AHB clock frequency = 32000000U */

/*******************************************************************************
 * API for BOARD_BootClockHSRUN configuration
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief This function executes configuration of clocks.
 *
 */
void BOARD_BootClockHSRUN(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

#endif /* _CLOCK_CONFIG_H_ */
