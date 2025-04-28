/*! *********************************************************************************
* Copyright 2013-2015, Freescale Semiconductor, Inc.
* Copyright 2016-2018 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

/*
 * How to set up clock using clock driver functions:
 *
 * 1. Setup clock sources.
 *
 * 2. Set up all selectors to provide selected clocks.
 *
 * 3. Set up all dividers.
 *
 * 4. Set up clock out source and pins.
 *
 * 5. Set up the specified peripheral clock.
 */

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Clocks v3.0
processor: QN908XC
package_id: QN9080C
mcu_data: ksdk2_0
processor_version: 2.0.0
board: QN908XCDK
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

#include "fsl_power.h"
#include "fsl_common.h"
#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* System clock frequency. */
extern uint32_t SystemCoreClock;

/*******************************************************************************
 ************************ BOARD_InitBootClocks function ************************
 ******************************************************************************/
void BOARD_InitBootClocks(void)
{
    BOARD_BootClockRUN();
}

/*******************************************************************************
 ********************* Configuration BOARD_BootClockLSRUN **********************
 ******************************************************************************/
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!Configuration
name: BOARD_BootClockLSRUN
outputs:
- {id: AHB_clock.outFreq, value: 8 MHz}
- {id: APB_clock.outFreq, value: 8 MHz}
- {id: CLK_32K_clock.outFreq, value: 32.768 kHz}
- {id: RTC_clock.outFreq, value: 8 MHz}
- {id: WDT_clock.outFreq, value: 8 MHz}
settings:
- {id: SYSCON.AHB_DIV.scale, value: '2', locked: true}
- {id: SYSCON.AUTO_M_MULT.scale, value: '6'}
- {id: SYSCON.BLE_DIV.scale, value: '1'}
- {id: SYSCON.CLK_32K_SEL.sel, value: SYSCON.XTAL32K}
- {id: SYSCON.CLK_SYS_SEL.sel, value: SYSCON.XTAL_DIV}
- {id: SYSCON.XTAL_DIV.scale, value: '2', locked: true}
sources:
- {id: SYSCON.XTAL.outFreq, value: 32 MHz, enabled: true}
- {id: SYSCON.XTAL32K.outFreq, value: 32.768 kHz, enabled: true}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/*******************************************************************************
 * Variables for BOARD_BootClockLSRUN configuration
 ******************************************************************************/
/*******************************************************************************
 * Code for BOARD_BootClockLSRUN configuration
 ******************************************************************************/
void BOARD_BootClockLSRUN(void)
{
    /*!< Power up/Power down the module. */

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(k32M_to_XTAL_CLK);          /*!< Switch XTAL_CLK to 32M */
#if (defined(BOARD_XTAL1_CLK_HZ) && (BOARD_XTAL1_CLK_HZ == CLK_XTAL_32KHZ))
    CLOCK_AttachClk(kXTAL32K_to_32K_CLK);       /*!< Switch 32K_CLK to XTAL32K (32768 Hz) */
#else
    CLOCK_AttachClk(kRCO32K_to_32K_CLK);        /*!< Switch 32K_CLK to RCO (~32000 Hz) */
                                                /*    Needs calibration!  */
#endif

    CLOCK_AttachClk(kXTAL_to_SYS_CLK);          /*!< Switch SYS_CLK to XTAL */
    CLOCK_AttachClk(kAPB_to_WDT_CLK);           /*!< Switch WDT_CLK to APB */

    /*!< Set up dividers */
    CLOCK_SetClkDiv(kCLOCK_DivOsc32mClk, 1U);   /*!< Set OSC32M_DIV divider to value 2 */
    CLOCK_SetClkDiv(kCLOCK_DivXtalClk, 1U);     /*!< Set XTAL_DIV divider to value 2 */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U);      /*!< Set AHB_DIV divider to value 2 */
    CLOCK_SetClkDiv(kCLOCK_DivFrg1, 0U);        /*!< Set FRG_MULT1 to value 0, Set FRG_DIV1 to value 255 */
    CLOCK_SetClkDiv(kCLOCK_DivFrg0, 0U);        /*!< Set FRG_MULT0 to value 0, Set FRG_DIV0 to value 255 */
    CLOCK_SetClkDiv(kCLOCK_DivApbClk, 0U);      /*!< Set APB_DIV divider to value 1 */

    /*!< Enable/Disable clock out source and pins.*/

    /*!< Enable/Disable the specified peripheral clock.*/

}

