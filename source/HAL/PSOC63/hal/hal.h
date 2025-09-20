/**
 * @file  hal.h
 * @brief Definition of generic HAL functions like init board/hardware, generic utils
 * @note PSOC63
 * @version 0.6.0
 */

#ifndef HAL_GENERIC_MODULE_H
#define HAL_GENERIC_MODULE_H

// QSPY includes
#include "qspyHelper.h"

// =======================
// Defines
// =======================
/**
 * @brief Number of system ticks per second used for timing operations.
 * 
 * This macro defines the frequency of the system tick timer, which is used for
 * timekeeping and delay functions throughout the HAL.
 */
#define HAL_TICKS_PER_SEC 1000U

/*! Define HAL generic status type */
typedef enum {
    HAL_STATUS_OK,
    HAL_STATUS_FAIL,
    HAL_STATUS_UNKNOWN
} HAL_status_t;

#define HAL_ASSERT_HANDLER()   __disable_irq(); while(true) {}

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

// =======================
// API
// =======================
HAL_status_t HAL_init_hardware(void);

#endif /* HAL_GENERIC_MODULE_H */

/* [] END OF FILE */
