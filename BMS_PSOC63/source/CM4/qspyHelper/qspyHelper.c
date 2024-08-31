/**
 * @file qspyHelper.c
 * @brief Implements helper functions for QSPY & QUtest framework. 
 *        Also includes API for Application.
 * 
 * @version 0.1.0
 */

#include "cy_pdl.h"
#include "BSP.h"
#include "qspyHelper.h"

/*****************************
 * Defines
 *****************************/
#define QS_TICKS_IN_1MS (SystemCoreClock / BSP_TICKS_PER_SEC)
typedef void (*cut)(void);

/*******************************/
/*** External data */
/******************************/
extern volatile uint32_t mainLoopTimer;

/*******************************/
/*** Private data */
/******************************/
static uint8_t qsTxBuf[2048]; // buffer for QS-TX channel
static uint8_t qsRxBuf[100];    // buffer for QS-RX channel
#if defined(Q_UTEST)
static uint8_t qsTestFuncArgs[50];  // buffer for functions args received from UT scripts
#endif //Q_UTEST

static QSTimeCtr QS_tickTime_;      // ms
static QSTimeCtr QS_tickPeriod_;    // ms

/*******************************/
/*** Private functions */
/******************************/
static void QS_initTimer_(void) {
    SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);
    NVIC_SetPriority(SysTick_IRQn, 1U);
}

void SysTick_Handler(void) {
    volatile uint32_t tmp;

    mainLoopTimer++;

    tmp = SysTick->CTRL; // clear CTRL_COUNTFLAG
    (void) tmp;
    QS_tickTime_ += QS_tickPeriod_; // account for the clock rollover
}

void QS_rxCallback_(uint8_t *data, uint16_t len) {
    uint16_t i;

    for(i = 0; i < len; i++) {
        QS_RX_PUT(data[i]);
    }
}

/*******************************/
/*** Helper functions */
/******************************/
void QS_onReset(void) {
    NVIC_SystemReset();
}

#if !defined(Q_UTEST)
QSTimeCtr QS_onGetTime(void) {
    return QS_tickTime_;
}
#endif //Q_UTEST

void QS_onFlush(void) {
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

void QS_onCommand(uint8_t cmdId,
                  uint32_t param1, 
                  uint32_t param2, 
                  uint32_t param3) 
{
    switch (cmdId)
    {
        case QS_CMD_RED_LED:
        {
            if(param1 == 1) {
                BSP_led_red_On();
            } else if(param1 == 0) {
                BSP_led_red_Off();
            } else {
                // ignore if invalid param
            }
            break;
        }
        case QS_CMD_UT_FUN:
        {
            // call the Code Under Test (CUT)
            void(*fp)(void) = (void(*)(void)) param1;
            (*fp)();
            QS_BEGIN_ID(UTEST, 0U) // app-specific record
                QS_FUN((void(*)(void)) param1); // function called
            QS_END()
            QS_FLUSH();
            break;
        }
        case QS_CMD_MCU_READ_REG:
        {
            uint32_t *mcuReg = (uint32_t *) param1;
            uint32_t mcuRegVal = *mcuReg;
            QS_BEGIN_ID(UTEST, 0U)              // app-specific record
                QS_STR("READ_MCU_REG");         // operation info
                QS_U32(0, (uint32_t) mcuReg);   // reg addr to read
                QS_U32(0, mcuRegVal);           // reg value
            QS_END()
            QS_FLUSH();
            break;
        }
        case QS_CMD_MCU_WRITE_REG:
        {
            uint32_t *mcuReg = (uint32_t *) param1;
            uint32_t mcuRegVal = param2;
            *mcuReg = mcuRegVal;
            QS_BEGIN_ID(UTEST, 0U)              // app-specific record
                QS_STR("WRITE_MCU_REG");         // operation info
                QS_U32(0, (uint32_t) mcuReg);   // reg addr to write
                QS_U32(0, mcuRegVal);           // reg value
            QS_END()
            QS_FLUSH();
            break;
        }
        default:
            break;  // just igmore if cmd isn't defined
    }
}

#if !defined(Q_UTEST)
Q_NORETURN Q_onError(char const * const module, int_t const id) {
    // NOTE: this implementation of the assertion handler is intended only
    // for debugging and MUST be changed for deployment of the application
    // (assuming that you ship your production code with assertions enabled).
    Q_UNUSED_PAR(module);
    Q_UNUSED_PAR(id);
    QS_ASSERTION(module, id, 10000U);

#ifndef NDEBUG
    // for debugging, hang on in an endless loop...
    for (;;) {
    }
#endif

    NVIC_SystemReset();
}
#endif  //Q_UTEST

void QS_onCleanup(void) 
{
    // just stub
}

/*******************************/
/*** Public API */
/******************************/
uint8_t QS_onStartup(void const *arg) {
    Q_UNUSED_PAR(arg);

    /** Configure QSPY timer */
    QS_initTimer_();

    /** Configure QSPY TX, RX buffers */
    QS_initBuf(qsTxBuf, sizeof(qsTxBuf));
    QS_rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

    /** Configure QSPY link layer -> UART */
    BSP_initUart(QS_rxCallback_);

    /** Configure QSPY tick */
    // QS_tickPeriod_ = SystemCoreClock / BSP_TICKS_PER_SEC;
    QS_tickPeriod_ = 1;
    QS_tickTime_ = 0; // to start the timestamp at zero

#if defined(Q_UTEST)
    QS_OBJ_DICTIONARY(qsTestFuncArgs);
#endif //Q_UTEST

    return QSPY_STATUS_SUCCESS;
}

void QS_onIdle(void) {
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

void QS_addUsrRecToDic(enum_t const rec) {
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
        default:
            for(;;) {}
    }
}

void QS_initGlbFilters(void) {
    QS_GLB_FILTER(QS_ALL_RECORDS);   // enable all records
}

/******************************** END OF FILE **********************************************************************/
