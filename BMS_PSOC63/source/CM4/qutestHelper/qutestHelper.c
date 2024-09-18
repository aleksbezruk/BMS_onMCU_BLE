/**
 * @file qutestHelper.c
 * @brief Implements helper functions for QUtest. 
 * 
 * @version 0.1.0
 */

#include "qspyHelper.h"

/*************************************
 * Data
 *************************************/
uint32_t errCode    __attribute__((section(".noinit")));
uint32_t errCodeStatus  __attribute__((section(".noinit")));

/*************************************
 * Code
 *************************************/
void QS_onTestEvt(QEvt * e) {
    // TODO: implement
}

void QS_onTestPost(
    void const * sender,
    QActive * recipient,
    QEvt const * e,
    bool status)
{
    // TODO: implement
}

void QS_onTestTeardown(void)
{
    // TODO: implement
}

void QS_onTestSetup(void)
{
    // TODO: implement
}

void QUTEST_runUTfunc(void(*func)(void))
{
    __disable_irq();
    func();
    __enable_irq();

    QS_BEGIN_ID(UTEST, 0U) // app-specific record
        QS_FUN((void(*)(void)) func); // function called
    QS_END()
    QS_FLUSH();
}

void QUTEST_readMcuReg(uint32_t regAddr)
{
    uint32_t *mcuReg = (uint32_t *) regAddr;
    uint32_t mcuRegVal = *mcuReg;
    QS_BEGIN_ID(UTEST, 0U)              // app-specific record
        QS_STR("READ_MCU_REG");         // operation info
        QS_U32(0, (uint32_t) mcuReg);   // reg addr to read
        QS_U32(0, mcuRegVal);           // reg value
    QS_END()
    QS_FLUSH();
}

void QUTEST_writeMcuReg(uint32_t regAddr, uint32_t value)
{
    uint32_t *mcuReg = (uint32_t *) regAddr;
    *mcuReg = value;
    QS_BEGIN_ID(UTEST, 0U)              // app-specific record
        QS_STR("WRITE_MCU_REG");         // operation info
        QS_U32(0, (uint32_t) mcuReg);   // reg addr to write
        QS_U32(0, value);               // reg value
    QS_END()
    QS_FLUSH();
}

void QUTEST_printError(uint32_t errCode)
{
    QS_BEGIN_ID(UTEST, 0U)              // app-specific record
        QS_STR("ERR_CODE");             // operation info
        QS_U32(0, errCode);             // err code
    QS_END()
    QS_FLUSH();
}

void QUTEST_injectError(uint32_t err)
{
    errCode = err;

    QS_BEGIN_ID(UTEST, 0U)              // app-specific record
        QS_STR("INJ_ERR_CODE");         // operation info
        QS_U32(0, err);                 // err code
    QS_END()
    QS_FLUSH();
}

void QUTEST_init(void)
{
    if (errCodeStatus != 0xaa55aa55) {
        errCodeStatus = 0xaa55aa55;
        errCode = 0xaa55aa55;
    }
}

/* [] END OF FILE */
