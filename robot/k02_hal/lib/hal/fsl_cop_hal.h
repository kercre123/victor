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
#ifndef __FSL_COP_HAL_H__
#define __FSL_COP_HAL_H__

#include "fsl_device_registers.h"
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/*! 
 * @addtogroup cop_hal
 * @{
 */

/*******************************************************************************
 * Definitions
 *******************************************************************************/

#define COPWATCHDOG_DISABLE_VALUE            0U
#define SERVICE_COPWDOG_FIRST_VALUE          0x55U
#define SERVICE_COPWDOG_SECOND_VALUE         0xaaU
#define RESET_COPWDOG_VALUE                  0U

/*! @brief COP clock source selection.*/
typedef enum _cop_clock_source {
    kCopLpoClock,                            /*!< LPO clock,1K HZ.*/
#if FSL_FEATURE_COP_HAS_MORE_CLKSRC
    kCopMcgIrClock,                          /*!< MCG IRC Clock   */
    kCopOscErClock,                          /*!< OSCER Clock     */
#endif
    kCopBusClock                             /*!< BUS clock       */
}cop_clock_source_t;

/*! @brief Define the value of the COP timeout cycles */
typedef enum _cop_timeout_cycles {
    kCopTimeout_short_2to5_or_long_2to13  = 1U,   /*!< 2 to 5 clock cycles when clock source is LPO or in short timeout mode otherwise 2 to 13 clock cycles */
    kCopTimeout_short_2to8_or_long_2to16  = 2U,   /*!< 2 to 8 clock cycles when clock source is LPO or in short timeout mode otherwise 2 to 16 clock cycles */
    kCopTimeout_short_2to10_or_long_2to18 = 3U    /*!< 2 to 10 clock cycles when clock source is LPO or in short timeout mode otherwise 2 to 18 clock cycles */
}cop_timeout_cycles_t;

#if FSL_FEATURE_COP_HAS_LONGTIME_MODE
/*! @breif Define the COP's timeout mode */
typedef enum _cop_timeout_mode{
    kCopShortTimeoutMode = 0U,      /*!< COP selects long timeout  */   
    kCopLongTimeoutMode  = 1U       /*!< COP selects short timeout */
}cop_timeout_mode_t;
#endif

