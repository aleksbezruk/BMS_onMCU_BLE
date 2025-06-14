/**
 * @file  hal.c
 * @brief Implementation of generic HAL functions like init board/hardware, generic utils
 * @note PSOC63
 * @version 0.5.0
 */

#include "hal.h"
#include "cybsp.h"


// =======================
// Code
// =======================
/**
 * @brief Generic API to init underlying hardware. Purpose is
 *        to configure all hardware (power, clocks, peripheral) to be ready for
 *        further BMS operations.
 * 
 * @note Configures peripherals pins as well: UART, ADC pins.
 * 
 * @details In general it may use low level BSP provided by
 *          particular Vendor if such exist and (or) use autogenerate vendor tool(s)
 *          that allow to configure hardware using convenient GUI. Using such COTS tools allow
 *          us to speed up development/prototyping process.
 * 
 * @param   None
 * 
 * @retval See \ref HAL_status_t. HAL_STATUS_OK - if success.
 */
HAL_status_t HAL_init_hardware(void)
{
    HAL_status_t status = HAL_STATUS_UNKNOWN;

    cy_rslt_t result = cybsp_init();
    if (result == CY_RSLT_SUCCESS) {
        status = HAL_STATUS_OK;
    } else {
        status = HAL_STATUS_FAIL;
    }

    return status;
}

/* [] END OF FILE */
