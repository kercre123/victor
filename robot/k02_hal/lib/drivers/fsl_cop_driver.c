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

#include "fsl_cop_driver.h"
#include "fsl_interrupt_manager.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*******************************************************************************
 * Code
 *******************************************************************************/

/*FUNCTION****************************************************************
 *
 * Function Name : cop_init 
 * Description   : Initialize COP.
 * This function is used to initialize the COP, after called, the COP 
 * will run immediately according to the configure.
 *
 *END*********************************************************************/
void COP_DRV_Init(uint32_t instance, const cop_user_config_t* initPtr)
{
    assert(initPtr);
    uint32_t baseAddr = g_copBaseAddr[instance];
    cop_common_config_t copCommonConfig;
    
    copCommonConfig.commonConfig.copWindowEnable = initPtr->copWindowModeEnable;
#if FSL_FEATURE_COP_HAS_LONGTIME_MODE    
    copCommonConfig.commonConfig.copTimeoutMode     = initPtr->copTimeoutMode;
    copCommonConfig.commonConfig.copStopModeEnable  = initPtr->copStopModeEnable;
    copCommonConfig.commonConfig.copDebugModeEnable = initPtr->copDebugModeEnable;
    copCommonConfig.commonConfig.copClockSource     = initPtr->copClockSource;
#else
    copCommonConfig.commonConfig.copClockSelect     = initPtr->copClockSource;
    copCommonConfig.commonConfig.reserved0          = 0;
#endif
    copCommonConfig.commonConfig.copTimeout         = initPtr->copTimeout;
    copCommonConfig.commonConfig.reserved1          = 0;
    
    COP_HAL_SetCommonConfiguration(baseAddr, copCommonConfig);
}

/*FUNCTION****************************************************************
 *
 * Function Name : COP_DRV_Deinit
 * Description   : Disable COP watchdog at reset.
 * This function is used to shutdown the COP.
 *
 *END*********************************************************************/
void COP_DRV_Deinit(uint32_t instance)
{
    uint32_t baseAddr = g_copBaseAddr[instance];
    
    COP_HAL_Disable(baseAddr);
}

/*FUNCTION****************************************************************
 *
 * Function Name : COP_DRV_IsRunning
 * Description   : Get COP running status.
 * This function is used to get the COP running status.
 * @retval true COP watchdog is running.
 * @retval false COP watchdog is disabled.
 *
 *END*********************************************************************/
bool COP_DRV_IsRunning(uint32_t instance)
{
   uint32_t baseAddr = g_copBaseAddr[instance];
   
   return COP_HAL_IsEnabled(baseAddr);
}

/*FUNCTION****************************************************************
 *
 * Function Name : cop_refresh 
 * Description   : Refresh COP.
 * This function is used to feed the COP, it will set the COP timer count to zero and 
 * should be called before COP timer is timeout, otherwise a RESET will assert.
 *
  *END*********************************************************************/
void COP_DRV_Refresh(uint32_t instance)
{
   uint32_t baseAddr = g_copBaseAddr[instance];
    
   INT_SYS_DisableIRQGlobal();

   COP_HAL_Refresh(baseAddr);

   INT_SYS_EnableIRQGlobal();
}

/*FUNCTION****************************************************************
 *
 * Function Name : cop_refresh 
 * Description   : Refresh COP.
 * This function is used to feed the COP, it will set the COP timer count to zero and 
 * should be called before COP timer is timeout, otherwise a RESET will assert.
 *
  *END*********************************************************************/
void COP_DRV_ResetSystem(uint32_t instance)
{
    uint32_t baseAddr = g_copBaseAddr[instance];
    COP_HAL_ResetSystem(baseAddr);
}


/*******************************************************************************
 * EOF
 *******************************************************************************/
