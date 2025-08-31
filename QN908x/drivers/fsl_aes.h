/*
 * The Clear BSD License
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSL_AES_H_
#define _FSL_AES_H_

#include "fsl_common.h"
#include "aes_reg_access.h"
/*!
 * @addtogroup qn_aes
 * @{
 */

/*! @file */
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @name Driver version */
/*@{*/
/*! @brief QN AES driver version 1.0.0. */
#define FSL_AES_DRIVER_VERSION (MAKE_VERSION(1, 0, 0))
/*@}*/

#define AES_BLOCK_SIZE     16 /* [bytes] */

/*! @brief Disable system interrupts */
#define HW_AES_GLOBAL_INT_DISABLE()   __disable_irq();

/*! @brief Enable system interrupts */
#define HW_AES_GLOBAL_INT_ENABLE()   __enable_irq();


/*! @brief HW AES Encryption status */
typedef enum _aes_enc_status
{
        HW_AES_Enc_Success                   = 0U,           /*HW AES encryption success  */
        HW_AES_Enc_Timeout                   = 1U,           /*HW AES module crash  */
        HW_AES_Previous_Enc_on_going         = 2U,           /*HW AES encryption is on going */
        HW_AES_Enc_End                       = 0xFFFFFFFF    /*HW AES encryption not start*/
}aes_enc_status_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization
 * @{
 */

/*! 
* @brief  This function performs AES-128 encryption on a 16-byte block.
*
* @param[in]  pInput Pointer to the location of the 16-byte plain text block.
*
* @param[in]  pKey Pointer to the location of the 128-bit key.
*
* @param[out]  pOutput Pointer to the location to store the 16-byte ciphered output.
*
* @pre All Input/Output pointers must refer to a memory address aligned to 4 bytes!
*
*/
aes_enc_status_t AES_128_Encrypt_HW(const uint8_t* pInput, const uint8_t* pKey, uint8_t* pOutput);

/*!
* @brief  This function performs AES-128 decryption on a 16-byte block.
*
* @param[in]  pInput Pointer to the location of the 16-byte plain text block.
*
* @param[in]  pKey Pointer to the location of the 128-bit key.
*
* @param[out]  pOutput Pointer to the location to store the 16-byte ciphered output.
*
* @pre All Input/Output pointers must refer to a memory address alligned to 4 bytes!
*
*/
aes_enc_status_t AES_128_Decrypt_HW(const uint8_t* pInput, const uint8_t* pKey, uint8_t* pOutput);

/*! @}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_RNG_H_ */