/*******************************************************************************
 ********************** Configuration BOARD_BootClockRUN ***********************
 ******************************************************************************/
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!Configuration
name: BOARD_BootClockRUN
called_from_default_init: true
outputs:
- {id: AHB_clock.outFreq, value: 16 MHz}
- {id: APB_clock.outFreq, value: 16 MHz}
- {id: BLE_clock.outFreq, value: 8 MHz}
- {id: CLK_32K_clock.outFreq, value: 32.768 kHz}
- {id: GPIO_clock.outFreq, value: 16 MHz}
- {id: RTC_clock.outFreq, value: 16 MHz}
- {id: WDT_clock.outFreq, value: 32.768 kHz}
settings:
- {id: BLECLK_OUTPUT_ENDI, value: Enabled}
- {id: Data_Path_ENDI, value: Enabled}
- {id: GPIOCLK_OUTPUT_ENDI, value: Enabled}
- {id: SYSCON.AUTO_M_MULT.scale, value: '6'}
- {id: SYSCON.CLK_32K_SEL.sel, value: SYSCON.XTAL32K}
- {id: SYSCON.CLK_SYS_SEL.sel, value: SYSCON.XTAL_DIV}
- {id: SYSCON.CLK_WDT_SEL.sel, value: SYSCON.CLK_32K_SEL}
- {id: SYSCON.XTAL_DIV.scale, value: '2', locked: true}
sources:
- {id: SYSCON.XTAL.outFreq, value: 32 MHz, enabled: true}
- {id: SYSCON.XTAL32K.outFreq, value: 32.768 kHz, enabled: true}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/*******************************************************************************
 * Variables for BOARD_BootClockRUN configuration
 ******************************************************************************/
/*******************************************************************************
 * Code for BOARD_BootClockRUN configuration
 ******************************************************************************/
void BOARD_BootClockRUN(void)
{
    /*!< Power up/Power down the module. */

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(k32M_to_XTAL_CLK);          /*!< Switch XTAL_CLK to 32M */
#if (defined(BOARD_XTAL1_CLK_HZ) && (BOARD_XTAL1_CLK_HZ == CLK_XTAL_32KHZ))
    CLOCK_AttachClk(kXTAL32K_to_32K_CLK);       /*!< Switch 32K_CLK to XTAL32K (32768 Hz) */
#else
    CLOCK_AttachClk(kRCO32K_to_32K_CLK);        /*!< Switch 32K_CLK to RCO (~32000 Hz) */
                                                /*    Needs calibration!  */
#endif

    CLOCK_AttachClk(kXTAL_to_SYS_CLK);          /*!< Switch SYS_CLK to XTAL */
    CLOCK_AttachClk(k32K_to_WDT_CLK);           /*!< Switch WDT_CLK to 32K */
    CLOCK_AttachClk(k8M_to_BLE_CLK);            /*!< Switch BLE_CLK to 8M */

    /*!< Set up dividers */
    CLOCK_SetClkDiv(kCLOCK_DivOsc32mClk, 1U);   /*!< Set OSC32M_DIV divider to value 2 */
    CLOCK_SetClkDiv(kCLOCK_DivXtalClk, 1U);     /*!< Set XTAL_DIV divider to value 2 */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 0U);      /*!< Set AHB_DIV divider to value 1 */
    CLOCK_SetClkDiv(kCLOCK_DivFrg1, 0U);        /*!< Set FRG_MULT1 to value 0, Set FRG_DIV1 to value 255 */
    CLOCK_SetClkDiv(kCLOCK_DivFrg0, 0U);        /*!< Set FRG_MULT0 to value 0, Set FRG_DIV0 to value 255 */
    CLOCK_SetClkDiv(kCLOCK_DivApbClk, 0U);      /*!< Set APB_DIV divider to value 1 */

    /*!< Enable/Disable clock out source and pins.*/

    /*!< Enable/Disable the specified peripheral clock.*/
    CLOCK_EnableClock(kCLOCK_Gpio);             /*!< Enable Gpio clock */
    CLOCK_EnableClock(kCLOCK_Dp);               /*!< Enable Dp clock */
    CLOCK_EnableClock(kCLOCK_Ble);              /*!< Enable Ble clock */

}

