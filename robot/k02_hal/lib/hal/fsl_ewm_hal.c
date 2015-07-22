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

#include "fsl_ewm_hal.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*******************************************************************************
 * Variables
 *******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
 
/*FUNCTION**********************************************************************
 *
 * Function Name : EWM_HAL_Init
 * Description   : Initialize EWM peripheral to reset state.
 *
 *END**************************************************************************/
void EWM_HAL_Init(uint32_t baseAddr)
{
    ewm_common_config_t ewmCommonConfig;
    
    ewmCommonConfig.commonConfig.ewmEnable          = (uint8_t)false;
    ewmCommonConfig.commonConfig.ewmInAssertState   = (uint8_t)kEWMLogicZeroAssert;
    ewmCommonConfig.commonConfig.ewmInputEnable     = (uint8_t)false;
    ewmCommonConfig.commonConfig.ewmIntEnable       = (uint8_t)false;
    ewmCommonConfig.commonConfig.reserved           = (uint8_t)0;
#if FSL_FEATURE_EWM_HAS_PRESCALER
    EWM_HAL_SetPrescalerValue(baseAddr, (uint8_t)0);
#endif
    EWM_HAL_SetCmpHighRegValue(baseAddr, (uint8_t)0x00); /*!< this value should be set prior to enable ewm to avoid rumaway code changes this register */
    EWM_HAL_SetCmpHighRegValue(baseAddr, (uint8_t)0xff); /*!< this value should be set prior to enable ewm to avoid rumaway code changes this register */
    
    EWM_HAL_SetCommonConfig(baseAddr, ewmCommonConfig);
}

/*******************************************************************************
 * EOF
 *******************************************************************************/

