/**
 * @file  hal_eeprom.h
 *
 * @brief Definition of HAL EEPROM functions
 *
 * @note QN9080
 * 
 * @version 0.7.0
 */

#ifndef HAL_EEPROM_MODULE_H
#define HAL_EEPROM_MODULE_H

#include <stdint.h>

/*! 
 * EEPROM tags definition.
 * @note The application TAGs should be subset of NXP SDK tags, \ref enum NVDS_TAG in nvds.h
 */
typedef enum {
    HAL_EEPROM_TAG_PCBA_TRIM = 0x7F
} HAL_EEPROM_tag_t;

/*! EEPROM status */
typedef enum {
    HAL_EEPROM_OK,
    HAL_EEPROM_FAIL
} HAL_EEPROM_status;

// ============================
// APIs
// ============================
HAL_EEPROM_status HAL_EEPROM_getData(HAL_EEPROM_tag_t tag, uint16_t *length, uint8_t *buf);
HAL_EEPROM_status HAL_EEPROM_putData(HAL_EEPROM_tag_t tag, uint16_t len, uint8_t *buf);

#endif /* HAL_EEPROM_MODULE_H */

/* [] END OF FILE */
