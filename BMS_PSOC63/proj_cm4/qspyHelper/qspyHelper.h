/**
 * @file qspyHelper.h
 * @brief Helper functions for QSPY & QUtest framework. Also includes API for Application
 * 
 * @version 0.1.0
 */

#include <stdint.h>
#include "qpc.h"

/*******************************/
/*** DEfinitions */
/******************************/
/** QSPY status */
typedef enum {
    QSPY_STATUS_ERROR,
    QSPY_STATUS_SUCCESS
} qspy_status_t;

/** Application-specific trace records */
enum AppRecords { 
    MAIN = QS_USER,
    BSP
};

/** QSPY/Qview cmds */
enum {
    QS_CMD_RED_LED,
    QS_CMD_MAX
};

/*******************************/
/*** API */
/******************************/
/**
 * @fn QS_onStartup
 * @param[in] arguments
 * 
 * @retval QSPY_STATUS_SUCCESS - on success, QSPY_STATUS_ERROR - on error
 */
uint8_t QS_onStartup(void const *arg);

/**
 * @fn QS_onIdle
 * @param None
 * @brief Sends data to Host with running QSPY. Application calls the function from Idle task
 * 
 * @retval None
 */
void QS_onIdle(void);

/**
 * @fn QS_onFlush
 * @param None
 * @brief Sends data to Host in blocking mode.
 * 
 * @retval None
 */
void QS_onFlush(void);

/**
 * @fn QS_onReset
 * @param None
 * @brief Resets CPU/MCU .
 * 
 * @retval None
 */
void QS_onReset(void);

/**
 * @fn QS_onCommand
 * @param[in] cmdId - command ID
 * @param[in] param1 - 1st cmd's parameter
 * @param[in] param2 - 2nd cmd's parameter
 * @param[in] param3 - 3rd cmd's parameter
 * @brief Handles a cmd received from Host.
 * 
 * @retval None
 */
void QS_onCommand(uint8_t cmdId,
                  uint32_t param1, 
                  uint32_t param2, 
                  uint32_t param3);

/**
 * @fn QS_onGetTime
 * @param None
 * @brief Helper function for QSPY & QUtest franework .
 * 
 * @retval None
 */
QSTimeCtr QS_onGetTime(void);

/**
 * @fn QS_addUsrRecToDic
 * @param[in] rec - user record  \ref AppRecords
 * @brief Add user/app recrods group to dictionary 
 * 
 * @retval None
 */
void QS_addUsrRecToDic(enum_t const rec);

/**
 * @fn QS_initGlbFilters
 * @param None
 * @brief Init global filters
 * 
 * @retval None
 */
void QS_initGlbFilters(void);

/******************************** END OF FILE **********************************************************************/