/**
 * @file qspyHelper.c
 * 
 * @brief Implements helper functions for QSPY & QUtest framework. 
 *        Also includes API for Application.
 * 
 * @version 0.4.0
 */

#include "qspyHelper.h"
#include "qutestHelper.h"

#ifndef BMS_DISABLE_RTOS
#include "MAIN.h"
#else
#include "MAIN.h"
#include "hal_gpio.h"
// Stub, test Discharge switch only for QN9080DK
void MAIN_post_evt(Main_evt_t* evt, Evt_types_t eventType)
{
    (void) eventType;
    static bool dischSwInit = false;

    if (evt->sysEvtData.swStates == 0x01) {
        if (!dischSwInit) {
            dischSwInit = true;
            HAL_GPIO_init_pin(
                HAL_DISCHARGE_PORT,
                HAL_DISCHARGE_PIN,
                HAL_GPIO_DIGITAL_OUTPUT,
                HAL_GPIO_PULL_DISABLED,
                HAL_GPIO_DRIVE_HIGH,
                HAL_DISCHARGE_OFF
            );
        }
        // Set discharge switch ON
        HAL_GPIO_set_pin(
            HAL_DISCHARGE_PORT,
            HAL_DISCHARGE_PIN,
            HAL_DISCHARGE_ON
        );
    } else {
        // Set discharge switch OFF
        HAL_GPIO_set_pin(
            HAL_DISCHARGE_PORT,
            HAL_DISCHARGE_PIN,
            HAL_DISCHARGE_OFF
        );
    }
}
#endif  // BMS_DISABLE_RTOS

#ifndef BMS_DISABLE_BLE
#include "BLE.h"
#endif  // BMS_DISABLE_BLE

// HAL
#include "hal.h"
#include "hal_led.h"
#include "hal_uart.h"
#include "hal_time.h"

// ===================
// Defines
// ===================
/*! Code under test */
typedef void (*cut)(void);

// ===================
// Private data
// ===================
static uint8_t qsTxBuf[2048];   /**< buffer for QS-TX channel */
static uint8_t qsRxBuf[100];     /**<  buffer for QS-RX channel */
#if defined(Q_UTEST)
//static uint8_t qsTestFuncArgs[50];  // buffer for functions args received from UT scripts
void QUTEST_runUTfunc(void(*func)(void));
void QUTEST_readMcuReg(uint32_t regAddr);
void QUTEST_writeMcuReg(uint32_t regAddr, uint32_t value);
void __gcov_dump(void); /* internal gcov function to write data */
void QUTEST_printError(uint32_t errCode);
void QUTEST_injectError(uint32_t err);
void QUTEST_init(void);
#endif //Q_UTEST

// ===================
// Code
// ===================
/**
 * @brief Callback for received data via UART
 * 
 * @param[in] data pointer to buffer that stores incoming UART data
 * 
 * @param[in] len  amount of received UART data
 * 
 * @retval None
 */
void QS_rxCallback(uint8_t *data, uint16_t len) 
{
    uint16_t i;

    for(i = 0; i < len; i++) {
        QS_RX_PUT(data[i]);
    }
}

/**
 * @brief Resets MCU
 * 
 * @param None
 * 
 * @retval None
 */
void QS_onReset(void) 
{
    QS_TEST_PROBE_DEF(&QS_onReset)
    QS_TEST_PROBE(
        QS_FLUSH();
        __disable_irq();
        __gcov_dump();  // dump memory before reset
        __enable_irq();
    )

    NVIC_SystemReset();
}

#if !defined(Q_UTEST)
/**
 * @brief Helper function for QSPY & QUtest franework to get time tick
 * 
 * @param None
 * 
 * @retval Tick time in ms
 */
QSTimeCtr QS_onGetTime(void)
{
    __disable_irq();
    QSTimeCtr ts = hal_time_get(); // get current time in ms
    __enable_irq();

    return ts;
}
#endif //Q_UTEST

/**
 * @brief Sends data collected in QSPY buffer to Host PC
 * 
 * @note Blocking call
 * 
 * @param None
 * 
 * @retval None
 */
void QS_onFlush(void) 
{
    for (;;) {
        uint16_t b = QS_getByte();
        if (b != QS_EOD) {
            while (HAL_UART_isTxReady() == false) { // while TX FIFO is full -> wait
            }
            HAL_UART_txData((uint8_t *)&b, 1U) ; // put into the DR register
        }
        else {
            break;
        }
    }
}

/**
 * @brief Handles a cmd received from Host PC
 * 
 * @param[in] cmdId - command ID
 * 
 * @param[in] param1 - 1st cmd's parameter
 * 
 * @param[in] param2 - 2nd cmd's parameter
 * 
 * @param[in] param3 - 3rd cmd's parameter
 * 
 * @retval None
 */
