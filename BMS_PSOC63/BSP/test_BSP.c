/*******************************************************************************
 * \file test_BSP.c
 * \version 0.4.0
 *
 * Helper for testing of BSP
 ********************************************************************************/
#include "cy_pdl.h"
#include "cycfg.h"
#include "BSP.h"
#include "qspyHelper.h"
#include "qpc.h"
#include "cy_syslib.h"

/*********************************************************************************
 * Func prototypes
 */
void BSP_power_init_test(void);
void BSP_clock_wcoInit_test(void);
void BSP_initUart_test(void);

/*********************************************************************************
 * Tests ...
 */
void BSP_test_init(void)
{
    QS_FUN_DICTIONARY(BSP_power_init_test);
    QS_FUN_DICTIONARY(BSP_clock_wcoInit_test);
    QS_FUN_DICTIONARY(BSP_initUart_test);
}

/***************************************************
 * Power domain tests
 ***************************************************/
void BSP_power_init_test(void)
{
    cy_en_syspm_status_t status = BSP_power_init();

    QS_BEGIN_ID(BSP, 0U)                               // app-specific record
        QS_FUN((void(*)(void)) &BSP_power_init_test);  // function called
        QS_STR("RETURN_VAL");                          // SPARE string
        QS_U32(0, (uint32_t) status);                  // status/retVal
    QS_END() 
    QS_FLUSH();
}

/***************************************************
 * Clock domain tests
 ***************************************************/
void BSP_clock_wcoInit_test(void)
{
    cy_en_sysclk_status_t status = BSP_clock_wcoInit();

    QS_BEGIN_ID(BSP, 0U)                                  // app-specific record
        QS_FUN((void(*)(void)) &BSP_clock_wcoInit_test);  // function called
        QS_STR("RETURN_VAL");                             // SPARE string
        QS_U32(0, (uint32_t) status);                     // status/retVal
    QS_END() 
    QS_FLUSH();
}

/***************************************************
 * UART domain tests
 ***************************************************/
void BSP_initUart_test(void)
{
    QS_FLUSH(); // send all data before UART re-init
    Cy_SysLib_Delay(1000U); // ms

    bsp_status_init_t status = BSP_initUart(&QS_rxCallback);

    QS_BEGIN_ID(BSP, 0U)                                  // app-specific record
        QS_FUN((void(*)(void)) &BSP_initUart_test);       // function called
        QS_STR("RETURN_VAL");                             // SPARE string
        QS_U32(0, (uint32_t) status);                     // status/retVal
    QS_END() 
    QS_FLUSH();
}

/************************** END OF FILE *****************************************/
