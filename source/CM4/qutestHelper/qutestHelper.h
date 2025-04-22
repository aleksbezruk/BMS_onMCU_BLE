/**
 * @file  qutestHelper.h
 * @brief qutestHelper functions definitions
 * 
 * @version 0.4.0
 */

#ifndef QUTESTHELPER_MODULE_H
#define QUTESTHELPER_MODULE_H

#include <stdint.h>

#if defined(Q_UTEST)
extern uint32_t errCode    __attribute__((section(".noinit")));
#define QUTEST_EMUL_ERR(var)         \
    if (errCode != 0xaa55aa55) {     \
        var = errCode;               \
        errCode = 0xaa55aa55;        \
    }
#else
#define QUTEST_EMUL_ERR(var)
#define QUTEST_printError(errCode)
#define QUTEST_init()
#endif //Q_UTEST

#endif // QUTESTHELPER_MODULE_H

/* [] END OF FILE */
