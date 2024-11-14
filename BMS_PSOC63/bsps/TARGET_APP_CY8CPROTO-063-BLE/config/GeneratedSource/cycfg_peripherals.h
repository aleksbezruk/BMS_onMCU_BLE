/*******************************************************************************
 * File Name: cycfg_peripherals.h
 *
 * Description:
 * Peripheral Hardware Block configuration
 * This file was automatically generated and should not be modified.
 * Configurator Backend 3.20.0
 * device-db 4.16.0.6098
 * mtb-pdl-cat1 3.11.0.34443
 *
 *******************************************************************************
 * Copyright 2024 Cypress Semiconductor Corporation (an Infineon company) or
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

#if !defined(CYCFG_PERIPHERALS_H)
#define CYCFG_PERIPHERALS_H

#include "cycfg_notices.h"
#include "cy_sysanalog.h"
#include "cy_sar.h"
#include "cycfg_routing.h"
#include "cy_sysclk.h"
#include "cy_scb_uart.h"

#if defined (CY_USING_HAL)
#include "cyhal_hwmgr.h"
#include "cyhal.h"
#endif /* defined (CY_USING_HAL) */

#if defined (CY_USING_HAL_LITE)
#include "cyhal_hw_types.h"
#endif /* defined (CY_USING_HAL_LITE) */

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

#define CYBSP_BLE_ENABLED 1U
#define CY_BLE_CORE_CORTEX_M4 4U
#define CY_BLE_CORE_CORTEX_M0P 0U
#define CY_BLE_CORE_DUAL 255U
#ifndef CY_BLE_CORE
    #define CY_BLE_CORE 4U
#endif
#define CY_BLE_IRQ bless_interrupt_IRQn
#define pass_0_aref_0_ENABLED 1U
#define pass_0_sar_0_ENABLED 1U
#define pass_0_sar_0_HW SAR
#define pass_0_sar_0_IRQ pass_interrupt_sar_IRQn
#define pass_0_sar_0_CTL ((uint32_t)CY_SAR_VREF_PWR_100 | (uint32_t)CY_SAR_VREF_SEL_VDDA | (uint32_t)CY_SAR_BYPASS_CAP_ENABLE | (uint32_t)CY_SAR_NEG_SEL_VSSA_KELVIN | (uint32_t)CY_SAR_CTRL_NEGVREF_HW | (uint32_t)CY_SAR_CTRL_COMP_DLY_12 | (uint32_t)CY_SAR_COMP_PWR_100 | (uint32_t)CY_SAR_DEEPSLEEP_SARMUX_OFF | (uint32_t)CY_SAR_SARSEQ_SWITCH_ENABLE)
#define pass_0_sar_0_SAMPLE ((uint32_t)SAR_SAMPLE_CTRL_EOS_DSI_OUT_EN_Msk | (uint32_t)CY_SAR_RIGHT_ALIGN | (uint32_t)CY_SAR_DIFFERENTIAL_UNSIGNED | (uint32_t)CY_SAR_SINGLE_ENDED_SIGNED | (uint32_t)CY_SAR_AVG_CNT_64 | (uint32_t)CY_SAR_AVG_MODE_SEQUENTIAL_FIXED | (uint32_t)CY_SAR_TRIGGER_MODE_FW_ONLY)
#define pass_0_sar_0_CH0_CONFIG (((uint32_t)SAR0_VPLUS0_PORT << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos) | (uint32_t)(SAR0_VPLUS0_PIN << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos) | CY_SAR_CHAN_SINGLE_ENDED | CY_SAR_CHAN_AVG_ENABLE | (uint32_t)CY_SAR_CHAN_SAMPLE_TIME_0)
#define pass_0_sar_0_CH1_CONFIG (((uint32_t)SAR0_VPLUS1_PORT << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos) | (uint32_t)(SAR0_VPLUS1_PIN << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos) | CY_SAR_CHAN_SINGLE_ENDED | CY_SAR_CHAN_AVG_ENABLE | (uint32_t)CY_SAR_CHAN_SAMPLE_TIME_1)
#define pass_0_sar_0_CH2_CONFIG (((uint32_t)SAR0_VPLUS2_PORT << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos) | (uint32_t)(SAR0_VPLUS2_PIN << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos) | CY_SAR_CHAN_SINGLE_ENDED | CY_SAR_CHAN_AVG_ENABLE | (uint32_t)CY_SAR_CHAN_SAMPLE_TIME_1)
#define pass_0_sar_0_CH3_CONFIG (((uint32_t)SAR0_VPLUS3_PORT << SAR_CHAN_CONFIG_POS_PORT_ADDR_Pos) | (uint32_t)(SAR0_VPLUS3_PIN << SAR_CHAN_CONFIG_POS_PIN_ADDR_Pos) | CY_SAR_CHAN_SINGLE_ENDED | CY_SAR_CHAN_AVG_ENABLE | (uint32_t)CY_SAR_CHAN_SAMPLE_TIME_1)
#define pass_0_sar_0_VREF_MV 2500UL
#define scb_5_ENABLED 1U
#define scb_5_HW SCB5
#define scb_5_IRQ scb_5_interrupt_IRQn

#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_BLE_obj;
#endif /* defined (CY_USING_HAL) */

extern const cy_stc_sysanalog_config_t pass_0_aref_0_config;
extern const cy_stc_sar_config_t pass_0_sar_0_config;

#if defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE)
extern const cyhal_resource_inst_t pass_0_sar_0_obj;
#endif /* defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE) */

#if defined(CY_USING_HAL_LITE) || defined (CY_USING_HAL)
extern const cyhal_clock_t pass_0_sar_0_clock;
#endif /* defined(CY_USING_HAL_LITE) || defined (CY_USING_HAL) */

#if defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE)
extern const cyhal_adc_configurator_t pass_0_sar_0_hal_config;
#endif /* defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE) */

extern const cy_stc_scb_uart_config_t scb_5_config;

#if defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE)
extern const cyhal_resource_inst_t scb_5_obj;
#endif /* defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE) */

#if defined(CY_USING_HAL_LITE) || defined (CY_USING_HAL)
extern const cyhal_clock_t scb_5_clock;
#endif /* defined(CY_USING_HAL_LITE) || defined (CY_USING_HAL) */

#if defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE)
extern const cyhal_uart_configurator_t scb_5_hal_config;
#endif /* defined (CY_USING_HAL) || defined(CY_USING_HAL_LITE) */

void init_cycfg_peripherals(void);
void reserve_cycfg_peripherals(void);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* CYCFG_PERIPHERALS_H */
