/**
 * @file qutestHelper.h
 * @brief qutestHelper definitions
 * 
 * @version 0.1.0
 */

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

/* [] END OF FILE */
