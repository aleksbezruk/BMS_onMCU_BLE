/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for CM4 in the the Dual CPU Empty 
*              Application for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "BSP.h"
#include "qspyHelper.h"
#if defined(Q_UTEST)
#include <stdio.h>
#include <gcov.h>
#endif // Q_UTEST

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

    BSP_init_led_green();
    BSP_init_led_red();

    /** Init QSPY */
    QS_onStartup(NULL);

    /** Init QSPY dictionary & filters */
    QS_addUsrRecToDic(MAIN);
#if defined(Q_UTEST)
    QS_addUsrRecToDic(UTEST);
#endif //Q_UTEST
    QS_initGlbFilters();

    /** dictionaries... */
    QS_FUN_DICTIONARY(&mainTask_);
#if defined(Q_UTEST)
    QS_FUN_DICTIONARY(__gcov_dump);
#endif //Q_UTEST

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
