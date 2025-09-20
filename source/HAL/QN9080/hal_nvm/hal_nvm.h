/**
 * @file hal_nvm.h
 * @brief Header file for Non-Volatile Memory (NVM) HAL.
 * @note QN9080 specific implementation
 * @version 0.6.0
 */

#ifndef HAL_NVM_H
#define HAL_NVM_H

#include "NVM_Interface.h"

/*!
 * @brief Macro to be called when the system is idle.
 */
#define HAL_NVM_ON_IDLE() NvIdle()

#endif // HAL_NVM_H

/* [] END OF FILE */
