/*! *********************************************************************************
 * \defgroup CONTROLLER - Controller Interface
 * @{
 ********************************************************************************** */
/*! *********************************************************************************
* Copyright (c) 2014, Freescale Semiconductor, Inc.
* Copyright 2016-2017, 2022 NXP
* All rights reserved.
* 
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#define NXP_BLE_STACK 1
#define __ROM_FW 1
#define CFG_WM_HCI

#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
// #include "controller_task_config.h"
// #include "ApplMain.h"  // File doesn't exist, commented out

#include "RNG_Interface.h"
#include <stdbool.h>

#include "qn_ble_config.h"
#include "qnip_config.h"
#include "rwip_config.h"
#include "compiler.h"
#include "co_list.h"   
#include "co_bt.h"
#include "co_llcp.h"
#include "co_hci.h"
#include "em_buf.h"
#include "ke_msg.h"
#include "ke_task.h"
#include "rwip_task.h"
#include "rwip.h"
#include "llc_task.h"
#include "lld_evt.h"
#include "llc.h"
#include "rwble_config.h"
#include "em_map.h"
#include "em_map_ble.h"
#include "llm_task.h"
#include "dbg_task.h"

extern uint8_t RNG_HwGetRandomNo(uint32_t* pRandomNo);
extern void EIF_Read(uint8_t *bufptr, uint32_t size, rwip_eif_callback callback, void *dummy);
extern void EIF_Write(uint8_t *bufptr, uint32_t size, rwip_eif_callback callback, void *dummy);
extern void EIF_FlowOn(void);
extern bool EIF_FlowOff(void);
extern void EIF_RxCallback(void);
extern void EIF_TxCallback(void);
extern void APP_PlfReset(uint32_t err);

uint32_t fw_min_instant_con_evt = CFG_MIN_INSTANT_CON_EVT;
bool_t gCtrlUseSecLibEcdh = CFG_USE_SECLIB_ECDH_IMPL;

const uint8_t user_rwip_coex_cfg[RWIP_COEX_CFG_MAX]=
{
    [RWIP_COEX_CON_IDX]     = (uint8_t)((RWIP_PTI_TXDIS << RWIP_TXBSY_POS) | (RWIP_PTI_RXDIS << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
    [RWIP_COEX_CON_DATA_IDX]= (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS)  | (RWIP_PTI_RXEN << RWIP_RXBSY_POS)  | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
    [RWIP_COEX_ADV_IDX]     = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS)  | (RWIP_PTI_RXDIS << RWIP_RXBSY_POS) | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
    [RWIP_COEX_SCAN_IDX]    = (uint8_t)((RWIP_PTI_TXDIS << RWIP_TXBSY_POS) | (RWIP_PTI_RXEN << RWIP_RXBSY_POS)  | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
    [RWIP_COEX_INIT_IDX]    = (uint8_t)((RWIP_PTI_TXEN << RWIP_TXBSY_POS)  | (RWIP_PTI_RXEN << RWIP_RXBSY_POS)  | (RWIP_PTI_DNABORTDIS << RWIP_DNABORT_POS)),
};

void BLE_EnableWlanCoex(void)
{
    rwip_coex_cfg = user_rwip_coex_cfg;
}

/*! @brief Exchange memory definitions */
#define CFG_EM_BLE_CS_COUNT (BLE_CONNECTION_MAX + 1)
#define CFG_EM_BLE_TX_DESC_COUNT (CFG_BLE_TX_DESC_CNT)
#define CFG_EM_BLE_RX_DESC_COUNT (CFG_BLE_RX_BUFFER_CNT)
#define CFG_EM_BLE_TX_BUFF_CNTL_COUNT (CFG_BLE_TX_BUFF_CNTL_CNT)
#define CFG_EM_BLE_TX_BUFF_DATA_COUNT (CFG_BLE_TX_BUFF_DATA_CNT)

                                            /*! @brief Offset of the public white list area */
