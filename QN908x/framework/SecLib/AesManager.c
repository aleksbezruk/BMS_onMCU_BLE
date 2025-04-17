/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* This is the source file for the AES Manager.
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "AesManager.h"
#include "Panic.h"
#include "FunctionLib.h"
#include "fsl_os_abstraction.h"
#include "fsl_common.h"
#include "SecLib.h"

/*! *********************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
********************************************************************************** */

/* Define the Manager status */
typedef enum
{
	gAESMGR_Evt_Start = (1<<0),
	gAESMGR_Evt_Complete = (1<<1)
} AESEventType_t;

/* Defines the AES encryption/decryption structure */
typedef struct AESMgr_tag
{
    /* AES type,include ECB CTR CMAC */
    AESType_t AESType;

	/* AES manager task callback  */
	pAESCallBack_t AESCallback;
	AES_param_t AES_param;

	/* user callback for complete AES encryption/decryption */
	pAESCompleteCallBack_t AESCompleteCallBack;
	AES_param_t AESComplete_param;

	volatile uint8_t events;
    volatile uint8_t state;
} AESMgr_t;


/*! *********************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
********************************************************************************** */
static void AESManagerTask(osaTaskParam_t argument);

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */
extern const uint8_t gUseRtos_c;

/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */
OSA_TASK_DEFINE( AESManagerTask, gAESTaskPriority_c, 1, gAESTaskStackSize_c, FALSE );

static osaTaskId_t gAESManagerTaskId;
static osaEventId_t mAESMTaskEventId;
static AESMgr_t mAES[gAESManagerMaxNumbers_c];

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
* \brief   Creates the AESManager's task and initializes internal data structures
*
********************************************************************************** */
void AESM_Initialize( void )
{
    static uint8_t AESInitialized = FALSE;

    /* Check if AESMGR is already initialized */
	if( AESInitialized == FALSE )
    {
        AESInitialized = TRUE;

		/* Fill the structure with zeros */
        FLib_MemSet( mAES, 0x00, sizeof(mAES) );

        mAESMTaskEventId = OSA_EventCreate( TRUE );
        if( NULL == mAESMTaskEventId )
        {
            panic(0,0,0,0);
        }
        else
        {
            gAESManagerTaskId = OSA_TaskCreate(OSA_TASK(AESManagerTask), NULL);
            if( NULL == gAESManagerTaskId )
            {
                panic(0,0,0,0);
            }
        }
    }
}

/*! *********************************************************************************
* \brief   Initialize a AES type
*
* \param[in] pAESId    pointer to a location where the encryption Id will be stored
* \param[in] type      the type of the AES:
*							gAESMGR_ECB_Enc_c
*							gAESMGR_ECB_Dec_c
*							gAESMGR_ECB_Block_Enc_c
*							gAESMGR_ECB_Block_Dec_c
*							gAESMGR_CTR_Enc_c
*							gAESMGR_CTR_Dec_c
*							gAESMGR_CMAC_Enc_c
* \return The AES number if success or gAES_MaxNumbersReached_c when reach max numbers.
*
********************************************************************************** */
AESStatus_t AESM_InitType( uint8_t* pAESId, AESType_t type )
{
    AESStatus_t status = gAES_Success_c;
#if gAESManagerMaxNumbers_c
    uint32_t i;
    AESMgr_t *pAES;

    *pAESId = gAESMgrInvalidIdx_c;

    for ( i=0; i<gAESManagerMaxNumbers_c; i++ )
    {
        pAES = &mAES[i];

		if ( (pAES->AESType == type) )
        {
            /* The AES type is allready used. */
            status = gAES_TypeInUse_c;
            break;
        }

        if ( pAES->AESType == gAESMGR_None_c )
        {
			pAES->AESType = type;
			*pAESId = i;
        }
    }
    /* There are no more free type numbers. */
    if( i == gAESManagerMaxNumbers_c )
    {
        status = gAES_MaxNumbersReached_c;
    }
#else
    (void)type;
    (void)pAESId;
#endif
    return status;
}


/*! *********************************************************************************
* \brief   Set AES encryption/decryption parameter
*
* \param[in] pAESId    AES encryption/decryption type Id
* \param[in] AES_p     the parameter of the AES HW
* \return    gAES_InvalidParameter_c if an error occured.
*
********************************************************************************** */
AESStatus_t AESM_SetParam( uint8_t pAESId,
				    	   AES_param_t AES_p,
						   pAESCallBack_t AEScallback )
{
#if gAESManagerMaxNumbers_c
    AESMgr_t *pAES;

	if( ( pAESId >= gAESManagerMaxNumbers_c ) || ( AEScallback == NULL ))
	{
		return gAES_InvalidParameter_c;
	}

	pAES = &mAES[pAESId];

	pAES->AES_param.CTR_counter = AES_p.CTR_counter;
	pAES->AES_param.Key	= AES_p.Key;
	pAES->AES_param.Len	= AES_p.Len;
	pAES->AES_param.pCipher = AES_p.pCipher;
	pAES->AES_param.pInitVector = AES_p.pInitVector;
	pAES->AES_param.pPlain = AES_p.pPlain;
	pAES->AES_param.Blocks = AES_p.Blocks;
	pAES->AESCallback = AEScallback;
#else
    (void)pAESId;
    (void)AES_p;
#endif
    return gAES_Success_c;
}

