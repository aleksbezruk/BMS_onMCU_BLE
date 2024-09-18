/******************************************************************************
* @file  main.c
*
* @brief This is the source code for CM4 in the the Dual CPU Application.
*
* @version 0.1.0
*/

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "BSP.h"
#include "qspyHelper.h"
#if defined(Q_UTEST)
#include <stdio.h>
#include <gcov.h>
#endif // Q_UTEST
#include "ADC.h"

/*******************************/
/*** Functions prototypes */
/******************************/
static void mainTask_(void);
#if defined(Q_UTEST)
static void utTask_(void);
void __gcov_dump(void); /* internal gcov function to write data */
#endif //Q_UTEST

/*******************************/
/*** Data */
/******************************/
volatile uint32_t mainLoopTimer;

/*******************************/
/*** Code */
/******************************/
#if defined(Q_UTEST)
extern void initialise_monitor_handles(void);
static void initSemihosting(void)
{
    initialise_monitor_handles();
}
#endif //Q_UTEST

int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS) {
        CY_ASSERT(0);
    }

    /** Enable global interrupts */
    __enable_irq();

#if defined(Q_UTEST)
    initSemihosting();
    printf("Semihosting started\n");
#endif //Q_UTEST

    /** Init LEDs */
    BSP_init_led_green();
    BSP_init_led_red();

    /** Init QSPY */
    QS_onStartup(NULL);

    /** Init QSPY dictionary & filters */
    QS_addUsrRecToDic(MAIN);
    QS_addUsrRecToDic(ADC);
#if defined(Q_UTEST)
    QS_addUsrRecToDic(UTEST);
    QS_addUsrRecToDic(BSP);
#endif //Q_UTEST
    QS_initGlbFilters();

    /** dictionaries... */
    QS_FUN_DICTIONARY(&mainTask_);
#if defined(Q_UTEST)
    QS_FUN_DICTIONARY(__gcov_dump);
    BSP_initUTdic();
#endif //Q_UTEST

    /** Init ADC */
    ADC_status_t adcStatus = ADC_init();
    if (adcStatus != ADC_STATUS_OK) {
        CY_ASSERT(0);
    }

    /** Main loop */
    for (;;) {
#if !defined(Q_UTEST)
        mainTask_();
#else
        utTask_();
#endif //Q_UTEST
    }
}


/**
 * @fn mainTask_
 * @brief Main task/dispatcher
 * 
 * @param None
 * @retval None
 */
static void mainTask_(void)
{
    if (mainLoopTimer == 1000U) {
        __disable_irq();
            mainLoopTimer = 0;
        __enable_irq();

        BSP_led_green_toggle();

        QS_BEGIN_ID(MAIN, 0 /*prio/ID for local Filters*/)
            QS_STR("Running main loop");
        QS_END()

        /** ADC measurements */
        ADC_task();
    } else {
        /** Do job on Idle */
        QS_onIdle();
    }
}

#if defined(Q_UTEST)
/**
 * @fn utTask_
 * @brief Unit tests task
 * 
 * @param None
 * @retval None
 */
static void utTask_(void)
{
    QS_onIdle();
}
#endif //Q_UTEST

/* [] END OF FILE */
