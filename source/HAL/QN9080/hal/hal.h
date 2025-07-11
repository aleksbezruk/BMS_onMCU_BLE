/**
 * @file  hal.h
 *
 * @brief Definition of generic HAL functions like init board/hardware, generic utils
 *
 * @note QN9080
 * 
 * @version 0.5.0
 */

#ifndef HAL_GENERIC_MODULE_H
#define HAL_GENERIC_MODULE_H

#include <stdbool.h>
#include "QN908XC.h"

#include "qspyHelper.h"

/* =========================
 * Defines
 * ========================= */
#define HAL_TICKS_PER_SEC 1000U

/*! Define HAL generic status type */
typedef enum {
    HAL_STATUS_OK,
    HAL_STATUS_FAIL,
    HAL_STATUS_UNKNOWN
} HAL_status_t;

#define HAL_ASSERT_HANDLER()   while(true) {}

/*! Assert an argument is true, else call assert handler */
/**
 * @brief Assert macro that checks a condition and calls the assert handler if false.
 * 
 * Usage: HAL_ASSERT(expr, __FILE__, __LINE__);
 * Note: Always use a trailing semicolon after this macro.
 */
#define HAL_ASSERT(condition, file, line)    do {    \
    if(!(condition))                                 \
    {                                                \
        /* QSPY record assert event */               \
        QS_BEGIN_ID(HAL, 0)                          \
            QS_STR("HAL assert failed: ");           \
            QS_STR((file));                          \
            QS_U32(0, (line));                       \
        QS_END()                                     \
        QS_FLUSH();                                  \
        /* Call assert handler */                    \
        HAL_ASSERT_HANDLER();                        \
    }                                                \
} while (false) /* Always use a trailing semicolon after this macro */

/* =========================
 * API
 * ========================= */
HAL_status_t HAL_init_hardware(void);

#endif /* HAL_GENERIC_MODULE_H */

/* [] END OF FILE */
