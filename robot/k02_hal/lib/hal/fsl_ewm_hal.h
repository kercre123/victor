/*
 * Copyright (c) 2013 - 2014, Freescale Semiconductor, Inc.
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
#ifndef __FSL_EWM_HAL_H__
#define __FSL_EWM_HAL_H__

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "fsl_device_registers.h"

/*! 
 * @addtogroup ewm_hal
 * @{
 */

/*******************************************************************************
 * Definitions
 *******************************************************************************/


#define EWM_FIRST_SERVICE_VALUE  (0xB4U)
#define EWM_SECOND_SERVICE_VALUE (0x2CU)

/*! @brief EWM_in's assertion state.*/
typedef enum _ewm_in_assertion_state{
    kEWMLogicZeroAssert = 0,         /*!< assert state of EWM_in signal is logic zero */
    kEWMLogicOneAssert  = 1          /*!< assert state of EWM_in signal is logic one */
}ewm_in_assertion_state_t;

/*! @brief ewm common config structure.*/
typedef union _ewm_common_config{
    uint8_t U;
    struct CommonConfig{
      uint8_t ewmEnable:          1;
      uint8_t ewmInAssertState:   1;
      uint8_t ewmInputEnable:     1;
      uint8_t ewmIntEnable:       1;
      uint8_t reserved:           4;
    }commonConfig;
}ewm_common_config_t;

/*! @brief ewm status return codes.*/
typedef enum _EWM_status {
    kStatus_EWM_Success         = 0x0U, /*!< Succeed. */
    kStatus_EWM_NotInitlialized = 0x1U, /*!< EWM is not initialized yet. */
    kStatus_EWM_NullArgument    = 0x2U, /*!< Argument is NULL.*/
}ewm_status_t;

/*******************************************************************************
 ** Variables
 *******************************************************************************/

/*******************************************************************************
 * API
 *******************************************************************************/

/*!
 * @brief Config EWM control register.
 * 
 * This function configures EWM control register,
 * EWM enable bitfeild, EWM ASSIN bitfeild and EWM INPUT enable bitfeild are WRITE ONCE, one more write will cause bus fault.
 *
 * @param baseAddr The EWM peripheral base address
 * @param commonConfig config EWM CTRL register
 */
static inline void EWM_HAL_SetCommonConfig(uint32_t baseAddr, ewm_common_config_t commonConfig)
{
    HW_EWM_CTRL_WR(baseAddr, commonConfig.U);
}

/*!
 * @brief Checks whether the EWM is enabled.
 * 
 * This function checks whether the EWM is enabled.
 *
 * @param baseAddr The EWM peripheral base address
 * @retval false means EWM is disabled
 * @retval true means WODG is enabled
 */
static inline bool EWM_HAL_IsEnabled(uint32_t baseAddr)
{
    return ((bool)BR_EWM_CTRL_EWMEN(baseAddr));
}

/*!
 * @brief Checks EWM_in assertion state.
 * 
 * This function checks EWM_in assertion state.
 *
 * @param baseAddr The EWM peripheral base address
 * @retval kEWMLogicZeroAssert means assert state of EWM_in signal is logic zero
 * @retval kEWMLogicOneAssert means assert state of EWM_in signal is logic one
 */
static inline ewm_in_assertion_state_t EWM_HAL_GetAssertionStateMode(uint32_t baseAddr)
{
    return ((ewm_in_assertion_state_t)BR_EWM_CTRL_ASSIN(baseAddr));
}

/*!
 * @brief Checks if EWM_in input is enabled.
 * 
 * This function checks whether the EWM_in is enabled.
 *
 * @param baseAddr The EWM peripheral base address
 * @retval true means EWM_in input is enable
 * @retval false means EWM_in input is disable
 */
static inline  bool EWM_HAL_GetInputCmd(uint32_t baseAddr)
{
    return ((bool)BR_EWM_CTRL_INEN(baseAddr));
}

/*!
 * @brief Checks if EWM interrupt is enabled.
 * 
 * This function checks whether the EWM interrupt is enabled.
 *
 * @param baseAddr The EWM peripheral base address
 * @retval true means EWM_in interrupt is enable
 * @retval false means EWM_in interrupt is disable
 */
static inline bool EWM_HAL_GetIntCmd(uint32_t baseAddr)
{
   return ((bool)BR_EWM_CTRL_INTEN(baseAddr));
}