void QS_onCommand(uint8_t cmdId,
                  uint32_t param1, 
                  uint32_t param2, 
                  uint32_t param3) 
{
    switch (cmdId)
    {
        case QS_CMD_RED_LED:
        {
            switch (param1)
            {
            case 1:
                HAL_LED_red_On();
                break;
            case 0:
                HAL_LED_red_Off();
                break;
            default:
                break;
            }
            break;
        }
#if defined(Q_UTEST)
        case QS_CMD_UT_FUN:
        {
            // call the Code Under Test (CUT)
            void(*fp)(void) = (void(*)(void)) param1;
            QUTEST_runUTfunc(fp);
            break;
        }
        case QS_CMD_MCU_READ_REG:
        {
            QUTEST_readMcuReg(param1);
            break;
        }
        case QS_CMD_MCU_WRITE_REG:
        {
            QUTEST_writeMcuReg(param1, param2);
            break;
        }
        case QS_CMD_INJECT_ERROR:
        {
            QUTEST_injectError(param1);
            break;
        }
#endif //Q_UTEST
        case QS_CMD_BMS_SW_STATES:
        {
            Evt_sys_data_t evt;
            evt.swStates = param1;
            MAIN_post_evt((Main_evt_t*) &evt, EVT_SYSTEM);
            break;
        }
#ifndef BMS_DISABLE_BLE
        case QS_CMD_BLE_START_ADV:
        {
            Ble_evt_t evt;
            evt.advParam.periodicAdvIntMin = param1;
            evt.advParam.periodicAdvIntMax = param2;
            evt.advParam.periodicAdvProp = param3;
            BLE_post_evt(&evt, EVT_BLE_ADV_ON);
            break;
        }
        case QS_CMD_BLE_STOP_ADV:
        {
            Ble_evt_t evt;
            BLE_post_evt(&evt, EVT_BLE_ADV_OFF);
            break;
        }
#endif  // BMS_DISABLE_BLE
        default:
        {
            break;  // just igmore if cmd isn't defined
        }
    }
}

// ====================
// Public API
// ====================

/**
 * @brief Init QSPY
 * 
 * @param[in] arg arguments
 * 
 * @retval QSPY_STATUS_SUCCESS - on success, QSPY_STATUS_ERROR - on error
 */
uint8_t QS_onStartup(void const *arg) 
{
    Q_UNUSED_PAR(arg);
    uint8_t status = QSPY_STATUS_SUCCESS;

    /** Initialize QUTEST */
    QUTEST_init();

    /** Configure QSPY TX, RX buffers */
    QS_initBuf(qsTxBuf, sizeof(qsTxBuf));
    QS_rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

    /** Configure QSPY link layer -> UART */
    HAL_UART_config_t uartConfig = {
        .baudRate = 115200u,
        .enParityCheck = false,
        .numStartBits = 1u,
        .numStopBits = 1u
    };
    if (HAL_UART_init(&uartConfig, QS_rxCallback) != HAL_UART_SUCCESS) {
        status = QSPY_STATUS_ERROR;
    }

    /** Config dictionaries */
    QS_FUN_DICTIONARY(QS_onReset);

    /** Check error Emulation used for testing purposes */
    QUTEST_EMUL_ERR(status);
    if (status != QSPY_STATUS_SUCCESS) {
        QUTEST_printError((uint32_t) status);
    }

    return status;
}

/**
 * @brief Sends data to Host with running QSPY. Application calls the function from Idle task
 * 
 * @param None
 * 
 * @retval None
 */
void QS_onIdle(void) 
{
    QS_rxParse();  // parse all the received bytes

    if (HAL_UART_isTxReady() == true ) {
        __disable_irq();
        uint16_t b = QS_getByte();
        __enable_irq();

        if (b != QS_EOD) {  // not End-Of-Data?
            HAL_UART_txData((uint8_t *)&b, 1U);
        }
    }
}

/**
 * @brief Add user/app recrods group to dictionary
 * 
 * @param[in] rec user record  \ref AppRecords
 * 
 * @retval None
 */
void QS_addUsrRecToDic(enum_t const rec) 
{
    switch(rec) {
        case MAIN:
        {
            QS_USR_DICTIONARY(MAIN);
            break;
        }
        case UTEST:
        {
            QS_USR_DICTIONARY(UTEST);
            break;
        }
        case BSP:
        {
            QS_USR_DICTIONARY(BSP);
            break;
        }
        case ADC_RCD:
        {
            QS_USR_DICTIONARY(ADC_RCD);
            break;
        }
        case BLE_TRACE:
        {
            // BLE trace records
            QS_USR_DICTIONARY(BLE_TRACE);
            break;
        }
        case BLE_BAS:
        {
            QS_USR_DICTIONARY(BLE_BAS);
            break;
        }
        case BLE_AIOS:
        {
            QS_USR_DICTIONARY(BLE_AIOS);
            break;
        }
        case HAL:
        {
            // HAL records
            QS_USR_DICTIONARY(HAL);
            break;
        }
        default:
        {
            // do nothing if record is not defined
            break;
        }
    }
}

/**
 * @brief Init global filters
 * 
 * @param None
 * 
 * @retval None
 */
void QS_initGlbFilters(void) 
{
    QS_GLB_FILTER(QS_ALL_RECORDS);   // enable all records
}

/**
 * @brief Get QSPY recive buffer status
 * 
 * @param None
 * 
 * @retval \ref QSPY_rx_status_t
 */
QSPY_rx_status_t QS_get_rxStatus(void)
{
    uint16_t nFree = QS_rxGetNfree();
    if (nFree == (sizeof(qsRxBuf) - 1U)) {
        return QSPY_RX_EMPTY;
    } else {
        return QSPY_RX_NOT_EMPTY;
    }
}

/**
 * @brief Get QSPY transmitter buffer status
 *
 * @param None
 *
 * @retval \ref QSPY_tx_status_t
 */
QSPY_tx_status_t QS_get_txStatus(void)
{
    uint16_t numBytes = QS_getTxBufNumBytes();
    bool isOngoingTx = HAL_UART_isTxActive();
    if ((numBytes > 0) || (isOngoingTx == true)) {  // not End-Of-Data or TX shift register/FIFO not empty
        return QSPY_TX_NOT_EMPTY;
    } else {
        return QSPY_TX_EMPTY;
    }
}

/* [] END OF FILE */