/*! @brief Define COP common configure */
typedef union _cop_common_config{
  uint32_t U;
  struct CommonConfig{
    uint32_t copWindowEnable    : 1;       /*!< Set COP watchdog run mode---Window mode or Normal mode >*/
#if FSL_FEATURE_COP_HAS_LONGTIME_MODE
    uint32_t copTimeoutMode     : 1;       /*!< Set COP watchdog timeout mode---Long timeout or Short tomeout >*/
#else
    uint32_t copClockSelect     : 1;       /*!< Set COP watchdog clock source >*/
#endif
    uint32_t copTimeout         : 2;       /*!< Set COP watchdog timeout value >*/
#if FSL_FEATURE_COP_HAS_LONGTIME_MODE    
    uint32_t copStopModeEnable  : 1;       /*!< Set COP enable or disable in STOP mode >*/ 
    uint32_t copDebugModeEnable : 1;       /*!< Set COP enable or disable in DEBUG mode >*/
    uint32_t copClockSource     : 2;       /*!< Set COP watchdog clock source >*/
#else 
    uint32_t reserved0          : 4;
#endif
    uint32_t reserved1          : 24;
  }commonConfig;
}cop_common_config_t;

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*! 
 * @name COP HAL.
 * @{
 */
  
  
/*!
 * @brief config the COP watchdog.
 *
 * The COP Control register is write once after reset.
 *
 * @param baseAddr The COP peripheral base address
 * @param copConfiguration configure COP control register
 */
static inline void COP_HAL_SetCommonConfiguration(uint32_t baseAddr, cop_common_config_t copConfiguration)
{
    HW_SIM_COPC_WR(baseAddr, copConfiguration.U);
}
  
/*!
 * @brief disable the COP watchdog.
 *
 * This function is used to disable the COP watchdog and 
 * should be called after reset if your application does not need COP watchdog.
 *
 * @param baseAddr The COP peripheral base address
 */
static inline void COP_HAL_Disable(uint32_t baseAddr)
{
    BW_SIM_COPC_COPT(baseAddr, COPWATCHDOG_DISABLE_VALUE);
}

/*!
 * @brief Determines whether the COP is enabled.
 *
 * This function is used to check whether the COP is running.
 *
 * @param baseAddr The COP peripheral base address
 * @retval true COP is enabled
 * @retval false COP is disabled
 */
static inline bool COP_HAL_IsEnabled(uint32_t baseAddr)
{
    return ((bool)BR_SIM_COPC_COPT(baseAddr));
}

/*!
 * @brief Gets the COP clock source.
 *
 * This function is used to get the COP clock source.
 *
 * @param baseAddr The COP peripheral base address
 * @retval clockSource COP clock source, see cop_clock_source_t
 */
static inline cop_clock_source_t COP_HAL_GetClockSource(uint32_t baseAddr)
{
#if FSL_FEATURE_COP_HAS_LONGTIME_MODE    
   return ((cop_clock_source_t)BR_SIM_COPC_COPCLKSEL(baseAddr));
#else
   return ((cop_clock_source_t)BR_SIM_COPC_COPCLKS(baseAddr));
#endif
}

/*!
 * @brief Returns timeout of COP watchdog .
 *
 * @param baseAddr The COP peripheral base address
 * @return COP timeout cycles
 */
static inline cop_timeout_cycles_t COP_HAL_GetTimeoutCycles(uint32_t baseAddr)
{
    return ((cop_timeout_cycles_t)BR_SIM_COPC_COPT(baseAddr));
}

/*!
 * @brief get the COP watchdog run mode---window mode or normal window.
 *
 * @param baseAddr The COP peripheral base address
 * @retval 0 COP watchdog run in normal mode
 * @retval 1 COP watchdog run in window mode
 */
static inline bool COP_HAL_GetWindowModeCmd(uint32_t baseAddr)
{
    return ((bool)BR_SIM_COPC_COPW(baseAddr));
}

#if FSL_FEATURE_COP_HAS_LONGTIME_MODE
/*!
 * @brief Get the COP watchdog timeout mode.
 *
 * @param baseAddr The COP peripheral base address
 * @retval true COP watchdog in long timeout mode
 * @retval false COP watchdog in  short timeout mode
 */
static inline cop_timeout_mode_t COP_HAL_GetTimeoutMode(uint32_t baseAddr)
{
    return ((cop_timeout_mode_t)BR_SIM_COPC_COPCLKS(baseAddr));
}
#endif


#if FSL_FEATURE_COP_HAS_DEBUG_ENABLE
/*!
 * @brief Get the COP watchdog running state in DEBUG mode.
 *
 * @param baseAddr The COP peripheral base address
 * @retval true COP watchdog enabled in debug mode
 * @retval false COP watchdog disabled in debug mode
 */
static inline bool COP_HAL_GetEnabledInDebugModeCmd(uint32_t baseAddr)
{
    return(BR_SIM_COPC_COPDBGEN(baseAddr));
}
#endif

#if FSL_FEATURE_COP_HAS_STOP_ENABLE
/*!
 * @brief Get the COP watchdog running state in STOP mode.
 *
 * @param baseAddr The COP peripheral base address
 * @retval true COP watchdog enabled in stop mode
 * @retval false COP watchdog disabled in stop mode
 */
static inline bool COP_HAL_GetEnabledInStopModeCmd(uint32_t baseAddr)
{
    return (BR_SIM_COPC_COPSTPEN(baseAddr));
}
#endif

/*!
 * @brief Servicing COP watchdog.
 *
 * This function reset COP timeout by write 0x55 then 0xAA, 
 * write any other value will generate a system reset.
 * The writing operations should be atomic.
 * @param baseAddr The COP peripheral base address
 */
static inline void COP_HAL_Refresh(uint32_t baseAddr)
{
    HW_SIM_SRVCOP_WR(baseAddr, SERVICE_COPWDOG_FIRST_VALUE);
    HW_SIM_SRVCOP_WR(baseAddr, SERVICE_COPWDOG_SECOND_VALUE);
}

/*!
 * @brief Reset system.
 *
 * This function reset system
 * @param baseAddr The COP peripheral base address
 */
static inline void COP_HAL_ResetSystem(uint32_t baseAddr)
{
    HW_SIM_SRVCOP_WR(baseAddr, RESET_COPWDOG_VALUE);
}

/*!
 * @brief Restores the COP module to reset value.
 *
 * This function restores the COP module to reset value.
 *
 * @param baseAddr The COP peripheral base address
 */
void COP_HAL_Init(uint32_t baseAddr);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __FSL_COP_HAL_H__*/
/*******************************************************************************
 * EOF
 *******************************************************************************/
