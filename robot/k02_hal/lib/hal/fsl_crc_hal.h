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

#if !defined(__FSL_CRC_HAL_H__)
#define __FSL_CRC_HAL_H__

#include <stdint.h>
#include <stdbool.h>
#include "fsl_device_registers.h"

/*! @addtogroup crc_hal*/
/*! @{*/

/*! @file*/

/*!*****************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief CRC status return codes.
 */
typedef enum _crc_status
{
    kStatus_CRC_Success         = 0U, /*!< Success. */
    kStatus_CRC_InvalidArgument = 1U, /*!< Invalid argument existed. */
    kStatus_CRC_Failed          = 2U  /*!< Execution failed. */
} crc_status_t;

/*!
 * @brief Define type of enumerating transpose modes for CRC peripheral.
 */
typedef enum _crc_transpose
{
    kCrcNoTranspose    = 0U, /*!< No transposition. @internal gui name="No Transpose" */
    kCrcTransposeBits  = 1U, /*!< Bits in bytes are transposed; bytes are not transposed. @internal gui name="Transpose Bits" */
    kCrcTransposeBoth  = 2U, /*!< Both bits in bytes and bytes are transposed. @internal gui name="Transpose Bits in Bytes and Bytes" */
    kCrcTransposeBytes = 3U  /*!< Only bytes are transposed; no bits in a byte are transposed. @internal gui name="Transpose Bytes" */
}crc_transpose_t;

/*!
 * @brief Define type of enumerating CRC protocol widths for CRC peripheral.
 */
typedef enum _crc_prot_width
{
    kCrc16Bits = 0U, /*!< 16-bit CRC protocol. @internal gui name="16 bits" */
    kCrc32Bits = 1U, /*!< 32-bit CRC protocol. @internal gui name="32 bits" */
}crc_prot_width_t;

/*!*****************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*! @name CRC related feature APIs*/

/*!
 * @brief This function initializes the module to a known state.
 *
 * @param baseAddr The CRC peripheral base address.
 */
void CRC_HAL_Init(uint32_t baseAddr);

/*!
 * @brief Returns current CRC result from data register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return Returns a 32-bit value.
 */
static inline uint32_t CRC_HAL_GetDataReg(uint32_t baseAddr)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    return HW_CRC_CRC_RD(baseAddr);
#else
    return HW_CRC_DATA_RD(baseAddr);
#endif
}

/*!
 * @brief Returns upper 16bits of current CRC result from data register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return Returns a 16-bit value.
 */
static inline uint16_t CRC_HAL_GetDataHReg(uint32_t baseAddr)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    return HW_CRC_CRCH_RD(baseAddr);
#else
    return HW_CRC_DATAH_RD(baseAddr);
#endif
}

/*!
 * @brief Returns lower 16bits of current CRC result from data register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return Returns a 16-bit value.
 */
static inline uint16_t CRC_HAL_GetDataLReg(uint32_t baseAddr)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    return HW_CRC_CRCL_RD(baseAddr);
#else
    return HW_CRC_DATAL_RD(baseAddr);
#endif
}

/*!
 * @brief Set CRC data register (4 bytes).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value New data for CRC computation. This parameter is a 32-bit value.
 */
static inline void CRC_HAL_SetDataReg(uint32_t baseAddr, uint32_t value)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    HW_CRC_CRC_WR(baseAddr, value);
#else
    HW_CRC_DATA_WR(baseAddr, value);
#endif
}

/*!
 * @brief Set CRC data register (upper 2 bytes).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value New data for CRC computation. This parameter is a 16-bit value.
 */
static inline void CRC_HAL_SetDataHReg(uint32_t baseAddr, uint16_t value)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    HW_CRC_CRCH_WR(baseAddr, value);
#else
    HW_CRC_DATAH_WR(baseAddr, value);
#endif
}

/*!
 * @brief Set CRC data register (lower 2 bytes).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value New data for CRC computation. This parameter is a 16-bit value.
 */
static inline void CRC_HAL_SetDataLReg(uint32_t baseAddr, uint16_t value)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    HW_CRC_CRCL_WR(baseAddr, value);
#else
    HW_CRC_DATAL_WR(baseAddr, value);
#endif
}

/*!
 * @brief Set CRC data register (HL byte).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value New data for CRC computation. This parameter is a 8-bit value.
 */
static inline void CRC_HAL_SetDataHLReg(uint32_t baseAddr, uint8_t value)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    HW_CRC_CRCHL_WR(baseAddr, value);
#else
    HW_CRC_DATAHL_WR(baseAddr, value);
#endif
}

/*!
 * @brief Set CRC data register (HU byte).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value New data for CRC computation. This parameter is a 8-bit value.
 */
