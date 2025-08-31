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

#ifndef _QN_BLE_CONFIG_H_
#define _QN_BLE_CONFIG_H_

/*! @brief GAP role: CFG_CENTRAL, CFG_PERIPHERAL, CFG_OBSERVER, CFG_BROADCASTER, CFG_ALLROLES */
#if !( defined(CFG_CENTRAL) || defined(CFG_PERIPHERAL) || defined(CFG_OBSERVER) || defined(CFG_BROADCASTER) )
#define CFG_ALLROLES
#endif

/*! @brief Maximum connected devices number (Link number). It's up to 16.
    Select the appropriate number for your application and get optimized memory usage. */
#define CFG_CON_DEV_MAX (gAppMaxConnections_c)

/*! @brief Maximum scanned devices number */
#ifndef CFG_SCAN_DEV_MAX
#define CFG_SCAN_DEV_MAX (20)
#endif

/*! @brief Maximum bonded devices number */
#define CFG_BOND_DEV_MAX (gMaxBondedDevices_c)

/*! @brief Number of devices in the white list */
#ifndef CFG_BLE_WHITELIST_MAX
#define CFG_BLE_WHITELIST_MAX (26) /* (CFG_CON_DEV_MAX + 2) */
#endif

/*! @brief Number of devices in the Resolution Address List */
#define CFG_BLE_RESOL_ADDR_LIST_MAX (gMaxResolvingListSize_c)

/*! @brief Number of devices capacity for the scan filtering */
#ifndef CFG_BLE_DUPLICATE_FILTER_MAX
#define CFG_BLE_DUPLICATE_FILTER_MAX (10)
#endif

/*! @brief TX buffer size */
#define CFG_REG_BLE_EM_TX_BUFFER_DATA_SIZE (260)

/*! @brief RX buffer size */
#define CFG_REG_BLE_EM_RX_BUFFER_SIZE (260)

/*! @brief EA programming latency for only 1 activity */
#define CFG_BLE_PROG_LATENCY_DFT (2)

/*! @brief Minimum number of connection event for instant calculation (6 according to the SIG) */
#define CFG_MIN_INSTANT_CON_EVT (32)

/*! @brief Device Appearance */
#define CFG_DEV_APPEARANCE (0x0000)

/*! @brief Duration before regenerate device address, unit(second) */
#define CFG_ADDR_RENEW_DUR (150)

/*! @brief Maximum device MTU size (23 ~ ) */
#define CFG_MAX_MTU (2048)
/*! @brief Maximum device MPS size (23 ~ QN_MAX_MTU) */
#define CFG_MAX_MPS (23)

/*! @briefSuggested value for the Controller's maximum transmitted number of payload octets to be used (27 ~ 251) */
#define CFG_SUGG_MAX_TX_SIZE (251)
/*! @briefSuggested value for the Controller's maximum packet transmission time to be used */
#define CFG_SUGG_MAX_TX_TIME (CFG_SUGG_MAX_TX_SIZE * 8 + 112)

/*! @brief Advertising channel map */
#define CFG_ADV_CHMAP (ADV_ALL_CHNLS_EN)
/*! @brief Advertising minimum interval */
#define CFG_ADV_INT_MIN (0x1E0)
/*! @brief Advertising maximum interval */
#define CFG_ADV_INT_MAX (0x1E0)
/*! @brief Non-connectable advertising minimum interval */
#define CFG_ADV_NONCONN_INT_MIN (0xA0)
/*! @brief Non-connectable advertising maximum interval */
#define CFG_ADV_NONCONN_INT_MAX (0xA0)

/*! @brief Advertising data excludes Flags and Local Name
    Flags will be added by GAP layer and Local Name will be tried to add in APP_ConstructAdvData() */
#define CFG_ADV_DATA ""
#define CFG_ADV_DATA_LEN (0)

/*! @brief Scan response data */
/*                                                    Length
 *                                                        Incomplete list of 16-bit Service UUIDs
 *                                                            GAP service
 *                                                                    GATT service
 */
#define CFG_SCAN_RSP_DATA "\x05\x02\x00\x18\x01\x18"
#define CFG_SCAN_RSP_DATA_LEN (6)

/*! @brief Scan interval, Value Time = N * 0.625 ms */
#define CFG_SCAN_INTV (GAP_INQ_SCAN_INTV)
/*! @brief Scan window, Value Time = N * 0.625 ms */
#define CFG_SCAN_WIND (GAP_INQ_SCAN_WIND)

/*! @brief Minimum connection interval, Value Time = N * 1.25 ms */
#define CFG_CONN_MIN_INTV (0x00F0) // 300ms interval
/*! @brief Maximum connection interval, Value Time = N * 1.25 ms */
#define CFG_CONN_MAX_INTV (0x00F0) // 300ms interval
/*! @brief Connection latency */
#define CFG_CONN_LATENCY (GAP_CONN_LATENCY)
/*! @brief Connection supervision timeout */
#define CFG_CONN_SUPERV_TIMEOUT (GAP_CONN_SUPERV_TIMEOUT)

/*! @brief Scan interval in any discovery or connection establishment, Value Time = N * 0.625 ms */
#define CFG_SCAN_FAST_INTV (GAP_SCAN_FAST_INTV)
/*! @brief Scan window in any discovery or connection establishment, Value Time = N * 0.625 ms */
#define CFG_SCAN_FAST_WIND (GAP_SCAN_FAST_WIND)

/*! @brief Minimum connection event */
#define CFG_CONN_MIN_CE (GAP_CONN_MIN_CE)
/*! @brief Maximum connection event */
#define CFG_CONN_MAX_CE (GAP_CONN_MAX_CE)

/*! @brief Device security requirements (minimum security level)
    The pairing procedure will use this parameter to check whether key distribution and security mode match the
   requirement */
#define CFG_SEC_MODE_LEVEL (GAP_SEC1_NOAUTH_PAIR_ENC)

/*! @brief IO capability for pairing */
#define CFG_IO_CAPABILITY (GAP_IO_CAP_DISPLAY_YES_NO)

/*! @brief Authentication requirements for pairing */
#define CFG_AUTH_REQ (GAP_AUTH_REQ_NO_MITM_BOND)

/*! @brief Allow the Link Layer to use the ECDH implementation from SecLib.
    Note that if NVDS is not programmed on chip, enabling this macro will 
    increase the startup time as well as the the power consumption
    each time the chip is powered on or restarted. Generating a new secret key 
    using the ECDH algorithm takes about 2 seconds and for bare-metal projects
    it is a blocking operation. If NVDS is programmed, the key will be generated 
    only once and stored on NVDS for subsequent usage */
#define CFG_USE_SECLIB_ECDH_IMPL    FALSE

#endif /* _QN_BLE_CONFIG_H_ */
