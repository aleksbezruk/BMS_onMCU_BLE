/**
 * @file qspyHelper.c
 * 
 * @brief Implements helper functions for QSPY & QUtest framework. 
 *        Also includes API for Application.
 * 
 * @version 0.1.0
 */

#include "cy_pdl.h"
#include "BSP.h"
#include "qspyHelper.h"
#include "qutestHelper.h"

#include "FreeRTOS.h"
#include "task.h"

#include "MAIN.h"
#include "BLE.h"

///////////////////
// Defines
///////////////////
#define QS_TICKS_IN_1MS (SystemCoreClock / BSP_TICKS_PER_SEC)

/** Code undet test */
typedef void (*cut)(void);

///////////////////
// Private data
///////////////////
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

static QSTimeCtr QS_tickTime_;       /**< ms */
static QSTimeCtr QS_tickPeriod_;     /**< ms */

///////////////////
// Private functions
///////////////////
static void QS_initTimer_(void) 
{
    (void)SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);
    NVIC_SetPriority(SysTick_IRQn, 1U);
}

void SysTick_Handler(void) 
{
    volatile uint32_t tmp;

    tmp = SysTick->CTRL; // clear CTRL_COUNTFLAG
    (void) tmp;
    QS_tickTime_ += QS_tickPeriod_; // account for the clock rollover

    // RTOS tick
    portDISABLE_INTERRUPTS();
    {
        /* Increment the RTOS tick. */
        if( xTaskIncrementTick() != pdFALSE )
        {
            /* A context switch is required.  Context switching is performed in
             * the PendSV interrupt.  Pend the PendSV interrupt. */
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
    }
    portENABLE_INTERRUPTS();
}

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

////////////////////////////
// Helper functions
////////////////////////////
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
    return QS_tickTime_;
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
            while (BSP_isUartTxReady() == false) { // while TX FIFO is full -> wait
            }
            BSP_uartTxData((uint8_t *)&b, 1U) ; // put into the DR register
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
                BSP_led_red_On();
                break;
            case 0:
                BSP_led_red_Off();
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
        case QS_CMD_BLE_START_ADV:
        {
            Ble_evt_t evt;
            evt.advData.periodicAdvIntMin = param1;
            evt.advData.periodicAdvIntMax = param2;
            evt.advData.periodicAdvProp = param3;
            BLE_post_evt(&evt, EVT_BLE_ADV_ON);
            break;
        }
        case QS_CMD_BLE_STOP_ADV:
        {
            Ble_evt_t evt;
            BLE_post_evt(&evt, EVT_BLE_ADV_OFF);
            break;
        }
        default:
            break;  // just igmore if cmd isn't defined
    }
}

///////////////////
// Public API 
///////////////////
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

    QUTEST_init();

    /** Configure QSPY timer */
    QS_initTimer_();

    /** Configure QSPY TX, RX buffers */
    QS_initBuf(qsTxBuf, sizeof(qsTxBuf));
    QS_rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

    /** Configure QSPY link layer -> UART */
    status = BSP_initUart(QS_rxCallback);
    /** Config dictionaries */
    QS_FUN_DICTIONARY(QS_onReset);

    QUTEST_EMUL_ERR(status);
    if (status != bsp_status_init_success) {
        QUTEST_printError((uint32_t) status);
        status = QSPY_STATUS_ERROR;
    }

    /** Configure QSPY tick */
    // QS_tickPeriod_ = SystemCoreClock / BSP_TICKS_PER_SEC;
    QS_tickPeriod_ = 1;
    QS_tickTime_ = 0; // to start the timestamp at zero

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

    if (BSP_isUartTxReady() == true ) {
        __disable_irq();
        uint16_t b = QS_getByte();
        __enable_irq();

        if (b != QS_EOD) {  // not End-Of-Data?
            BSP_uartTxData((uint8_t *)&b, 1U);
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
            QS_USR_DICTIONARY(MAIN);
            break;
        case UTEST:
            QS_USR_DICTIONARY(UTEST);
            break;
        case BSP:
            QS_USR_DICTIONARY(BSP);
            break;
        case ADC:
            QS_USR_DICTIONARY(ADC);
            break;
        case BLE_TRACE:
            QS_USR_DICTIONARY(BLE_TRACE);
            break;
        case BLE_BAS:
            QS_USR_DICTIONARY(BLE_BAS);
            break;
        default:
            break;
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

/******************************** END OF FILE **********************************************************************/