/*******************************************************************************
 ********************* Configuration BOARD_BootClockHSRUN **********************
 ******************************************************************************/
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!Configuration
name: BOARD_BootClockHSRUN
outputs:
- {id: AHB_clock.outFreq, value: 32 MHz}
- {id: APB_clock.outFreq, value: 32 MHz}
- {id: BLE_clock.outFreq, value: 16 MHz}
- {id: CLK_32K_clock.outFreq, value: 32.768 kHz}
- {id: GPIO_clock.outFreq, value: 32 MHz}
- {id: RTC_clock.outFreq, value: 32 MHz}
- {id: WDT_clock.outFreq, value: 32 MHz}
settings:
- {id: BLECLK_OUTPUT_ENDI, value: Enabled}
- {id: BLECLK_OUTPUT_SEL, value: _16MHz}
- {id: Data_Path_ENDI, value: Enabled}
- {id: GPIOCLK_OUTPUT_ENDI, value: Enabled}
- {id: SYSCON.AHB_DIV.scale, value: '1', locked: true}
- {id: SYSCON.BLE_DIV.scale, value: '2', locked: true}
- {id: SYSCON.CLK_32K_SEL.sel, value: SYSCON.XTAL32K}
- {id: SYSCON.CLK_SYS_SEL.sel, value: SYSCON.XTAL_DIV}
- {id: SYSCON.XTAL_DIV.scale, value: '1', locked: true}
- {id: 'SYSCON::CLK_CTRL[CLK_BLE_SEL].bitField', value: BLE run at 16M}
sources:
- {id: SYSCON.XTAL.outFreq, value: 32 MHz, enabled: true}
- {id: SYSCON.XTAL32K.outFreq, value: 32.768 kHz, enabled: true}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/*******************************************************************************
 * Variables for BOARD_BootClockHSRUN configuration
 ******************************************************************************/
/*******************************************************************************
 * Code for BOARD_BootClockHSRUN configuration
 ******************************************************************************/
void BOARD_BootClockHSRUN(void)
{
    /*!< Power up/Power down the module. */

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(k32M_to_XTAL_CLK);          /*!< Switch XTAL_CLK to 32M */
#if (defined(BOARD_XTAL1_CLK_HZ) && (BOARD_XTAL1_CLK_HZ == CLK_XTAL_32KHZ))
    CLOCK_AttachClk(kXTAL32K_to_32K_CLK);       /*!< Switch 32K_CLK to XTAL32K (32768 Hz) */
#else
    CLOCK_AttachClk(kRCO32K_to_32K_CLK);        /*!< Switch 32K_CLK to RCO (~32000 Hz) */
                                                /*    Needs calibration!  */
#endif

    CLOCK_AttachClk(kXTAL_to_SYS_CLK);          /*!< Switch SYS_CLK to XTAL */
    CLOCK_AttachClk(kAPB_to_WDT_CLK);           /*!< Switch WDT_CLK to APB */
    CLOCK_AttachClk(k16M_to_BLE_CLK);                  /*!< Switch BLE_CLK to 16M */

    /*!< Set up dividers */
    CLOCK_SetClkDiv(kCLOCK_DivOsc32mClk, 1U);   /*!< Set OSC32M_DIV divider to value 2 */
    CLOCK_SetClkDiv(kCLOCK_DivXtalClk, 0U);     /*!< Set XTAL_DIV divider to value 1 */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 0U);      /*!< Set AHB_DIV divider to value 1 */
    CLOCK_SetClkDiv(kCLOCK_DivFrg1, 0U);        /*!< Set FRG_MULT1 to value 0, Set FRG_DIV1 to value 255 */
    CLOCK_SetClkDiv(kCLOCK_DivFrg0, 0U);        /*!< Set FRG_MULT0 to value 0, Set FRG_DIV0 to value 255 */
    CLOCK_SetClkDiv(kCLOCK_DivApbClk, 0U);      /*!< Set APB_DIV divider to value 1 */

    /*!< Enable/Disable clock out source and pins.*/

    /*!< Enable/Disable the specified peripheral clock.*/
    CLOCK_EnableClock(kCLOCK_Gpio);       /*!< Enable Gpio clock */
    CLOCK_EnableClock(kCLOCK_Dp);       /*!< Enable Dp clock */
    CLOCK_EnableClock(kCLOCK_Ble);       /*!< Enable Ble clock */

}

