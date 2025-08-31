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
#include "EmbeddedTypes.h"
#include "FunctionLib.h"

#include "aes_reg_access.h"
#include "fsl_aes.h"

/*******************************************************************************
 * Definitions
 *****************************************************************************/
#define HW_AES_MIN_WAIT_CNT      (0xFFFF)    /* cycle check status count*/

/*******************************************************************************
 * Variables
 ******************************************************************************/
extern struct llm_le_env_tag llm_le_env;
extern bool rwble_sleep_check(void);

/*******************************************************************************
 * Code
 ******************************************************************************/

aes_enc_status_t AES_128_Encrypt_HW(const uint8_t* pInput, const uint8_t* pKey, uint8_t* pOutput)
{
        uint8_t tempBuffIn[AES_BLOCK_SIZE];
        uint8_t tempBuffOut[AES_BLOCK_SIZE];
        uint8_t encKey[AES_BLOCK_SIZE];
        uint32_t wait_cnt = HW_AES_MIN_WAIT_CNT;

        if (!rwble_sleep_check())
        {
         return HW_AES_Previous_Enc_on_going;
        }

        if( *(uint8_t *)(0x04000168 + 76) == false )//llm_le_env.enc_pend
        {
                /* modify the text block/key endian mode for the encryption.*/
                FLib_MemCpyReverseOrder (encKey, pKey, AES_BLOCK_SIZE);
                FLib_MemCpyReverseOrder (tempBuffIn, pInput, AES_BLOCK_SIZE);

                /* copy the key in the register dedicated for the encryption */
                ble_aeskey31_0_set(co_read32p(encKey));
                ble_aeskey63_32_set(co_read32p(encKey+4));
                ble_aeskey95_64_set(co_read32p(encKey+8));
                ble_aeskey127_96_set(co_read32p(encKey+12));

                /* copy data from sys ram to em */
                em_wr(tempBuffIn, EM_BLE_ENC_PLAIN_OFFSET, EM_BLE_ENC_LEN);

                /* set the pointer on the data to encrypt */
                ble_aesptr_set(EM_BLE_ENC_PLAIN_OFFSET);

                /* start the encryption */
                ble_aescntl_set(BLE_AES_START_BIT);

                /* Check BLE interrupt status */
                while(!(ble_intstat_get() & BLE_CRYPTINTSTAT_BIT))
                {
                        if((wait_cnt--)==0)
                        {
                                return HW_AES_Enc_Timeout;
                        }
                }

                /* Clear status */
                ble_intack_clear(BLE_CRYPTINTSTAT_BIT);

                /* copy data from em to sys ram */
                em_rd(tempBuffOut, EM_BLE_ENC_CIPHER_OFFSET, 0x10);

                /* modify the cipher endian for the encryption.*/
                FLib_MemCpyReverseOrder(pOutput, tempBuffOut, AES_BLOCK_SIZE);

                return HW_AES_Enc_Success;
        }
        else
        {
                return HW_AES_Previous_Enc_on_going;
        }
}

aes_enc_status_t AES_128_Decrypt_HW(const uint8_t* pInput, const uint8_t* pKey, uint8_t* pOutput)
{
        uint8_t tempBuffIn[AES_BLOCK_SIZE] = {0};
        uint8_t tempBuffOut[AES_BLOCK_SIZE] = {0};
        uint8_t decKey[AES_BLOCK_SIZE] = {0};
        uint32_t wait_cnt = HW_AES_MIN_WAIT_CNT;

        if (!rwble_sleep_check())
        {
         return HW_AES_Previous_Enc_on_going;
        }

        if( *(uint8_t *)(0x04000168 + 76) == false )
        {
                /* modify the text block/key endian mode for the decryption.*/
                FLib_MemCpyReverseOrder (decKey, pKey, AES_BLOCK_SIZE);
                FLib_MemCpyReverseOrder (tempBuffIn, pInput, AES_BLOCK_SIZE);

                /* copy the key in the register dedicated for the encryption */
                ble_aeskey31_0_set(co_read32p(decKey));
                ble_aeskey63_32_set(co_read32p(decKey+4));
                ble_aeskey95_64_set(co_read32p(decKey+8));
                ble_aeskey127_96_set(co_read32p(decKey+12));

                /* copy data from sys ram to em */
                em_wr(tempBuffIn, EM_BLE_ENC_PLAIN_OFFSET, EM_BLE_ENC_LEN);

                /* set the pointer on the data to encrypt */
                ble_aesptr_set(EM_BLE_ENC_PLAIN_OFFSET);

                /* start the decryption */
                ble_aescntl_set(BLE_AES_MODE_BIT | BLE_AES_START_BIT);

                /* Check BLE interrupt status */
                while(!(ble_intstat_get() & BLE_CRYPTINTSTAT_BIT))
                {
                        if((wait_cnt--)==0)
                        {
                                return HW_AES_Enc_Timeout;
                        }
                }

                /* Clear status */
                ble_intack_clear(BLE_CRYPTINTSTAT_BIT);

                /* copy data from em to sys ram */
                em_rd(tempBuffOut, EM_BLE_ENC_CIPHER_OFFSET, 0x10);

                /* modify the cipher endian for the decryption.*/
                FLib_MemCpyReverseOrder(pOutput, tempBuffOut, AES_BLOCK_SIZE);

                return HW_AES_Enc_Success;
        }
        else
        {
                return HW_AES_Previous_Enc_on_going;
        }
}
