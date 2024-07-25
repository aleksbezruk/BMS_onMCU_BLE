/**
 * @file qspyHelper.c
 * @brief IMplements helper functions for QSPY & QUtest framework. 
 *        Also includes API for Application.
 * 
 * @version 0.1.0
 */

#include "cy_pdl.h"
#include "BSP.h"
// #include "qspyHelper.h"

/*******************************/
/*** Helper functions */
/******************************/
void QS_onReset(void) {
    NVIC_SystemReset();
}

// qspy_status_t QS_onStartup(void) {

// }

/*******************************/
/*** Public API */
/******************************/

/******************************** END OF FILE **********************************************************************/
