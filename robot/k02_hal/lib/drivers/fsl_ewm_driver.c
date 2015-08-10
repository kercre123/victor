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

#include "fsl_ewm_driver.h"
#include "fsl_interrupt_manager.h"
#include "fsl_clock_manager.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 *******************************************************************************/

/*FUNCTION****************************************************************
 *
 * Function Name : EWM_DRV_Init
 * Description   : Initialize EWM
 * This function is used to initialize the EWM, after called, the EWM 
 * will run immediately according to the configure.
 *
 *END*********************************************************************/
ewm_status_t EWM_DRV_Init(uint32_t instance, const ewm_user_config_t* userConfigPtr)
{
    assert(userConfigPtr&&(instance < HW_EWM_INSTANCE_COUNT));
    
    ewm_common_config_t ewmCommonConfig;
    uint32_t baseAddr;
    
    baseAddr = g_ewmBaseAddr[instance];
    ewmCommonConfig.commonConfig.ewmEnable          = userConfigPtr->ewmEnable;
    ewmCommonConfig.commonConfig.ewmInAssertState   = userConfigPtr->ewmInAssertionState;
    ewmCommonConfig.commonConfig.ewmInputEnable     = userConfigPtr->ewmInputEnable;
    ewmCommonConfig.commonConfig.ewmIntEnable       = userConfigPtr->ewmIntEnable;
    ewmCommonConfig.commonConfig.reserved           = 0;   
    if(userConfigPtr->ewmIntEnable)
    {
        INT_SYS_EnableIRQ(g_ewmIrqId[instance]);    /*!< Enable EWM interrupt in NVIC level */
    }
    else
    {
        INT_SYS_DisableIRQ(g_ewmIrqId[instance]);   /*!< Disable EWM interrupt in NVIC level */
    }
    CLOCK_SYS_EnableEwmClock(instance);             /*!< Enable ewm clock */
    
    EWM_HAL_SetCmpLowRegValue(baseAddr, userConfigPtr->ewmCmpLowValue);
    
    EWM_HAL_SetCmpHighRegValue(baseAddr, userConfigPtr->ewmCmpHighValue);
    
#if FSL_FEATURE_EWM_HAS_PRESCALER
    EWM_HAL_SetPrescalerValue(baseAddr, userConfigPtr->ewmPrescalerValue);
#endif
    
    EWM_HAL_SetCommonConfig(baseAddr, ewmCommonConfig);
    
    return kStatus_EWM_Success;
}

/*FUNCTION****************************************************************
 *
 * Function Name : EWM_DRV_Deinit
 * Description   : Shutdown EWM clock
 * This function is used to shutdown the EWM.
 *
 *END*********************************************************************/
ewm_status_t EWM_DRV_Deinit(uint32_t instance)
{
    assert(instance < HW_EWM_INSTANCE_COUNT);
  
    uint32_t baseAddr = g_ewmBaseAddr[instance];
    
    EWM_HAL_SetIntCmd(baseAddr, (bool)0);
    
    CLOCK_SYS_DisableEwmClock(instance);
    
    return kStatus_EWM_Success;
}

/*FUNCTION****************************************************************
 *
 * Function Name : EWM_DRV_IsRunning
 * Description   : Get EWM running status
 * This function is used to get the EWM running status.
 *
 *END*********************************************************************/
bool EWM_DRV_IsRunning(uint32_t instance)
{
    assert(instance < HW_EWM_INSTANCE_COUNT);
  
    uint32_t baseAddr = g_ewmBaseAddr[instance]; 
  
    return EWM_HAL_IsEnabled(baseAddr);
}

/*FUNCTION****************************************************************
 *
 * Function Name : EWM_DRV_Refresh
 * Description   : Refresh EWM counter.
 * This function is used to feed the EWM, it will set the EWM timer count to zero and 
 * should be called before EWM timer is timeout, otherwise a EWM_out output signal will assert.
 *
  *END*********************************************************************/
void EWM_DRV_Refresh(uint32_t instance)
{
    assert(instance < HW_EWM_INSTANCE_COUNT);
    
    uint32_t baseAddr;
    
    baseAddr = g_ewmBaseAddr[instance];
  
    INT_SYS_DisableIRQGlobal();
    
    EWM_HAL_Refresh(baseAddr);

    INT_SYS_EnableIRQGlobal();
}

/*FUNCTION*********************************************************************
 *
 * Function Name : EWM_DRV_GetIntCmd
 * Description   : Return EWM interrupt status
 *END*************************************************************************/
bool EWM_DRV_GetIntCmd(uint32_t instance)
{
  
   assert(instance < HW_EWM_INSTANCE_COUNT);
  
   uint32_t baseAddr;
  
   baseAddr = g_ewmBaseAddr[instance];
   
   return (EWM_HAL_GetIntCmd(baseAddr));
}

/*FUNCTION*********************************************************************
 *
 * Function Name : EWM_DRV_SetIntCmd
 * Description   : Enable/disable EWM interrupt
 *END*************************************************************************/
void EWM_DRV_SetIntCmd(uint32_t instance, bool enable)
{
    uint32_t baseAddr;
    baseAddr = g_ewmBaseAddr[instance];
    
    EWM_HAL_SetIntCmd(baseAddr, enable);
}
/*******************************************************************************
 * EOF
 *******************************************************************************/

