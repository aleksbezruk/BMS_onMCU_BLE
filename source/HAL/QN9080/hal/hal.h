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
#define HAL_ASSERT(x)    do {           \
    if(!(x))                            \
    {                                   \
        HAL_ASSERT_HANDLER();           \
    }                                   \
} while (false)

/* =========================
 * API
 * ========================= */
HAL_status_t HAL_init_hardware(void);

#endif /* HAL_GENERIC_MODULE_H */

/* [] END OF FILE */
