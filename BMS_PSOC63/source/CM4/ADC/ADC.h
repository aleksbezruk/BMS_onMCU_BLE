/******************************************************************************
* @file  ADC.h
*
* @brief Analog to digital converter module functions declaration.
*
* @version 0.1.0
*/

/*******************************/
/*** Defines */
/******************************/
typedef enum {
    ADC_STATUS_OK,
    ADC_STATUS_FAIL
} ADC_status_t;

/*******************************/
/*** Code */
/******************************/
ADC_status_t ADC_init(void);

/* [] END OF FILE */
