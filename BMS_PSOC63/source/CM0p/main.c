/******************************************************************************
* @file  main.c
*
* @brief This is the source code for CM0+ in the the Dual CPU Application.
*
* @version 0.1.0
*/

#include "cy_pdl.h"
#include "cycfg.h"
#include "cybsp.h"

int main(void)
{
    /* Enable global interrupts */
    __enable_irq();
    
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable CM4. CY_CORTEX_M4_APPL_ADDR must be updated if CM4 memory layout is changed. */
    Cy_SysEnableCM4(CY_CORTEX_M4_APPL_ADDR);

    for (;;)
    {
        // Cy_SysPm_DeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
    }
}

/* [] END OF FILE */
