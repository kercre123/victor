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
#ifndef __FSL_RNGA_HAL_H__
#define __FSL_RNGA_HAL_H__

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "fsl_device_registers.h"

/*! 
 * @addtogroup rnga_hal
 * @{
 */

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*! @brief the max cpu clock cycles rnga module used to get a new random data */
#define MAX_COUNT  4096

/*! @brief RNGA working mode */
typedef enum _rnga_mode
{
    kRNGAModeNormal = 0U, /*!< Normal Mode. */
    kRNGAModeSleep = 1U,  /*!< Sleep Mode. */
} rnga_mode_t;

/*! @brief Defines the value of output register level */
typedef enum _rnga_output_reg_level
{
    kRNGAOutputRegLevelNowords = 0U, /*!< output register no words. */
    kRNGAOutputRegLevelOneword = 1U, /*!< output register one word. */
} rnga_output_reg_level_t;

/*!
 * @brief Status structure for RNGA
 *
 * This structure holds the return code of RNGA module.
 */

typedef enum _rnga_status
{
    kStatus_RNGA_Success = 0U, /*!< Success */
    kStatus_RNGA_InvalidArgument = 1U, /*!< Invalid argument */
    kStatus_RNGA_Underflow = 2U, /*!< Underflow */
    kStatus_RNGA_Timeout = 3U, /*!< Timeout */
} rnga_status_t;

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*! 
 * @name RNGA HAL.
 * @{
 */


/*!
 * @brief Initializes the RNGA module.
 *
 * This function initializes the RNGA to a default state.
 *
 * @param baseAddr, RNGA base address
 */
static inline void RNGA_HAL_Init(uint32_t baseAddr)
{
    HW_RNG_CR_WR(baseAddr, 0);
}


/*!
 * @brief Enables the RNGA module.
 *
 * This function enables the RNGA random data generation and loading.
 *
 * @param baseAddr, RNGA base address
 */
static inline void RNGA_HAL_Enable(uint32_t baseAddr)
{
    BW_RNG_CR_GO(baseAddr, 1);
}


/*!
 * @brief Disables the RNGA module.
 * 
 * This function disables the RNGA module.
 *
 * @param baseAddr, RNGA base address
*/
static inline void RNGA_HAL_Disable(uint32_t baseAddr)
{
    BW_RNG_CR_GO(baseAddr, 0);
}


/*!
 * @brief Sets the RNGA high assurance.
 * 
 * This function sets the RNGA high assurance(notification of security
 * violations.
 *
 * @param baseAddr, RNGA base address
 * @param enable,   0 means notification of security violations disabled.
 *                  1 means notification of security violations enabled.
*/
static inline void RNGA_HAL_SetHighAssuranceCmd(uint32_t baseAddr, bool enable)
{
    BW_RNG_CR_HA(baseAddr, enable);
}


/*!
 * @brief Sets the RNGA interrupt mask.
 * 
 * This function sets the RNGA error interrupt mask.
 *
 * @param baseAddr, RNGA base address
 * @param enable,   0 means unmask RNGA interrupt.
 *                  1 means mask RNGA interrupt.
*/
static inline void RNGA_HAL_SetIntMaskCmd(uint32_t baseAddr, bool enable)
{
    BW_RNG_CR_INTM(baseAddr, enable);
}


/*!
 * @brief Clears the RNGA interrupt.
 * 
 * This function clears the RNGA interrupt.
 *
 * @param baseAddr, RNGA base address
 * @param enable,   0 means do not clear the interrupt.
 *                  1 means clear the interrupt.
*/
static inline void RNGA_HAL_ClearIntFlag(uint32_t baseAddr, bool enable)
{
    BW_RNG_CR_CLRI(baseAddr, enable);
}


/*!
 * @brief Sets the RNGA in sleep mode or normal mode.
 * 
 * This function specifies whether the RNGA is in sleep mode or normal mode.
 *
 * @param baseAddr, RNGA base address
 * @param mode,  kRNGAModeNormal means set RNGA in normal mode.
 *               kRNGAModeSleep means set RNGA in sleep mode.
*/
static inline void RNGA_HAL_SetWorkModeCmd(uint32_t baseAddr, rnga_mode_t mode)
{
    BW_RNG_CR_SLP(baseAddr, (uint32_t)mode);
}


/*!
 * @brief Gets the output register size.
 *
 * This function gets the size of the output register as 
 * 32-bit random data words it can hold.
 *
 * @param baseAddr, RNGA base address
 * @return 1 means one word(this value is fixed).
 */
static inline uint8_t RNGA_HAL_GetOutputRegSize(uint32_t baseAddr)
{
    return BR_RNG_SR_OREG_SIZE(baseAddr);
}


/*!
 * @brief Gets the output register level.
 *
 * This function gets the number of random-data words that are in OR 
 * [RANDOUT], which indicates if OR is valid.
 *
 * @param baseAddr, RNGA base address
 * @return 0 means no words(empty), 1 means one word(valid).
 */
static inline rnga_output_reg_level_t RNGA_HAL_GetOutputRegLevel(uint32_t baseAddr)
{
    return (rnga_output_reg_level_t)(BR_RNG_SR_OREG_LVL(baseAddr));
}


/*!
 * @brief Gets the RNGA working mode.
 *
 * This function checks whether the RNGA works in sleep mode or normal mode. 
 *
 * @param baseAddr, RNGA base address
 * @return  Kmode_RNGA_Normal means in normal mode
 *          Kmode_RNGA_Sleep means in sleep mode
*/
static inline rnga_mode_t RNGA_HAL_GetWorkMode(uint32_t baseAddr)
{
    return (rnga_mode_t)BR_RNG_SR_SLP(baseAddr);
}


/*!
 * @brief Gets the RNGA status whether an error interrupt has occurred. 
 *
 * This function gets the RNGA status whether an OR underflow
 * condition has occurred since the error interrupt was last cleared  or the RNGA was
 * reset.
 *
 * @param baseAddr, RNGA base address
 * @return 0 means no underflow, 1 means underflow
*/
static inline bool RNGA_HAL_GetErrorIntCmd(uint32_t baseAddr)
{
    return (BR_RNG_SR_ERRI(baseAddr));
}


/*!
 * @brief Gets the RNGA status whether an output register underflow has occurred. 
 *
 * This function gets the RNGA status whether an OR underflow
 * condition has occurred since the register (SR) was last read or the RNGA was
 * reset.
 *
 * @param baseAddr, RNGA base address
 * @return 0 means no underflow, 1 means underflow
*/
static inline bool RNGA_HAL_GetOutputRegUnderflowCmd(uint32_t baseAddr)
{
    return (BR_RNG_SR_ORU(baseAddr));
}


/*!
 * @brief Gets the  most recent RNGA read status. 
 *
 * This function gets the RNGA status whether the most recent read of
 * OR[RANDOUT] causes an OR underflow condition.
 *
 * @param baseAddr, RNGA base address
 * @return 0 means no underflow, 1 means underflow
*/
static inline bool RNGA_HAL_GetLastReadStatusCmd(uint32_t baseAddr)
{
    return (BR_RNG_SR_LRS(baseAddr));
}


/*!
 * @brief Gets the RNGA status whether a security violation has occurred. 
 *
 * This function gets the RNGA status whether a security violation has
 * occurred when high assurance is enabled.
 *
 * @param baseAddr, RNGA base address
 * @return 0 means no security violation, 1 means security violation
*/
static inline bool RNGA_HAL_GetSecurityViolationCmd(uint32_t baseAddr)
{
    return (BR_RNG_SR_SECV(baseAddr));
}


/*!
 * @brief Gets a random data from the RNGA. 
 *
 * This function gets a random data from RNGA.
 *
 * @param baseAddr, RNGA base address
 * @return random data obtained
*/
static inline uint32_t RNGA_HAL_ReadRandomData(uint32_t baseAddr)
{
    return (BR_RNG_OR_RANDOUT(baseAddr));
}


/*!
 * @brief Get random data.
 *
 * This function is used to get a random data from RNGA
 *
 * @param baseAddr, RNGA base address
 * @param data, pointer address used to store random data
 * @return one random data
 */
rnga_status_t RNGA_HAL_GetRandomData(uint32_t baseAddr, uint32_t *data);


/*!
 * @brief Inputs an entropy value used to seed the RNGA. 
 *
 * This function specifies an entropy value that RNGA uses with
 * its ring oscillations to seed its pseudorandom algorithm.
 *
 * @param baseAddr, RNGA base address
 *        data, external entropy value
*/
static inline void RNGA_HAL_WriteSeed(uint32_t baseAddr, uint32_t data)
{
    BW_RNG_ER_EXT_ENT(baseAddr, data);
}

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __FSL_RNGA_HAL_H__*/
/*******************************************************************************
 * EOF
 *******************************************************************************/