static inline void CRC_HAL_SetDataHUReg(uint32_t baseAddr, uint8_t value)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    HW_CRC_CRCHU_WR(baseAddr, value);
#else
    HW_CRC_DATAHU_WR(baseAddr, value);
#endif
}

/*!
 * @brief Set CRC data register (LL byte).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value New data for CRC computation. This parameter is a 8-bit value.
 */
static inline void CRC_HAL_SetDataLLReg(uint32_t baseAddr, uint8_t value)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    HW_CRC_CRCLL_WR(baseAddr, value);
#else
    HW_CRC_DATALL_WR(baseAddr, value);
#endif
}

/*!
 * @brief Set CRC data register (LU byte).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value New data for CRC computation. This parameter is a 8-bit value.
 */
static inline void CRC_HAL_SetDataLUReg(uint32_t baseAddr, uint8_t value)
{
#if FSL_FEATURE_CRC_HAS_CRC_REG
    HW_CRC_CRCLU_WR(baseAddr, value);
#else
    HW_CRC_DATALU_WR(baseAddr, value);
#endif
}

/*!
 * @brief Returns polynomial register value.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return Returns a 32-bit value.
 */
static inline uint32_t CRC_HAL_GetPolyReg(uint32_t baseAddr)
{
    return HW_CRC_GPOLY_RD(baseAddr);
}

/*!
 * @brief Returns upper 16bits of polynomial register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return Returns a 16-bit value.
 */
static inline uint16_t CRC_HAL_GetPolyHReg(uint32_t baseAddr)
{
    return HW_CRC_GPOLYH_RD(baseAddr);
}

/*!
 * @brief Returns lower 16bits of polynomial register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return Returns a 16-bit value.
 */
static inline uint16_t CRC_HAL_GetPolyLReg(uint32_t baseAddr)
{
    return HW_CRC_GPOLYL_RD(baseAddr);
}

/*!
 * @brief Set polynomial register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value Polynomial value. This parameter is a 32-bit value.
 */
static inline void CRC_HAL_SetPolyReg(uint32_t baseAddr, uint32_t value)
{
    HW_CRC_GPOLY_WR(baseAddr, value);
}

/*!
 * @brief Set upper 16 bits of polynomial register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value Polynomial value. This parameter is a 16-bit value.
 */
static inline void CRC_HAL_SetPolyHReg(uint32_t baseAddr, uint16_t value)
{
    HW_CRC_GPOLYH_WR(baseAddr, value);
}

/*!
 * @brief Set lower 16 bits of polynomial register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value Polynomial value. This parameter is a 16-bit value.
 */
static inline void CRC_HAL_SetPolyLReg(uint32_t baseAddr, uint16_t value)
{
    HW_CRC_GPOLYL_WR(baseAddr, value);
}

/*!
 * @brief Returns CRC control register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return Returns a 32-bit value.
 */
static inline uint32_t CRC_HAL_GetCtrlReg(uint32_t baseAddr)
{
    return HW_CRC_CTRL_RD(baseAddr);
}

/*!
 * @brief Set CRC control register.
 *
 * @param baseAddr The CRC peripheral base address.
 * @param value Control register value. This parameter is a 32-bit value.
 */
static inline void CRC_HAL_SetCtrlReg(uint32_t baseAddr, uint32_t value)
{
    HW_CRC_CTRL_WR(baseAddr, value);
}

/*!
 * @brief Get CRC seed mode.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return CRC seed mode
 *         -true: Seed mode is enabled
 *         -false: Data mode is enabled
 */
static inline bool CRC_HAL_GetSeedOrDataMode(uint32_t baseAddr)
{
    return (bool)BR_CRC_CTRL_WAS(baseAddr);
}

/*!
 * @brief Set CRC seed mode.
 *
 * @param baseAddr The CRC peripheral base address.
 * @param enable Enable or disable seed mode.
          -true: use CRC data register for seed values
          -false: use CRC data register for data values
 */
static inline void CRC_HAL_SetSeedOrDataMode(uint32_t baseAddr, bool enable)
{
    BW_CRC_CTRL_WAS(baseAddr, enable);
}

/*!
 * @brief Get CRC transpose type for writes.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return CRC input transpose type for writes.
 */
static inline crc_transpose_t CRC_HAL_GetWriteTranspose(uint32_t baseAddr)
{
    return (crc_transpose_t)BR_CRC_CTRL_TOT(baseAddr);
}

/*!
 * @brief Set CRC transpose type for writes.
 *
 * @param baseAddr The CRC peripheral base address.
 * @param transp The CRC input transpose type.
 */
static inline void CRC_HAL_SetWriteTranspose(uint32_t baseAddr, crc_transpose_t transp)
{
    BW_CRC_CTRL_TOT(baseAddr, transp);
}

/*!
 * @brief Get CRC transpose type for reads.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return CRC output transpose type.
 */