#define APP_EM_BLE_WPB_OFFSET               (EM_BLE_CS_OFFSET + CFG_EM_BLE_CS_COUNT * REG_BLE_EM_CS_SIZE)
                                            /*! @brief Offset of the private white list area */
#define APP_EM_BLE_WPV_OFFSET               (APP_EM_BLE_WPB_OFFSET + CFG_BLE_WHITELIST_MAX * REG_BLE_EM_WPB_SIZE)
                                            /*! @brief Offset of the private white list area */
#define APP_EM_BLE_RAL_OFFSET               (APP_EM_BLE_WPV_OFFSET + CFG_BLE_WHITELIST_MAX * REG_BLE_EM_WPV_SIZE)
                                            /*! @brief Offset of the TX descriptor area */
#define APP_EM_BLE_TX_DESC_OFFSET           (APP_EM_BLE_RAL_OFFSET + CFG_BLE_RESOL_ADDR_LIST_MAX * REG_BLE_EM_RAL_SIZE)
                                            /*! @brief Offset of the RX descriptor area */
#define APP_EM_BLE_RX_DESC_OFFSET           (APP_EM_BLE_TX_DESC_OFFSET + CFG_EM_BLE_TX_DESC_COUNT * REG_BLE_EM_TX_DESC_SIZE)
                                            /*! @brief Offset of the TX buffer area */
#define APP_EM_BLE_TX_BUFFER_CNTL_OFFSET    (APP_EM_BLE_RX_DESC_OFFSET + CFG_EM_BLE_RX_DESC_COUNT * REG_BLE_EM_RX_DESC_SIZE)
                                            /*! @brief Offset of the TX buffer area */
#define APP_EM_BLE_TX_BUFFER_DATA_OFFSET    (APP_EM_BLE_TX_BUFFER_CNTL_OFFSET + (CFG_EM_BLE_TX_BUFF_CNTL_COUNT + EM_BLE_TX_BUFF_ADV_COUNT) * REG_BLE_EM_TX_BUFFER_CNTL_SIZE)
                                            /*! @brief Offset of the RX buffer area */
#define APP_EM_BLE_RX_BUFFER_OFFSET         (APP_EM_BLE_TX_BUFFER_DATA_OFFSET + CFG_EM_BLE_TX_BUFF_DATA_COUNT * CFG_REG_BLE_EM_TX_BUFFER_DATA_SIZE)
                                            /*! @brief End of BLE EM */
#define APP_EM_BLE_END                      (APP_EM_BLE_RX_BUFFER_OFFSET + CFG_BLE_RX_BUFFER_CNT * CFG_REG_BLE_EM_RX_BUFFER_SIZE)


/*! @brief Exchange memory */
uint32_t rwip_exchange_memory[RWIP_MEM_ALIGN(APP_EM_BLE_END)];
/*! @brief Unloaded area */
struct unloaded_area_tag unloaded_area;
/*! @brief Heap definitions - use uint32 to ensure that memory blocks are 32bits aligned. */
uint32_t rwip_heap_env[RWIP_CALC_HEAP_LEN(RWIP_HEAP_ENV_SIZE)];         /* Memory allocated for environment variables */
uint32_t rwip_heap_db[RWIP_CALC_HEAP_LEN(RWIP_HEAP_DB_SIZE)];           /* Memory allocated for Attribute database */
uint32_t rwip_heap_msg[RWIP_CALC_HEAP_LEN(RWIP_HEAP_MSG_SIZE)];         /* Memory allocated for kernel messages */
uint32_t rwip_heap_non_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_NON_RET_SIZE)]; /* Non Retention memory block */

ke_state_t l2cc_state[BLE_CONNECTION_MAX];  /* Defines the place holder for the states of all the task instances. */
ke_state_t llc_state[BLE_CONNECTION_MAX];   /* Defines the place holder for the states of all the task instances. */

