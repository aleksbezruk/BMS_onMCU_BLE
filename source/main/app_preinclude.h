/*! *********************************************************************************
 * @file  app_preinclude.h
 *
 * @brief QN908x configuration file that is used to keep:
 *          - hardware / BSP configuration ;
 *          - software configuration - BLE stack, timers etc.
 *
 * @version 0.5.0
********************************************************************************** */


#ifndef _APP_PREINCLUDE_H_
#define _APP_PREINCLUDE_H_

/*! *********************************************************************************
 * 	BMS specific Configuration
 ********************************************************************************** */
/** 
 *  @note Disable RTOS and/or BLE - temporary solution
 *  @todo Comment out when RTOS port will be ready for QN908x
 */
#define BMS_DISABLE_RTOS

#define BMS_DISABLE_BLE

/*! Enable QSPY */
#define Q_SPY

/*! *********************************************************************************
 * 	Board Configuration
 ********************************************************************************** */
#define gLEDSupported_d         1

/* Defines the number of available keys for the keyboard module */
#define gKBD_KeysCount_c        2

/* Specifies the number of physical LEDs on the target board */
#define gLEDsOnTargetBoardCnt_c 3

/* Specifies if the LED operation is inverted. LED On = GPIO Set */
#define gLED_InvertedMode_d     1

/*! *********************************************************************************
 * 	App Configuration
 ********************************************************************************** */
 
/*! Maximum number of connections supported for this application */
#define gAppMaxConnections_c           1

/*! Number of credit-based channels supported */
#define gL2caMaxLeCbChannels_c         2
 
/*! Enable/disable use of bonding capability */
#define gAppUseBonding_d   0

/*! Enable/disable use of pairing procedure */
#define gAppUsePairing_d   0

/*! Enable/disable use of privacy */
#define gAppUsePrivacy_d   0

#define gPasskeyValue_c                999999

#if (gAppUseBonding_d) && (!gAppUsePairing_d)
  #error "Enable pairing to make use of bonding"
#endif

/* Enamble or disable DC to DC converter */
#define gDCDC_Mode 1

/* Rf Rx Mode:  kRxModeBalanced, kRxModeHighEfficiency  OR  kRxModeHighPerformance
 *  see rx_mode_t enum  */
#define gBleRfRxMode kRxModeHighPerformance

/* Enable HS Clock to support 2Mbps PHY mode setting */
#define gBleUseHSClock2MbpsPhy_c 0

#if ( !defined(gBleUseHSClock2MbpsPhy_c) || (defined(gBleUseHSClock2MbpsPhy_c) && (gBleUseHSClock2MbpsPhy_c == 0))) 
/*! specifies host preferred values for the transmitter PHY to be used for all
subsequent connections over the LE transport */
#define gConnDefaultTxPhySettings_c            (gLePhy1MFlag_c)

/*! specifies host preferred values for the receiver PHY to be used for all
subsequent connections over the LE transport */
#define gConnDefaultRxPhySettings_c            (gLePhy1MFlag_c)
#endif

/*! *********************************************************************************
 * 	Framework Configuration
 ********************************************************************************** */
/* enable NVM to be used as non volatile storage management by the host stack */
#define gAppUseNvm_d                    1

/* Defines Num of Serial Manager interfaces */
#define gSerialManagerMaxInterfaces_c   0

/* Defines Size for Timer Task*/
#define gTmrTaskStackSize_c  600

/* Defines pools by block size and number of blocks. Must be aligned to 4 bytes.*/
#define AppPoolsDetails_c \
         _block_size_  32  _number_of_blocks_    5 _eol_  \
         _block_size_  64  _number_of_blocks_    5 _eol_  \
         _block_size_ 128  _number_of_blocks_    5 _eol_  \
         _block_size_ 512  _number_of_blocks_    5 _eol_

/* Defines number of timers needed by the application */
#define gTmrApplicationTimers_c         5

/* Defines number of timers needed by the protocol stack */
#if defined(gAppMaxConnections_c) && defined(gL2caMaxLeCbChannels_c)
    #define gTmrStackTimers_c (2 + (gAppMaxConnections_c * 2) + gL2caMaxLeCbChannels_c)
#else
    #define gTmrStackTimers_c (6)
#endif

/* Set this define TRUE if the PIT frequency is an integer number of MHZ */
#define gTMR_PIT_FreqMultipleOfMHZ_d    0

/* Enable/Disable Periodical Interrupt Timer */
#define gTMR_PIT_Timestamp_Enabled_d 0

/* Enables / Disables the precision timers platform component */
#define gTimestamp_Enabled_d            0

/*! *********************************************************************************
 * 	RTOS Configuration
 ********************************************************************************** */
/* Defines the RTOS used */
#define FSL_RTOS_FREE_RTOS      1

/* Defines number of OS events used */
#define osNumberOfEvents        5

/* Defines main task stack size */
#define gMainThreadStackSize_c  1100

/* Defines controller task stack size */
#define gControllerTaskStackSize_c 2048

/* Defines total heap size used by the OS - 11k */
#define gTotalHeapSize_c        11264 

/*! *********************************************************************************
 * 	NVM Module Configuration - gAppUseNvm_d shall be defined aboved as 1 or 0
 ********************************************************************************** */    
             
#if gAppUseNvm_d
    #define gNvmMemPoolId_c 1
    /* Defines NVM pools by block size and number of blocks. Must be aligned to 4 bytes.*/
    #define NvmPoolsDetails_c \
         _block_size_ 32   _number_of_blocks_    20 _pool_id_(1) _eol_ \
         _block_size_ 60   _number_of_blocks_    10 _pool_id_(1) _eol_ \
         _block_size_ 80   _number_of_blocks_    10 _pool_id_(1) _eol_ \
         _block_size_ 100  _number_of_blocks_    2 _pool_id_(1) _eol_
             
    /* configure NVM module */
    #define  gNvStorageIncluded_d                (1)
    #define  gNvFragmentation_Enabled_d          (1)
    #define  gUnmirroredFeatureSet_d             (1)
    #define  gNvRecordsCopiedBufferSize_c        (512)
#endif

/*! *********************************************************************************
 * 	BLE Stack Configuration
 ********************************************************************************** */

#define gMaxBondedDevices_c         16
#define gMaxResolvingListSize_c     16

/*! *********************************************************************************
 * 	Memory Pools Configuration
 ********************************************************************************** */ 
             
/* Defines pools by block size and number of blocks. Must be aligned to 4 bytes.
 * DO NOT MODIFY THIS DIRECTLY. CONFIGURE AppPoolsDetails_c
 * If gMaxBondedDevices_c increases, adjust NvmPoolsDetails_c
*/
#if gAppUseNvm_d
    #define PoolsDetails_c \
         AppPoolsDetails_c \
         NvmPoolsDetails_c
#else
    #define PoolsDetails_c \
         AppPoolsDetails_c
#endif

#endif /* _APP_PREINCLUDE_H_ */

/*! *********************************************************************************
 * @}
 ********************************************************************************** */
