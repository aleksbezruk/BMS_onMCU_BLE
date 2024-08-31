/*******************************************************************************
 * \file test_BSP.c
 * \version 0.1.0
 *
 * Helper for testing of BSP
 ********************************************************************************/
#include "cy_pdl.h"
#include "cycfg.h"
#include "BSP.h"
#include "qspyHelper.h"
#include "qpc.h"

/*********************************************************************************
 * Func prototypes
 */
void BSP_power_init_test(void);

/*********************************************************************************
 * Tests ...
 */
void BSP_test_init(void)
{
    QS_FUN_DICTIONARY(BSP_power_init_test);
}

void BSP_power_init_test(void)
{
    cy_en_syspm_status_t status = BSP_power_init();

    QS_BEGIN_ID(BSP, 0U)                              // app-specific record
        QS_FUN((void(*)(void)) &BSP_power_init_test);  // function called
        QS_STR("RETURN_VAL");                          // SPARE string
        QS_U32(0, (uint32_t) status);                 // status/retVal
    QS_END() 
    QS_FLUSH();
}

/************************** END OF FILE *****************************************/