const struct ke_task_desc ble_task_desc[] = {
        [TASK_LLM] = {NULL, &llm_default_handler, llm_state, LLM_STATE_MAX, LLM_IDX_MAX},
#if (BLE_PERIPHERAL || BLE_CENTRAL)
        [TASK_LLC] = {NULL, &llc_default_handler, llc_state, LLC_STATE_MAX, BLE_CONNECTION_MAX},
#endif
        [TASK_LLD] = {NULL, NULL, NULL, 1, 1},
        [TASK_DBG] = {NULL, &dbg_default_handler, dbg_state, DBG_STATE_MAX, DBG_IDX_MAX},
#if (BLE_APP_PRESENT)
        [TASK_APP] = {NULL, &app_default_handler, app_state, APP_STATE_MAX, APP_IDX_MAX},
        [TASK_AHI] = {NULL, &ahi_default_handler, ahi_state, AHI_STATE_MAX, AHI_IDX_MAX},
  #if (BLE_PERIPHERAL || BLE_CENTRAL)
        [TASK_L2CC] = {NULL, &l2cc_default_handler, l2cc_state, L2CC_STATE_MAX, BLE_CONNECTION_MAX},
        [TASK_GATTM] = {NULL, &gattm_default_handler, gattm_state, GATTM_STATE_MAX, GATTM_IDX_MAX},
        [TASK_GATTC] = {NULL, &gattc_default_handler, gattc_state, GATTC_STATE_MAX, BLE_CONNECTION_MAX},
  #endif
        [TASK_GAPM] = {NULL, &gapm_default_handler, gapm_state, GAPM_STATE_MAX, GAPM_IDX_MAX},
  #if (BLE_PERIPHERAL || BLE_CENTRAL)
        [TASK_GAPC] = {NULL, &gapc_default_handler, gapc_state, GAPC_STATE_MAX, BLE_CONNECTION_MAX},
  #endif
#endif /* BLE_APP_PRESENT */
};

struct app_static_cfg app_configuration = {
    .unloaded_area          = &unloaded_area, 
    .plf_reset_cb           = APP_PlfReset,
    .hci_user_ext_func      = NULL,
    .hci_common_callback    = hci_vs_command_callback_handler,  //Vendor Specific commands handler
    .rwip_heap_env          = rwip_heap_env,
    .rwip_heap_db           = rwip_heap_db,
    .rwip_heap_msg          = rwip_heap_msg,
    .rwip_heap_non_ret      = rwip_heap_non_ret,
    .rwip_heap_env_size     = RWIP_HEAP_ENV_SIZE,
    .rwip_heap_db_size      = RWIP_HEAP_DB_SIZE,
    .rwip_heap_msg_size     = RWIP_HEAP_MSG_SIZE,
    .rwip_heap_non_ret_size = RWIP_HEAP_NON_RET_SIZE,
#if (BLE_PROFILES)
    .prf_init               = prf_init,
    .prf_cleanup            = prf_cleanup,
    .prf_create             = prf_create,
    .prf_get_id_from_task   = prf_get_id_from_task,
    .prf_get_task_from_id   = prf_get_task_from_id,
    .prf_add_profile        = prf_add_profile,
    .prf_env_get            = prf_env_get,
#else
    .prf_init               = NULL,
    .prf_cleanup            = NULL,
    .prf_create             = NULL,
    .prf_get_id_from_task   = NULL,
    .prf_get_task_from_id   = NULL,
    .prf_add_profile        = NULL,
    .prf_env_get            = NULL,
#endif
    {/* struct rwip_eif_api */
        .read               = EIF_Read,
        .write              = EIF_Write,
        .flow_on            = EIF_FlowOn,
        .flow_off           = EIF_FlowOff,
    },
    .rf_api                 = NULL,
    .ke_task_max            = CFG_TASK_MAX,
    .ble_task_desc          = ble_task_desc,
#if defined(CFG_WM_NP)
    .app_main_task          = TASK_AHI,
#else
    .app_main_task          = TASK_APP,
#endif
    .ble_con_max            = BLE_CONNECTION_MAX,
    .prog_latency_def       = CFG_BLE_PROG_LATENCY_DFT,
    .bg_sleep_duration      = MAX_SLEEP_DURATION_EXTERNAL_WAKEUP, //MAX_SLEEP_DURATION_PERIODIC_WAKEUP
};

