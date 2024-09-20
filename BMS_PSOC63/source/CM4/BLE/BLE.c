/******************************************************************************
* @file  BLE.c
*
* @brief BLE module functions definition.
*
* @details
*           https://documentation.infineon.com/html/psoc6/moa1717991724927.html#moa1717991724927 
*           https://github.com/Infineon/btstack 
*           https://github.com/Infineon/btstack-integration
*
*           *********************************
*           1. *** BLE stack details: ***
*           *********************************
*             1.1 Infineon/Cypress stack is used -> AIROC™ BTSTACK
*             1.2 Bluetooth® porting layer -> 
*                 AIROC™ BTSTACK requires a porting layer specific to the device it is running on. 
*             1.3 Porting layer for Infineon Bluetooth® devices 
*                 is hosted on GitHub as a library called btstack-integration
*             1.4 Cypress/Infineon BLE stack requires OS layer based on RTOS.
*                 Porting/integration layer of btstack is based oo a FreeRTOS package.
*                 The FreeRTOS package is delivered by Cypress and their BLE stack
*                 looks like is tested on FreeRTOS, so FreeRTOS also will be used 
*                 in the current project .
*           
*           ******************************************************
*           2. *** BLE stack & FreeRTOS libraries/packages config 
*           ******************************************************
*             2.1 Run library manager on 'BMS_PSOC63' folder:
*                    make library-manager
*             2.2 Select library: btstack-integration
*             2.3 Select library: btstack
*             2.4 Select library: freertos
*
*           ******************************************************
*           3. BLE stack architecture
*           ******************************************************
*             3.1 TODO
*
*           ******************************************************
*           4. BLE profile for BMS implementation details
*           ******************************************************
*             4.1 TODO
*
* @version 0.1.0
*/

/*******************************/
/*** Defines */
/******************************/

/*******************************/
/*** Private data */
/******************************/

/*******************************/
/*** Code */
/******************************/
void BLE_init(void)
{

}


/* [] END OF FILE */
