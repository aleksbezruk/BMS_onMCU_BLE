/*! *********************************************************************************
* Copyright 2013-2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */


#ifndef _AES_REG_ACCESS_H_
#define _AES_REG_ACCESS_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "compiler.h"
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



#define BLE_CRYPTINTSTAT_BIT         ((uint32_t)0x00000010)
#define BLE_CRYPTINTSTAT_POS         4

// field definitions
#define BLE_AES_MODE_BIT     ((uint32_t)0x00000002)
#define BLE_AES_MODE_POS     1
#define BLE_AES_START_BIT    ((uint32_t)0x00000001)
#define BLE_AES_START_POS    0


#define BLE_AESPTR_ADDR   0x200200D4
#define BLE_AESCNTL_ADDR   0x200200C0


/// Macro to write a BLE register
#define REG_BLE_WR(addr, value)      (*(volatile uint32_t *)(addr)) = (value)
/// Macro to read a BLE register
#define REG_BLE_RD(addr)             (*(volatile uint32_t *)(addr))




/// Write bytes to EM
__FORCEINLINE void em_wr(void const *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy((void *)(em_addr + EM_BASE_ADDR), sys_addr, len);
}

/// Read bytes from EM
__FORCEINLINE void em_rd(void *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy(sys_addr, (void *)(em_addr + EM_BASE_ADDR), len);
}


/**
 * @brief INTSTAT register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     12        AUDIOINT2STAT   0
 *     11        AUDIOINT1STAT   0
 *     10        AUDIOINT0STAT   0
 *     09            SWINTSTAT   0
 *     08     EVENTAPFAINTSTAT   0
 *     07     FINETGTIMINTSTAT   0
 *     06    GROSSTGTIMINTSTAT   0
 *     05         ERRORINTSTAT   0
 *     04         CRYPTINTSTAT   0
 *     03         EVENTINTSTAT   0
 *     02           SLPINTSTAT   0
 *     01            RXINTSTAT   0
 *     00         CSCNTINTSTAT   0
 * </pre>
 */
#define BLE_INTSTAT_ADDR   0x20020010
#define BLE_INTSTAT_OFFSET 0x00000010
#define BLE_INTSTAT_INDEX  0x00000004
#define BLE_INTSTAT_RESET  0x00000000

__FORCEINLINE uint32_t ble_intstat_get(void)
{
    return REG_BLE_RD(BLE_INTSTAT_ADDR);
}

/**
 * @brief AESPTR register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00               AESPTR   0x0
 * </pre>
 */
#define BLE_AESPTR_ADDR   0x200200D4
#define BLE_AESPTR_OFFSET 0x000000D4
#define BLE_AESPTR_INDEX  0x00000035
#define BLE_AESPTR_RESET  0x00000000
__FORCEINLINE void ble_aesptr_set(uint32_t value)
{
    REG_BLE_WR(BLE_AESPTR_ADDR, value);
}

/**
 * @brief AESCNTL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     01             AES_MODE   0
 *     00            AES_START   0
 * </pre>
 */
#define BLE_AESCNTL_ADDR   0x200200C0
#define BLE_AESCNTL_OFFSET 0x000000C0
#define BLE_AESCNTL_INDEX  0x00000030
#define BLE_AESCNTL_RESET  0x00000000

__FORCEINLINE void ble_aescntl_set(uint32_t value)
{
    REG_BLE_WR(BLE_AESCNTL_ADDR, value);
}



/**
 * @brief INTACK register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     12         AUDIOINT2ACK   0
 *     11         AUDIOINT1ACK   0
 *     10         AUDIOINT0ACK   0
 *     09             SWINTACK   0
 *     08      EVENTAPFAINTACK   0
 *     07      FINETGTIMINTACK   0
 *     06     GROSSTGTIMINTACK   0
 *     05          ERRORINTACK   0
 *     04          CRYPTINTACK   0
 *     03          EVENTINTACK   0
 *     02            SLPINTACK   0
 *     01             RXINTACK   0
 *     00          CSCNTINTACK   0
 * </pre>
 */
#define BLE_INTACK_ADDR   0x20020018
#define BLE_INTACK_OFFSET 0x00000018
#define BLE_INTACK_INDEX  0x00000006
#define BLE_INTACK_RESET  0x00000000
__FORCEINLINE void ble_intack_clear(uint32_t value)
{
    REG_BLE_WR(BLE_INTACK_ADDR, value);
}