/*!
 * @brief Enable/Disable EWM interrupt.
 * 
 * This function sets EWM enable/disable.
 *
 * @param baseAddr The EWM peripheral base address
 * @param enable Set EWM interrupt enable/disable
 */
static inline void EWM_HAL_SetIntCmd(uint32_t baseAddr, bool enable)
{
   BW_EWM_CTRL_INTEN(baseAddr, enable);
}

/*!
 * @brief Set EWM compare low register value.
 * 
 * This function sets EWM compare low register value and defines the minimum cycles to service EWM,
 * when counter value is greater than or equal to ewm compare low register value, refresh EWM can be successful,
 * and this register is write once, one more write will cause bus fault.
 *
 * @param baseAddr The EWM peripheral base address
 * @param minServiceCycles The EWM compare low register value 
 */
static inline void EWM_HAL_SetCmpLowRegValue(uint32_t baseAddr, uint8_t minServiceCycles)
{
    BW_EWM_CMPL_COMPAREL(baseAddr, minServiceCycles);
}

/*!
 * @brief Return EWM compare low register value.
 * 
 * This function return compare low register value.
 *
 * @param baseAddr The EWM peripheral base address
 * @retval compare low register value
*/
static inline uint8_t EWM_HAL_GetCmpLowRegValue(uint32_t baseAddr)
{
    return (BR_EWM_CMPL_COMPAREL(baseAddr));
}

/*!
 * @brief Set EWM compare high register value.
 * 
 * This function sets EWM compare high register value and defines the maximum cycles to service EWM,
 * when counter value is less than or equal to ewm compare high register value, refresh EWM can be successful,
 * the compare high register value must be greater than compare low register value,
 * and this register is write once, one more write will cause bus fault.
 *
 * @param baseAddr The EWM peripheral base address
 * @param maxServiceCycles The EWM compare low register value 
 */
static inline void EWM_HAL_SetCmpHighRegValue(uint32_t baseAddr, uint8_t maxServiceCycles)
{
    BW_EWM_CMPH_COMPAREH(baseAddr, maxServiceCycles);
}

/*!
 * @brief Return EWM compare high register value.
 * 
 * This function return compare high register value, the maximum value is 0xfe, otherwise EWM counter never expires.
 *
 * @param baseAddr The EWM peripheral base address
 * @retval compare high register value
*/
static inline uint8_t EWM_HAL_GetCmpHighRegValue(uint32_t baseAddr)
{
    return (BR_EWM_CMPH_COMPAREH(baseAddr));
}

#if FSL_FEATURE_EWM_HAS_PRESCALER
/*!
 * @brief Return EWM prescaler register value.
 * 
 * This function returns prescaler register value.
 *
 * @param baseAddr The EWM peripheral base address
 * @retval prescaler register value
*/
static inline uint8_t EWM_HAL_GetPrescalerValue(uint32_t baseAddr)
{
    return (BR_EWM_CLKPRESCALER_CLK_DIV(baseAddr));
}
#endif

#if FSL_FEATURE_EWM_HAS_PRESCALER
/*!
 * @brief Set EWM prescaler register value.
 * 
 * This function Set prescaler register value, and this register is writen once.
 *
 * @param baseAddr The EWM peripheral base address
 * @param prescalerValue The prescaler register value
*/
static inline void EWM_HAL_SetPrescalerValue(uint32_t baseAddr,uint8_t prescalerValue)
{
    BW_EWM_CLKPRESCALER_CLK_DIV(baseAddr, prescalerValue);
}
#endif

/*!
 * @brief Service EWM.
 * 
 * This function reset EWM counter to zero and 
 * the period of writing the frist value and the second value should be within 15 bus cycles.
 *
 * @param baseAddr The EWM peripheral base address
*/
static inline void EWM_HAL_Refresh(uint32_t baseAddr)
{
    BW_EWM_SERV_SERVICE(baseAddr, (uint8_t)EWM_FIRST_SERVICE_VALUE);
    BW_EWM_SERV_SERVICE(baseAddr, (uint8_t)EWM_SECOND_SERVICE_VALUE);
}

/*!
 * @brief Restores the EWM module to reset value.
 *
 * This function restores the EWM module to reset value.
 *
 * @param baseAddr The EWM peripheral base address
 */
void EWM_HAL_Init(uint32_t baseAddr);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __FSL_EWM_HAL_H__*/
/*******************************************************************************
 * EOF
 *******************************************************************************/