/*! *********************************************************************************
* \brief     start encryption/decryption
*
* \param[in] pAESId   AES encryption/decryption type Id
*
* \return 	 gAES_InvalidParameter_c and gAES_Non_excution_c if an error occured.
*
********************************************************************************** */
AESStatus_t AESM_Start( uint8_t pAESId )
{
	AESMgr_t *pAES;
	pAES = &mAES[pAESId];

	if(( pAES->AES_param.pPlain==NULL ) || ( pAES->AES_param.pCipher==NULL )
		 ||( pAES->AES_param.Len==0 ) || ( pAESId >= gAESManagerMaxNumbers_c ))
	{
		return gAES_InvalidParameter_c;
	}

	/* Encryption or decryption is on going*/
	if(pAES->state == ENC_PENDING)
	{
		return gAES_Non_excution_c;
	}
	else
	{
		if( (pAES->events & gAESMGR_Evt_Start) ==0 )
		{
			pAES->events |= gAESMGR_Evt_Start;
			(void)OSA_EventSet(mAESMTaskEventId, gAESMGR_Evt_Start);
			return gAES_Success_c;
		}
		return gAES_Non_excution_c;
	}
}

/*! *********************************************************************************
* \brief     set complete event when finish encryption/decryption
*
* \param[in] pAESId   AES encryption/decryption type Id
*
* \return    gAES_InvalidParameter_c and gAES_Non_excution_c if an error occured.
*
********************************************************************************** */
AESStatus_t AESM_Complete( uint8_t pAESId )
{
	AESMgr_t *pAES;
	pAES = &mAES[pAESId];

	if((pAES->AES_param.pPlain==NULL) || (pAES->AES_param.pCipher==NULL) ||(pAES->AES_param.Len==0)
		|| (pAESId >= gAESManagerMaxNumbers_c))
	{
		return gAES_InvalidParameter_c;
	}

	/* Encryption or decryption is on going*/
	pAES->events |= gAESMGR_Evt_Complete;
	(void)OSA_EventSet(mAESMTaskEventId, gAESMGR_Evt_Complete);
	pAES->state = ENC_PENDING;
	return gAES_Success_c;
}

/*! *********************************************************************************
* \brief     get state of encryption/decryption
*
* \param[in] pAESId   AES encryption/decryption type Id
*
* \return    encryption/decryption state
*
********************************************************************************** */
uint8_t	AESM_GetState( uint8_t pAESId )
{
	return mAES[pAESId].state;
}

/*! *********************************************************************************
* \brief     set callback function ,when encryption/decryption complete it will be execute
*
* \param[in] pAESId     AES encryption/decryption type Id
*
********************************************************************************** */
void AESM_SetCompletionCallback( uint8_t pAESId, pAESCompleteCallBack_t cb )
{
	mAES[pAESId].AESComplete_param.pCipher = mAES[pAESId].AES_param.pCipher;
	if( mAES[pAESId].AESType == gAESMGR_CMAC_Enc_c )
	{
		mAES[pAESId].AESComplete_param.Len = 16;
	}
	else
	{
		mAES[pAESId].AESComplete_param.Len = mAES[pAESId].AES_param.Len;
	}
	mAES[pAESId].AESCompleteCallBack = cb;
}

/*! *********************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
********************************************************************************** */
/*! *********************************************************************************
* \brief   The main task of the  AES Manager
*
* \param[in] initialData unused
*
********************************************************************************** */
static void AESManagerTask(osaTaskParam_t argument)
{
    uint16_t i;
    uint8_t ev;

	osaEventFlags_t  mAESMTaskEventFlags = 0;

	while( 1 )
	{
		/* Wait for an event. The task will block here. */
		(void)OSA_EventWait(mAESMTaskEventId, osaEventFlagsAll_c, FALSE, osaWaitForever_c ,&mAESMTaskEventFlags);

		for( i = 0; i < gAESManagerMaxNumbers_c; i++ )
		{
			OSA_InterruptDisable();
			ev = mAES[i].events;
			mAES[i].events = 0;
			OSA_InterruptEnable();

			if( (ev & gAESMGR_Evt_Start) &&
				(NULL != mAES[i].AESCallback))
			{
				mAES[i].state = ENC_PENDING;
				mAES[i].AESCallback( mAES[i].AES_param );
				mAES[i].state = ENC_COMPLETE;
			}

			if ( (ev & gAESMGR_Evt_Complete) &&
				 (NULL != mAES[i].AESCompleteCallBack))
			{
				if( (mAES[i].AESType == gAESMGR_ECB_Dec_c) || (mAES[i].AESType == gAESMGR_ECB_Block_Dec_c) || (mAES[i].AESType == gAESMGR_CTR_Dec_c) )
				{
					mAES[i].AESCompleteCallBack( mAES[i].AESComplete_param.pPlain, mAES[i].AESComplete_param.Len );
				}
				else
				{
					mAES[i].AESCompleteCallBack( mAES[i].AESComplete_param.pCipher, mAES[i].AESComplete_param.Len );
				}
					mAES[i].state = ENC_COMPLETE;
			}
		}

        /* For BareMetal break the while(1) after 1 run */
        if (gUseRtos_c == 0)
        {
            break;
        }
    } /* while(1) */
}