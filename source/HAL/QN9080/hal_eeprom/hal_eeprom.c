/**
 * @file  hal_eeprom.h
 *
 * @brief Declaration of HAL EEPROM functions
 *
 * @note QN9080
 * 
 * @version 0.7.0
 */

#include "hal_eeprom.h"
#include "nvds.h"

#include "qspyHelper.h"

// ==========================
// Code
// ==========================
HAL_EEPROM_status HAL_EEPROM_getData(HAL_EEPROM_tag_t tag, uint16_t *length, uint8_t *buf)
{
    uint8_t status = nvds_get(tag, length, buf);

    return ((status == NVDS_OK)? HAL_EEPROM_OK: HAL_EEPROM_FAIL);
}

HAL_EEPROM_status HAL_EEPROM_putData(HAL_EEPROM_tag_t tag, uint16_t len, uint8_t *buf)
{
    uint8_t status = nvds_put(tag, len, buf);

    QS_BEGIN_ID(HAL, 0)
        QS_STR("EEPROM save data, status:");
        QS_U8(0, status);
    QS_END()   

    return ((status == NVDS_OK)? HAL_EEPROM_OK: HAL_EEPROM_FAIL);
}

/* [] END OF FILE */