static inline crc_transpose_t CRC_HAL_GetReadTranspose(uint32_t baseAddr)
{
    return (crc_transpose_t)BR_CRC_CTRL_TOTR(baseAddr);
}

/*!
 * @brief Set CRC transpose type for reads.
 *
 * @param baseAddr The CRC peripheral base address.
 * @param transp The CRC output transpose type.
 */
static inline void CRC_HAL_SetReadTranspose(uint32_t baseAddr, crc_transpose_t transp)
{
    BW_CRC_CTRL_TOTR(baseAddr, transp);
}

/*!
 * @brief Get CRC XOR mode.
 *
 * Some CRC protocols require the final checksum to be XORed with 0xFFFFFFFF
 * or 0xFFFF. XOR mode enables "on the fly" complementing of read data.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return CRC XOR mode
 *         -true: XOR mode is enabled
 *         -false: XOR mode is disabled
 */
static inline bool CRC_HAL_GetXorMode(uint32_t baseAddr)
{
    return (bool)BR_CRC_CTRL_FXOR(baseAddr);
}

/*!
 * @brief Set CRC XOR mode.
 *
 * Some CRC protocols require the final checksum to be XORed with 0xFFFFFFFF
 * or 0xFFFF. XOR mode enables "on the fly" complementing of read data.
 *
 * @param baseAddr The CRC peripheral base address.
 * @param enable Enable or disable XOR mode.
 */
static inline void CRC_HAL_SetXorMode(uint32_t baseAddr, bool enable)
{
    BW_CRC_CTRL_FXOR(baseAddr, enable);
}

/*!
 * @brief Get CRC protocol width.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return CRC protocol width
 *         -kCrc16Bits: 16-bit CRC protocol
 *         -kCrc32Bits: 32-bit CRC protocol
 */
static inline crc_prot_width_t CRC_HAL_GetProtocolWidth(uint32_t baseAddr)
{
    return (crc_prot_width_t)BR_CRC_CTRL_TCRC(baseAddr);
}

/*!
 * @brief Set CRC protocol width.
 *
 * @param baseAddr The CRC peripheral base address.
 * @param width The CRC protocol width
 *         -kCrc16Bits: 16-bit CRC protocol
 *         -kCrc32Bits: 32-bit CRC protocol
 */
static inline void CRC_HAL_SetProtocolWidth(uint32_t baseAddr, crc_prot_width_t width)
{
    BW_CRC_CTRL_TCRC(baseAddr, width);
}

/*!
 * @brief CRC_HAL_GetCrc32
 *
 * This method appends 32-bit data to current CRC calculation
 * and returns new result. If newSeed is true, seed set and
 * result calculated from seed new value (new crc calculation).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param data input data for crc calculation
 * @param newSeed Sets new crc calculation.
 *        - true: New seed set and used for new calculation.
 *        - false: seed argument ignored, continues old calculation.
 * @param seed New seed if newSeed is true, else ignored
 * @return new crc result.
 */
uint32_t CRC_HAL_GetCrc32(uint32_t baseAddr, uint32_t data, bool newSeed, uint32_t seed);

/*!
 * @brief CRC_HAL_GetCrc16
 *
 * This method appends 16-bit data to current CRC calculation
 * and returns new result. If newSeed is true, seed set and
 * result calculated from seed new value (new crc calculation).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param data input data for crc calculation
 * @param newSeed Sets new crc calculation.
 *        - true: New seed set and used for new calculation.
 *        - false: seed argument ignored, continues old calculation.
 * @param seed New seed if newSeed is true, else ignored
 * @return new crc result.
 */
uint32_t CRC_HAL_GetCrc16(uint32_t baseAddr, uint16_t data, bool newSeed, uint32_t seed);

/*!
 * @brief CRC_HAL_GetCrc8
 *
 * This method appends 8-bit data to current CRC calculation
 * and returns new result. If newSeed is true, seed set and
 * result calculated from seed new value (new crc calculation).
 *
 * @param baseAddr The CRC peripheral base address.
 * @param data input data for crc calculation
 * @param newSeed Sets new crc calculation.
 *        - true: New seed set and used for new calculation.
 *        - false: seed argument ignored, continues old calculation.
 * @param seed New seed if newSeed is true, else ignored
 * @return new crc result.
 */
uint32_t CRC_HAL_GetCrc8(uint32_t baseAddr, uint8_t data, bool newSeed, uint32_t seed);

/*!
 * @brief CRC_HAL_GetCrcResult
 *
 * This method returns current result of CRC calculation.
 * The result is ReadTranspose dependent.
 *
 * @param baseAddr The CRC peripheral base address.
 * @return result of CRC calculation.
 */
uint32_t CRC_HAL_GetCrcResult(uint32_t baseAddr);

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*! @}*/

#endif /* __FSL_CRC_HAL_H__*/