const struct fw_static_cfg fw_configuration = {
    .ble_whitelist_max          = CFG_BLE_WHITELIST_MAX,
    .ble_resol_addr_list_max    = CFG_BLE_RESOL_ADDR_LIST_MAX,
    .ble_duplicate_filter_max   = CFG_BLE_DUPLICATE_FILTER_MAX,
    .gapm_scan_filter_size      = 0,
    .gap_max_le_mtu             = 0,
    .gap_tmr_lim_adv_timeout    = 0,
    .att_trans_rtx              = 0,
#if (BLE_PERIPHERAL || BLE_CENTRAL)
    .smpc_rep_att_timer_def_val = 0,
    .smpc_rep_att_timer_max_val = 0,
    .smpc_rep_att_timer_mult    = 0,
    .smpc_timout_timer_duration = 0,
    .app_pkt_statistic          = NULL,
#endif
};

const struct em_offset_cfg em_offset_config = {
    .em_ble_wpb_offset = APP_EM_BLE_WPB_OFFSET,
    .em_ble_wpv_offset = APP_EM_BLE_WPV_OFFSET,
    .em_ble_ral_offset = APP_EM_BLE_RAL_OFFSET,
    .em_ble_tx_desc_offset = APP_EM_BLE_TX_DESC_OFFSET,
    .em_ble_rx_desc_offset = APP_EM_BLE_RX_DESC_OFFSET,
    .em_ble_tx_buffer_ctrl_offset = APP_EM_BLE_TX_BUFFER_CNTL_OFFSET,
    .em_ble_tx_buffer_data_offset = APP_EM_BLE_TX_BUFFER_DATA_OFFSET,
    .em_ble_rx_buffer_offset = APP_EM_BLE_RX_BUFFER_OFFSET,
    .em_ble_tx_buf_data_cnt = CFG_BLE_TX_BUFF_DATA_CNT,
    .em_ble_rx_buffer_size = CFG_REG_BLE_EM_RX_BUFFER_SIZE,
    .em_ble_rx_buffer_cnt = CFG_BLE_RX_BUFFER_CNT,
};

/*! @brief Initialize BLE configuration. */
void NXP_InitBleCfg(struct ble_config_st *cfg)
{
    cfg->fw = (struct fw_static_cfg *)&fw_configuration;
    cfg->app = (struct app_static_cfg *)&app_configuration;
    cfg->em = (struct em_offset_cfg *)&em_offset_config;

    /* Flag */
    cfg->flags = 0;
    
#if (defined(BOARD_XTAL1_CLK_HZ) && (BOARD_XTAL1_CLK_HZ == CLK_XTAL_32KHZ))
    cfg->sw_32k_calib_enable = false;
#else
    cfg->sw_32k_calib_enable = true;
#endif

    cfg->ppm_32k = -0x6000;

    CFG_API_FLAG_SET(FAST_CORRECT);

#if defined(CFG_WM_HCI)
    CFG_API_FLAG_SET(HCI);
    CFG_API_FLAG_RESET(ACI);
#elif defined(CFG_WM_NP)
    CFG_API_FLAG_RESET(HCI);
    CFG_API_FLAG_SET(ACI);
#else
    CFG_API_FLAG_RESET(HCI);
    CFG_API_FLAG_RESET(ACI);
#endif

    /* Interrupt mask in GLOBAL_INT_DISABLE() */
    cfg->int_mask0 = 0xFFFFFFFF;
    cfg->int_mask1 = 0xFFFFFFFF;
    
    RNG_Init();
    RNG_HwGetRandomNo(&(cfg->fw_seed));
}
