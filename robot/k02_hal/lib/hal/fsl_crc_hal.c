/*
 * Copyright (c) 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
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
#include "fsl_crc_hal.h"

/*******************************************************************************
 * Code
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_HAL_Init
 * Description   : This function initializes the module to a known state.
 *
 *END**************************************************************************/
void CRC_HAL_Init(uint32_t baseAddr)
{
    uint32_t seedAndData = 0;

    BW_CRC_CTRL_TCRC(baseAddr, kCrc32Bits);
    /*SetReadTranspose (no transpose)*/
    BW_CRC_CTRL_TOTR(baseAddr, kCrcNoTranspose);
    /*SetWriteTranspose (no transpose)*/
    BW_CRC_CTRL_TOT(baseAddr, kCrcNoTranspose);
    /*SetXorMode (xor mode disabled)*/
    BW_CRC_CTRL_FXOR(baseAddr, false);
    /*SetSeedOrDataMode (seed selected)*/
    BW_CRC_CTRL_WAS(baseAddr, true);

#if FSL_FEATURE_CRC_HAS_CRC_REG
    HW_CRC_CRC_WR(baseAddr, seedAndData);
#else
    HW_CRC_DATA_WR(baseAddr, seedAndData);
#endif
    /*SetSeedOrDataMode (seed selected)*/
    BW_CRC_CTRL_WAS(baseAddr, false);

#if FSL_FEATURE_CRC_HAS_CRC_REG
    HW_CRC_CRC_WR(baseAddr, seedAndData);
#else
    HW_CRC_DATA_WR(baseAddr, seedAndData);
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_HAL_GetCrc32
 * Description   : This method appends 32-bit data to current CRC calculation
 *                 and returns new result
 *
 *END**************************************************************************/
uint32_t CRC_HAL_GetCrc32(uint32_t baseAddr, uint32_t data, bool newSeed, uint32_t seed)
{
    if (newSeed == true)
    {
        CRC_HAL_SetSeedOrDataMode(baseAddr, true);
        CRC_HAL_SetDataReg(baseAddr, seed);
        CRC_HAL_SetSeedOrDataMode(baseAddr, false);
        CRC_HAL_SetDataReg(baseAddr, data);
        return CRC_HAL_GetCrcResult(baseAddr);
    }
    else
    {
        CRC_HAL_SetDataReg(baseAddr, data);
        return CRC_HAL_GetCrcResult(baseAddr);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_HAL_GetCrc16
 * Description   : This method appends 16-bit data to current CRC calculation
 *                 and returns new result
 *
 *END**************************************************************************/
uint32_t CRC_HAL_GetCrc16(uint32_t baseAddr, uint16_t data, bool newSeed, uint32_t seed)
{
    if (newSeed == true)
    {
        CRC_HAL_SetSeedOrDataMode(baseAddr, true);
        CRC_HAL_SetDataReg(baseAddr, seed);
        CRC_HAL_SetSeedOrDataMode(baseAddr, false);
        CRC_HAL_SetDataLReg(baseAddr, data);
        return CRC_HAL_GetCrcResult(baseAddr);
    }
    else
    {
        CRC_HAL_SetDataLReg(baseAddr, data);
        return CRC_HAL_GetCrcResult(baseAddr);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_HAL_GetCrc8
 * Description   : This method appends 8-bit data to current CRC calculation
 *                 and returns new result
 *
 *END**************************************************************************/
uint32_t CRC_HAL_GetCrc8(uint32_t baseAddr, uint8_t data, bool newSeed, uint32_t seed)
{
    if (newSeed == true)
    {
        CRC_HAL_SetSeedOrDataMode(baseAddr, true);
        CRC_HAL_SetDataReg(baseAddr, seed);
        CRC_HAL_SetSeedOrDataMode(baseAddr, false);
        CRC_HAL_SetDataLLReg(baseAddr, data);
        return CRC_HAL_GetCrcResult(baseAddr);
    }
    else
    {
        CRC_HAL_SetDataLLReg(baseAddr, data);
        return CRC_HAL_GetCrcResult(baseAddr);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_HAL_GetCrcResult
 * Description   : This method returns current result of CRC calculation
 *
 *END**************************************************************************/
uint32_t CRC_HAL_GetCrcResult(uint32_t baseAddr)
{
    uint32_t result = 0;
    crc_transpose_t transpose;
    crc_prot_width_t width;

    width = CRC_HAL_GetProtocolWidth(baseAddr);

    switch(width)
    {
    case kCrc16Bits:
        transpose = CRC_HAL_GetReadTranspose(baseAddr);

        if( (transpose == kCrcTransposeBoth) || (transpose == kCrcTransposeBytes) )
        {
            /* Return upper 16bits of CRC because of transposition in 16bit mode */
            result = CRC_HAL_GetDataHReg(baseAddr);
        }
        else
        {
            result = CRC_HAL_GetDataLReg(baseAddr);
        }
        break;
    case kCrc32Bits:
        result = CRC_HAL_GetDataReg(baseAddr);
        break;
    default:
        break;
    }
    return result;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/



