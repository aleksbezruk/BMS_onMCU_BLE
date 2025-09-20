/**
 * @file hal_time.h
 * @brief HAL time management functions for QN9080.     
 * @note QN9080 specific implementation details.
 * @version 0.6.0
 */

#ifndef HAL_TIME_H
#define HAL_TIME_H

#include <stdint.h>

/**
 * @brief Initialize the HAL time module.
 */
void hal_time_init(void);

/**
 * @brief Get the current time in milliseconds.
 * @return Current time in milliseconds.
 */
uint32_t hal_time_get(void);

/**
 * @brief Delay execution for a specified number of milliseconds.
 * @param ms Delay duration in milliseconds.
 */
void hal_time_delay(uint32_t ms);

#endif /* HAL_TIME_H */
/* [] END OF FILE */
