/**
 * @file  hal_uart.c
 *
 * @brief Implementation of UART functions.
 *
 * @note QN9080
 * 
 * @version 0.5.0
 */

#include "fsl_device_registers.h"
#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_usart.h"

#include "hal.h"
#include "hal_uart.h"

///////////////////////
// Defines
///////////////////////
#define RX_BUF_LEN  16u

///////////////////////
// Functions prototype
///////////////////////
static uint32_t _getUsartClock(void);
static inline usart_stop_bit_count_t _getNumStopBits(uint8_t numStopBits);
static void USART_ISR(void);

///////////////////////
// Private data
///////////////////////
static HAL_UART_rxCallback_t rxCallback;

///////////////////////
// Code
///////////////////////
/**
 * @brief  Initialize UART
 * 
 * @details Configrure & enable UART; Setup RX callback
 * 
 * @param[in] config - UART settings
 * 
 * @param[in]  callback - UART RX callback
 * 
 * @note    1. UART -> Flexcomm0
 *          2. At start up Vector Table is allocated in Flash only (see SystemInit func).
 *             VTOR should be changed to map into RAM (see Address map in qn908x.ld).
 *             Also MXP's OSA Layer's func OSA_InstallIntHandler() relocates VTOR in case 
 *             it wasn't done previously.
 *             VTOR shall be relocated explicitly in HAL_init_hardware().
 *             See InstallIRQHandler() in fsl_common.c for reference.
 *             __ram_vector_table__ shall be provided to linker script (--defsym=__ram_vector_table__=1).
 *          3. NXP SDK for QN908x contains the module UART_Adapter.c as part of manufacturer's framework.
 *             The module has exaples: \ref USART_Initialize(), USART_ISR().
 * 
 * @attention   'numStartBits' option isn't supported by QN908x port.
 *              Also 'enParityCheck' is RFU: for now Parity check is disabled by default.
 * 
 * @retval 0 - success, otherwise - fail status
 */
HAL_UART_status_t HAL_UART_init(HAL_UART_config_t *config, HAL_UART_rxCallback_t callback)
{
    HAL_ASSERT(config != NULL);
    HAL_ASSERT((config->baudRate >= 115200u) && (config->baudRate <= 921600u));
    HAL_ASSERT(config->numStopBits <= 2u);

    USART_Type *base = USART0;
    usart_config_t configPer;

    /** RX callback */
    rxCallback = callback;

    /** Enables the clock for the UART (Flexcomm0 module) */
    CLOCK_EnableClock(kCLOCK_Flexcomm0);

    /** Init UART peripheral with specified settings */
    USART_GetDefaultConfig(&configPer);
    configPer.baudRate_Bps = config->baudRate;
    configPer.stopBitCount = _getNumStopBits(config->numStopBits);
    configPer.txWatermark = kUSART_TxFifo0; // N/A because TX_IRQ isn't used 
    configPer.rxWatermark = kUSART_RxFifo1; // IRQ when data byte is received (FIFO is no longer empty) 
    configPer.enableRx = 1u;
    configPer.enableTx = 1u;
    USART_Init(base, &configPer, _getUsartClock());

    /** Config UART IRQ for receiving data & Enable */
    USART_EnableInterrupts(base, kUSART_RxLevelInterruptEnable);
    InstallIRQHandler(FLEXCOMM0_IRQn,  (uint32_t) USART_ISR);
    // TODO: define interrupts configuration including priority in separate Header file
    // that based on RTOS kernel aware/unaware interrupts concept 
    NVIC_SetPriority(FLEXCOMM0_IRQn, 1u);
    NVIC_EnableIRQ(FLEXCOMM0_IRQn);

    return HAL_UART_SUCCESS;
}

/**
 * @brief  Check if UART TX FIFO is ready to receive data
 * 
 * @param  None
 * 
 * @retval true - UART is ready, false - not ready
 */
bool HAL_UART_isTxReady(void)
{
    if ((kUSART_TxFifoNotFullFlag) & USART_GetStatusFlags(USART0)) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief  Transmit Raw data via UART
 * 
 * @param[in]   data - data buffer
 * 
 * @param[in]   len  - length of data
 * 
 * @retval None 
 */
void HAL_UART_txData(uint8_t *data, uint16_t len)
{
    USART_WriteBlocking(USART0, data, len);
}

/**
 * @brief  Check if UART transmission is ongoing
 * 
 * @details Checks TX FIFO and transmit shift register status
 * 
 * @param  None
 * 
 * @retval true - TX is ongoing, false - no active transmission
 */
bool HAL_UART_isTxActive(void)
{
    if (((kUSART_TxFifoEmptyFlag) & USART_GetStatusFlags(USART0)) && 
         (USART0->STAT & USART_STAT_TXIDLE_MASK))
    {
        return false;
    } else {
        return true;
    }
}

/////////////////////////////////////
/// ISR code
/////////////////////////////////////
static void USART_ISR(void)
{
    uint32_t fifoStatus;
    USART_Type *base = USART0;
    uint8_t rxBuf[RX_BUF_LEN];
    uint8_t i = 0;

    /** Check if data was received */
    if ((kUSART_RxFifoNotEmptyFlag)& USART_GetStatusFlags(base)) {
        uint8_t cnt = 0;
        do {
            HAL_ASSERT(cnt < RX_BUF_LEN);
            rxBuf[i++] = USART_ReadByte(base);
            cnt++;
            __DSB();
            __ISB();
            fifoStatus = USART_GetStatusFlags(base);
            __DSB();
            __ISB();
        } while(kUSART_RxFifoNotEmptyFlag & fifoStatus);
        rxCallback(rxBuf, cnt);
    }

    /** Check errors */
    if (kUSART_RxError & USART_GetStatusFlags(base)) {
        USART_ClearStatusFlags(base, kUSART_RxError);
    }
}

////////////////////////////////////
/// Auxiliary functions
////////////////////////////////////
/**
 * @brief  Get UART clock frequency in [Hz].
 * 
 * @param  None
 * 
 * @retval Frequency [Hz].
 */
static uint32_t _getUsartClock(void)
{
    return CLOCK_GetFreq(kCLOCK_CoreSysClk);
}

/**
 * @brief  Get UART stop bits configuration.
 * 
 * @param[in] numStopBits -  Requested number of stop bits: 1, 2.
 * 
 * @retval Stop bits config for UART periph.
 */
static inline usart_stop_bit_count_t _getNumStopBits(uint8_t numStopBits)
{
    return (usart_stop_bit_count_t) (numStopBits - 1u);
}

/* [] END OF FILE */