/**
// * @brief AESKEY31_0 register definition
// * <pre>
// *   Bits           Field Name   Reset Value
// *  -----   ------------------   -----------
// *  31:00           AESKEY31_0   0x0
// * </pre>
// */
#define BLE_AESKEY31_0_ADDR   0x200200C4
#define BLE_AESKEY31_0_OFFSET 0x000000C4
#define BLE_AESKEY31_0_INDEX  0x00000031
#define BLE_AESKEY31_0_RESET  0x00000000
__FORCEINLINE uint32_t ble_aeskey31_0_get(void)
{
    return REG_BLE_RD(BLE_AESKEY31_0_ADDR);
}

__FORCEINLINE void ble_aeskey31_0_set(uint32_t value)
{
    REG_BLE_WR(BLE_AESKEY31_0_ADDR, value);
}

/**
 * @brief AESKEY63_32 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00          AESKEY63_32   0x0
 * </pre>
 */
#define BLE_AESKEY63_32_ADDR   0x200200C8
#define BLE_AESKEY63_32_OFFSET 0x000000C8
#define BLE_AESKEY63_32_INDEX  0x00000032
#define BLE_AESKEY63_32_RESET  0x00000000

__FORCEINLINE uint32_t ble_aeskey63_32_get(void)
{
    return REG_BLE_RD(BLE_AESKEY63_32_ADDR);
}

__FORCEINLINE void ble_aeskey63_32_set(uint32_t value)
{
    REG_BLE_WR(BLE_AESKEY63_32_ADDR, value);
}

/**
 * @brief AESKEY95_64 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00          AESKEY95_64   0x0
 * </pre>
 */
#define BLE_AESKEY95_64_ADDR   0x200200CC
#define BLE_AESKEY95_64_OFFSET 0x000000CC
#define BLE_AESKEY95_64_INDEX  0x00000033
#define BLE_AESKEY95_64_RESET  0x00000000

__FORCEINLINE uint32_t ble_aeskey95_64_get(void)
{
    return REG_BLE_RD(BLE_AESKEY95_64_ADDR);
}

__FORCEINLINE void ble_aeskey95_64_set(uint32_t value)
{
    REG_BLE_WR(BLE_AESKEY95_64_ADDR, value);
}


/**
 * @brief AESKEY127_96 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00         AESKEY127_96   0x0
 * </pre>
 */
#define BLE_AESKEY127_96_ADDR   0x200200D0
#define BLE_AESKEY127_96_OFFSET 0x000000D0
#define BLE_AESKEY127_96_INDEX  0x00000034
#define BLE_AESKEY127_96_RESET  0x00000000

__FORCEINLINE uint32_t ble_aeskey127_96_get(void)
{
    return REG_BLE_RD(BLE_AESKEY127_96_ADDR);
}

__FORCEINLINE void ble_aeskey127_96_set(uint32_t value)
{
    REG_BLE_WR(BLE_AESKEY127_96_ADDR, value);
}

/**
 * @brief DEEPSLCNTL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     31           EXTWKUPDSB   0
 *     15      DEEP_SLEEP_STAT   0
 *     04      SOFT_WAKEUP_REQ   0
 *     03   DEEP_SLEEP_CORR_EN   0
 *     02        DEEP_SLEEP_ON   0
 *     01       RADIO_SLEEP_EN   0
 *     00         OSC_SLEEP_EN   0
 * </pre>
 */
#define BLE_DEEPSLCNTL_ADDR   0x20020030
#define BLE_DEEPSLCNTL_OFFSET 0x00000030
#define BLE_DEEPSLCNTL_INDEX  0x0000000C
#define BLE_DEEPSLCNTL_RESET  0x00000000

__FORCEINLINE void ble_soft_wakeup_req_setf(uint8_t softwakeupreq)
{
    REG_BLE_WR(BLE_DEEPSLCNTL_ADDR, (REG_BLE_RD(BLE_DEEPSLCNTL_ADDR) & ~((uint32_t)0x00000010)) | ((uint32_t)softwakeupreq << 4));
}

/**
 ****************************************************************************************
 * @brief Read a packed 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @return The 16 bits value.
 ****************************************************************************************
 */
__FORCEINLINE uint16_t co_read16p(void const *ptr16)
{
    uint16_t value = ((uint8_t *)ptr16)[0] | ((uint8_t *)ptr16)[1] << 8;
    return value;
}
/**
 ****************************************************************************************
 * @brief Read a packed 32 bits word.
 * @param[in] ptr32 The address of the first byte of the 32 bits word.
 * @return The 32 bits value.
 ****************************************************************************************
 */
__FORCEINLINE uint32_t co_read32p(void const *ptr32)
{
    uint16_t addr_l, addr_h;
    addr_l = co_read16p((uint16_t *)ptr32);
    addr_h = co_read16p((uint16_t *)ptr32 + 1);
    return ((uint32_t)addr_l | (uint32_t)addr_h << 16);
}


#endif