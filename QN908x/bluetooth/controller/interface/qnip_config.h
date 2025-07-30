/*! *********************************************************************************
 * \defgroup CONTROLLER - Controller Interface
 * @{
 ********************************************************************************** */
/*! *********************************************************************************
* Copyright (c) 2014, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
* 
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#ifndef _QNIP_CONFIG_H_
#define _QNIP_CONFIG_H_

/* Don't change following configuration */

#if defined(__ROM_FW)

/// System configuration: CFG_QN908XA, CFG_QN908XB
#if (!defined(CFG_QN908XA))
#define CFG_QN908XB
#endif

/// BLE configuration
#define CFG_BLE
#define CFG_EMB
//#define CFG_HOST
#define CFG_PRF
#define CFG_FW
#define CFG_AHITL
#define CFG_HCITL
#define CFG_ATTC
#define CFG_ATTS
#define CFG_EXT_DB
#define CFG_SEC_CON
#define CFG_SLEEP
#define CFG_CHNL_ASSESS
#define CFG_WLAN_COEX
#define CFG_NVDS
//#define CFG_PLF_DBG
#define CFG_DBG
//#define CFG_DBG_MEM
//#define CFG_DBG_FLASH
//#define CFG_DBG_NVDS
//#define CFG_DBG_STACK_PROF
#define CFG_HW_AUDIO
#define CFG_BLE_2MBPS

#else

/// System configuration: CFG_QN908XA, CFG_QN908XB
#if (!defined(CFG_QN908XA))
#define CFG_QN908XB
#endif

/// BLE configuration
#define CFG_BLE
#define CFG_EMB
//#define CFG_HOST // GGG
//#define CFG_PRF // GGG
//#define CFG_APP // GGG
//#define CFG_AHITL // GGG
#define CFG_HCITL
//#define CFG_ATTC // GGG
//#define CFG_ATTS // GGG
//#define CFG_EXT_DB
#define CFG_SEC_CON
#define CFG_SLEEP
//#define CFG_CHNL_ASSESS // GGG
//#define CFG_WLAN_COEX // GGG
#define CFG_NVDS
//#define CFG_PLF_DBG // GGG
//#define CFG_DBG // GGG
//#define CFG_DBG_MEM // GGG
//#define CFG_DBG_FLASH
//#define CFG_DBG_NVDS // GGG
//#define CFG_DBG_STACK_PROF // GGG
//#define CFG_HW_AUDIO
//#define CFG_BLE_2MBPS // GGG

#define CFG_SIM
//#define CFG_SIM_ADV
//#define CFG_SIM_SCAN
//#define CFG_SIM_INIT
//#define CFG_SIM_DTM_TX
//#define CFG_SIM_DTM_RX
//#define CFG_SIM_LE_2MBPS
//#define CFG_LE_DATA_PACKET_LEN_EXT

#endif

#endif

