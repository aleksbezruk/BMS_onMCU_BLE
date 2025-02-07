/*******************************************************************************
 * File Name: cycfg_peripherals.c
 *
 * Description:
 * Peripheral Hardware Block configuration
 * This file was automatically generated and should not be modified.
 * Configurator Backend 3.20.0
 * device-db 4.16.0.6098
 * mtb-pdl-cat1 3.11.0.34443
 *
 *******************************************************************************
 * Copyright 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#include "cycfg_peripherals.h"

#if defined (CY_USING_HAL)
const cyhal_resource_inst_t CYBSP_BLE_obj =
{
    .type = CYHAL_RSC_BLESS,
    .block_num = 0U,
    .channel_num = 0U,
};
#endif /* defined (CY_USING_HAL) */

const cy_stc_sysanalog_config_t pass_0_aref_0_config =
{
    .startup = CY_SYSANALOG_STARTUP_FAST,
    .iztat = CY_SYSANALOG_IZTAT_SOURCE_LOCAL,
    .vref = CY_SYSANALOG_VREF_SOURCE_LOCAL_1_2V,
    .deepSleep = CY_SYSANALOG_DEEPSLEEP_DISABLE,
};
const cy_stc_sar_config_t pass_0_sar_0_config =
{
    .ctrl = (uint32_t) pass_0_sar_0_CTL,
    .sampleCtrl = (uint32_t) pass_0_sar_0_SAMPLE,
    .sampleTime01 = (1023UL << (uint32_t)CY_SAR_SAMPLE_TIME0_SHIFT) | (1023UL << (uint32_t)CY_SAR_SAMPLE_TIME1_SHIFT),
    .sampleTime23 = (2UL << (uint32_t)CY_SAR_SAMPLE_TIME2_SHIFT) | (2UL << (uint32_t)CY_SAR_SAMPLE_TIME3_SHIFT),
    .rangeThres = (0UL << (uint32_t)CY_SAR_RANGE_HIGH_SHIFT) | (0UL << (uint32_t)CY_SAR_RANGE_LOW_SHIFT),
    .rangeCond = CY_SAR_RANGE_COND_BELOW,
    .chanEn = 15UL,
    .chanConfig = {(uint32_t) pass_0_sar_0_CH0_CONFIG, (uint32_t) pass_0_sar_0_CH1_CONFIG, (uint32_t) pass_0_sar_0_CH2_CONFIG, (uint32_t) pass_0_sar_0_CH3_CONFIG, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL},
    .intrMask = CY_SAR_INTR_EOS,
    .satIntrMask = 0UL,
    .rangeIntrMask = 0UL,
    .configRouting = false,
    .vrefMvValue = pass_0_sar_0_VREF_MV,
    .clock = CY_SAR_CLK_PERI,
    .fifoCfgPtr = NULL,
    .trTimer = false,
    .scanCnt = false,
    .scanCntIntr = false,
};

#if defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE)
const cyhal_resource_inst_t pass_0_sar_0_obj =
{
    .type = CYHAL_RSC_ADC,
    .block_num = 0,
    .channel_num = 0,
};
#endif /* defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE) */

#if defined(CY_USING_HAL_LITE) || defined (CY_USING_HAL)
const cyhal_clock_t pass_0_sar_0_clock =
{
    .block = CYHAL_CLOCK_BLOCK_PERIPHERAL_16BIT,
    .channel = 0,
#if defined (CY_USING_HAL)
    .reserved = false,
    .funcs = NULL,
#endif /* defined (CY_USING_HAL) */
};
#endif /* defined(CY_USING_HAL_LITE) || defined (CY_USING_HAL) */

#if defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE)
const cyhal_adc_configurator_t pass_0_sar_0_hal_config =
{
    .resource = &pass_0_sar_0_obj,
    .config = &pass_0_sar_0_config,
    .clock = &pass_0_sar_0_clock,
    .num_channels = 4,
    .achieved_acquisition_time = NULL,
};
#endif /* defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE) */

const cy_stc_scb_uart_config_t scb_5_config =
{
    .uartMode = CY_SCB_UART_STANDARD,
    .enableMutliProcessorMode = false,
    .smartCardRetryOnNack = false,
    .irdaInvertRx = false,
    .irdaEnableLowPowerReceiver = false,
    .oversample = 8,
    .enableMsbFirst = false,
    .dataWidth = 8UL,
    .parity = CY_SCB_UART_PARITY_NONE,
    .stopBits = CY_SCB_UART_STOP_BITS_1,
    .enableInputFilter = false,
    .breakWidth = 11UL,
    .dropOnFrameError = false,
    .dropOnParityError = false,
    .receiverAddress = 0x0UL,
    .receiverAddressMask = 0x0UL,
    .acceptAddrInFifo = false,
    .enableCts = false,
    .ctsPolarity = CY_SCB_UART_ACTIVE_LOW,
    .rtsRxFifoLevel = 0UL,
    .rtsPolarity = CY_SCB_UART_ACTIVE_LOW,
    .rxFifoTriggerLevel = 63UL,
    .rxFifoIntEnableMask = 0UL,
    .txFifoTriggerLevel = 63UL,
    .txFifoIntEnableMask = 0UL,
};

#if defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE)
const cyhal_resource_inst_t scb_5_obj =
{
    .type = CYHAL_RSC_SCB,
    .block_num = 5U,
    .channel_num = 0U,
};
#endif /* defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE) */

#if defined(CY_USING_HAL_LITE) || defined (CY_USING_HAL)
const cyhal_clock_t scb_5_clock =
{
    .block = CYHAL_CLOCK_BLOCK_PERIPHERAL_24_5BIT,
    .channel = 0,
#if defined (CY_USING_HAL)
    .reserved = false,
    .funcs = NULL,
#endif /* defined (CY_USING_HAL) */
};
#endif /* defined(CY_USING_HAL_LITE) || defined (CY_USING_HAL) */

#if defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE)
const cyhal_uart_configurator_t scb_5_hal_config =
{
    .resource = &scb_5_obj,
    .config = &scb_5_config,
    .clock = &scb_5_clock,
#if defined (CY_USING_HAL)
    .gpios = {.pin_tx = P5_1, .pin_rts = NC, .pin_cts = NC},
#endif /* defined (CY_USING_HAL) */
};
#endif /* defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE) */

void init_cycfg_peripherals(void)
{
    SAR_MUX_SWITCH0(pass_0_sar_0_HW) |= CY_SAR_MUX_FW_VSSA_VMINUS;
    SAR_MUX_SWITCH_SQ_CTRL(pass_0_sar_0_HW) |= CY_SAR_MUX_SQ_CTRL_VSSA;
    Cy_SysClk_PeriphAssignDivider(PCLK_PASS_CLOCK_SAR, CY_SYSCLK_DIV_16_BIT, 0U);
    Cy_SysClk_PeriphAssignDivider(PCLK_SCB5_CLOCK, CY_SYSCLK_DIV_24_5_BIT, 0U);
}
void reserve_cycfg_peripherals(void)
{
#if defined (CY_USING_HAL)
    cyhal_hwmgr_reserve(&CYBSP_BLE_obj);
    cyhal_hwmgr_reserve(&pass_0_sar_0_obj);
    cyhal_hwmgr_reserve(&scb_5_obj);
#endif /* defined (CY_USING_HAL) */
}
