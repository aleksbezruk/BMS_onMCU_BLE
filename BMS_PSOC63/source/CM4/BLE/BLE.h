/******************************************************************************
* @file  BLE.h
*
* @brief BLE module functions declaration.
*
* @version 0.1.0
*/

/*******************************/
/*** Defines */
/******************************/
typedef enum {
    BLE_STATUS_OK,
    BLE_STATUS_FAIL
} BLE_status_t;

/*******************************/
/*** Code */
/******************************/
BLE_status_t BLE_init(void);

/* [] END OF FILE */
