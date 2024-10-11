/**
 * @file qspyHelper.h
 * 
 * @brief Helper functions for QSPY & QUtest framework. Also includes API for Application
 * 
 * @version 0.1.0
 */

#include <stdint.h>
#include "qpc.h"

///////////////////
// Definitions
///////////////////

/** QSPY status */
typedef enum {
    QSPY_STATUS_ERROR,
    QSPY_STATUS_SUCCESS
} qspy_status_t;

/** Application-specific trace records */
enum AppRecords { 
    MAIN = QS_USER,
    UTEST,
    BSP,
    ADC,
    BLE_TRACE
};

/** QSPY/Qview cmds */
enum {
    QS_CMD_RED_LED,
    QS_CMD_UT_FUN,
    QS_CMD_MCU_READ_REG,
    QS_CMD_MCU_WRITE_REG,
    QS_CMD_INJECT_ERROR,
    QS_CMD_MAX
};

///////////////////
// API 
///////////////////
uint8_t QS_onStartup(void const *arg);
void QS_onIdle(void);
void QS_onFlush(void);
void QS_onReset(void);
void QS_onCommand(uint8_t cmdId,
                  uint32_t param1, 
                  uint32_t param2, 
                  uint32_t param3);
QSTimeCtr QS_onGetTime(void);
void QS_addUsrRecToDic(enum_t const rec);
void QS_initGlbFilters(void);
void QS_rxCallback(uint8_t *data, uint16_t len);

/******************************** END OF FILE **********************************************************************/