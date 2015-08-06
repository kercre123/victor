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

#include "fsl_cop_hal.h"

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
 * Function Name : COP_HAL_Init
 * Description   : Initialize COP peripheral to reset state.
 *
 *END**************************************************************************/ 
void COP_HAL_Init(uint32_t baseAddr)
{
    cop_common_config_t copCommonConfig;
    
    copCommonConfig.commonConfig.copWindowEnable    = (uint8_t)false;
#if FSL_FEATURE_COP_HAS_LONGTIME_MODE    
    copCommonConfig.commonConfig.copTimeoutMode     = kCopShortTimeoutMode;
    copCommonConfig.commonConfig.copStopModeEnable  = (uint8_t)false;
    copCommonConfig.commonConfig.copDebugModeEnable = (uint8_t)false;
    copCommonConfig.commonConfig.copClockSource     = kCopLpoClock;
#else
    copCommonConfig.commonConfig.copClockSelect     = kCopLpoClock;
    copCommonConfig.commonConfig.reserved0          = 0;
#endif
    copCommonConfig.commonConfig.copTimeout         = kCopTimeout_short_2to10_or_long_2to18;
    copCommonConfig.commonConfig.reserved1          = 0;
    
    COP_HAL_SetCommonConfiguration(baseAddr, copCommonConfig);
}

/*******************************************************************************
 * EOF
 *******************************************************************************/
