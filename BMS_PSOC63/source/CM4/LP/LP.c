/**
 * @file  LP.c
 *
 * @brief Implements Low Power modes and tickless mode of the BMS/MCU.
 *
 * @version 0.4.0
 */

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

#include "LP.h"
#include "qspyHelper.h"

///////////////////////
// Private data
///////////////////////
static volatile LP_modes_t _mode;

///////////////////////
// Code
///////////////////////
/**
 * @brief Get BMS peripherals rediness status for Low Power mode
 * 
 * @param None
 * 
 * @retval see @LP_periph_ready_t
 */
LP_periph_ready_t LP_getPeriphStatus(void)
{
    LP_periph_ready_t readiness = LP_PERIPH_READY;

    QSPY_rx_status_t qspy_rx_status = QS_get_rxStatus();
    if (qspy_rx_status == QSPY_RX_NOT_EMPTY) {
        readiness = LP_PERIPH_NOT_READY;
    }

    return readiness;
}

/**
 * @brief Get BMS peripherals rediness status for Low Power mode
 * 
 * @param[in] mode - Low Power mode to set, see @LP_modes_t
 * 
 * @retval see @LP_periph_ready_t
 */
void LP_setMode(LP_modes_t mode)
{
    CY_ASSERT((mode >= LP_DISABLED_MODE) && (mode <= LP_SHELF_MODE));
    _mode = mode;
}

/**
 * @brief Get BMS peripherals rediness status for Low Power mode
 * 
 * @param None
 * 
 * @retval None
 */
void LP_enterSleep(void)
{
    switch (_mode)
    {
        case LP_DISABLED_MODE:
        {
            // do nothing
            break;
        }

        case LP_SLEEP_MODE:
        {
            /** @todo: implement */
            __asm("nop");
            break;
        }

        case LP_DEEP_SLEEP_MODE:
        {
            /** @todo: implement */
            __asm("nop");
            break;
        }

        default:
        {
            CY_ASSERT(0);   // unexpected behavior
        }
    }
}

/* [] END OF FILE */
