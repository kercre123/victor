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

#include "assert.h"
#include "fsl_cmp_driver.h"
#include "fsl_cmp_hal.h"
#include "fsl_clock_manager.h"
#include "fsl_interrupt_manager.h"

/*! @brief Table of pointers to internal state structure for CMP instances. */
static cmp_state_t * volatile g_cmpStatePtr[HW_CMP_INSTANCE_COUNT];

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_StructInitUserConfigDefault
 * Description   :  Fill initial user configuration for default setting. 
 * The default setting will make the CMP module at least to be an comparator.
 * It includes the setting of :
 *     .hystersisMode = kCmpHystersisOfLevel0
 *     .pinoutEnable = true
 *     .pinoutUnfilteredEnable = true
 *     .invertEnable = false
 *     .highSpeedEnable = false
 *     .dmaEnable = false
 *     .risingIntEnable = false
 *     .fallingIntEnable = false
 *     .triggerEnable = false
 * However, it is still recommended to fill some fields of structure such as
 * channel mux according to application. Note that this API will not set the
 * configuration to hardware.
 *
 *END*************************************************************************/
cmp_status_t CMP_DRV_StructInitUserConfigDefault(cmp_user_config_t *userConfigPtr,
    cmp_chn_mux_mode_t plusInput, cmp_chn_mux_mode_t minusInput)
{
    if (!userConfigPtr)
    {
        return kStatus_CMP_InvalidArgument;
    }
    
    userConfigPtr->hystersisMode = kCmpHystersisOfLevel0;
    userConfigPtr->pinoutEnable = true;
    userConfigPtr->pinoutUnfilteredEnable = false;
    userConfigPtr->invertEnable = false;
    userConfigPtr->highSpeedEnable = false;
#if FSL_FEATURE_CMP_HAS_DMA
    userConfigPtr->dmaEnable = false;
#endif /* FSL_FEATURE_CMP_HAS_DMA */
    userConfigPtr->risingIntEnable = false;
    userConfigPtr->fallingIntEnable = false;
    userConfigPtr->plusChnMux = plusInput;
    userConfigPtr->minusChnMux = minusInput;
#if FSL_FEATURE_CMP_HAS_TRIGGER_MODE
    userConfigPtr->triggerEnable = false;
#endif /* FSL_FEATURE_CMP_HAS_TRIGGER_MODE */

#if FSL_FEATURE_CMP_HAS_PASS_THROUGH_MODE
    userConfigPtr->passThroughEnable = false;
#endif /* FSL_FEATURE_CMP_HAS_PASS_THROUGH_MODE */

    return kStatus_CMP_Success;
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_Init
 * Description   : Initialize the CMP module. It will enable the clock and
 * set the interrupt switcher for CMP module. And the CMP module will be
 * configured for the basic comparator.
 *
 *END*************************************************************************/
cmp_status_t CMP_DRV_Init(uint32_t instance, cmp_user_config_t *userConfigPtr,
    cmp_state_t *userStatePtr)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);

    uint32_t baseAddr = g_cmpBaseAddr[instance];

    if ( (!userConfigPtr) || (!userStatePtr) )
    {
        return kStatus_CMP_InvalidArgument;
    }

    /* Enable clock for CMP. */
    if (!CLOCK_SYS_GetCmpGateCmd(instance) )
    {
        CLOCK_SYS_EnableCmpClock(instance);
    }

    /* Reset all the registers. */
    CMP_HAL_Init(baseAddr);

    /* Configure the comparator. */
    CMP_HAL_SetHystersisMode(baseAddr, userConfigPtr->hystersisMode);
    CMP_HAL_SetOutputPinCmd(baseAddr, userConfigPtr->pinoutEnable);
    CMP_HAL_SetUnfilteredOutCmd(baseAddr, userConfigPtr->pinoutUnfilteredEnable);
    CMP_HAL_SetInvertLogicCmd(baseAddr, userConfigPtr->invertEnable);
    CMP_HAL_SetHighSpeedCmd(baseAddr, userConfigPtr->highSpeedEnable);
#if FSL_FEATURE_CMP_HAS_DMA
    CMP_HAL_SetDmaCmd(baseAddr, userConfigPtr->dmaEnable);
#endif /* FSL_FEATURE_CMP_HAS_DMA */
    CMP_HAL_SetOutputRisingIntCmd(baseAddr, userConfigPtr->risingIntEnable);
    CMP_HAL_SetOutputFallingIntCmd(baseAddr,userConfigPtr->fallingIntEnable);
    CMP_HAL_SetMinusInputChnMuxMode(baseAddr, userConfigPtr->minusChnMux);
    CMP_HAL_SetPlusInputChnMuxMode(baseAddr, userConfigPtr->plusChnMux);
    
#if FSL_FEATURE_CMP_HAS_TRIGGER_MODE
    CMP_HAL_SetTriggerModeCmd(baseAddr, userConfigPtr->triggerEnable);
#endif /* FSL_FEATURE_CMP_HAS_TRIGGER_MODE */

#if FSL_FEATURE_CMP_HAS_PASS_THROUGH_MODE
    CMP_HAL_SetPassThroughModeCmd(baseAddr, userConfigPtr->passThroughEnable);
#endif /* FSL_FEATURE_CMP_HAS_PASS_THROUGH_MODE */

    /* Configure the NVIC. */
    if ( (userConfigPtr->risingIntEnable) || (userConfigPtr->fallingIntEnable) )
    {
        /* Enable the CMP interrupt in NVIC. */
        INT_SYS_EnableIRQ(g_cmpIrqId[instance] );
    }
    else
    {
        /* Disable the CMP interrupt in NVIC. */
        INT_SYS_DisableIRQ(g_cmpIrqId[instance] );
    }

    userStatePtr->isInUsed = true; /* Mark it as in used. */
    g_cmpStatePtr[instance] = userStatePtr; /* Linked the user-provided memory into context record. */
    
    return kStatus_CMP_Success;
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_Deinit
 * Description   : De-initialize the CMP module. It will shutdown the CMP's
 * clock and disable the interrupt. This API should be called when CMP is no
 * longer used in application and it will help to reduce the power consumption.
 *
 *END*************************************************************************/
void CMP_DRV_Deinit(uint32_t instance)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);

    uint32_t i;
    uint32_t baseAddr = g_cmpBaseAddr[instance];

    /* Be sure to disable the CMP module. */
    CMP_HAL_Disable(baseAddr);
    
    /* Disable the CMP interrupt in NVIC. */
    INT_SYS_DisableIRQ(g_cmpIrqId[instance] );

    /* Unmask the CMP not in use. */
    g_cmpStatePtr[instance]->isInUsed = false;

    /* Disable the clock if necessary. */
    for (i = 0U; i < HW_CMP_INSTANCE_COUNT; i++)
    {
        if ( (g_cmpStatePtr[i]) && (g_cmpStatePtr[i]->isInUsed) )
        {
            /* There are still some CMP instances in used. */
            break;
        }
    }
    if (i == HW_CMP_INSTANCE_COUNT)
    {
        /* Disable the shared clock. */
        CLOCK_SYS_DisableCmpClock(0U);
    }
    
    g_cmpStatePtr[instance] = NULL;
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_Start
 * Description   : Start the CMP module. The configuration would not take
 * effect until the module is started.
 *
 *END*************************************************************************/
void CMP_DRV_Start(uint32_t instance)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);
    
    uint32_t baseAddr = g_cmpBaseAddr[instance];
    CMP_HAL_Enable(baseAddr);
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_Stop
 * Description   : Stop the CMP module. Note that this API would shutdown
 * the module, but only pauses the features tenderly.
 *
 *END*************************************************************************/
void CMP_DRV_Stop(uint32_t instance)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);
    
    uint32_t baseAddr = g_cmpBaseAddr[instance];
    CMP_HAL_Disable(baseAddr);
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_EnableDac
 * Description   : Enable the internal DAC in CMP module. It will take
 * effect actually only when internal DAC has been chosen as one of input
 * channel for comparator. Then the DAC channel can be programmed to provide
 * a reference voltage level.
 *
 *END*************************************************************************/
cmp_status_t CMP_DRV_EnableDac(uint32_t instance, cmp_dac_config_t *dacConfigPtr)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);

    uint32_t baseAddr = g_cmpBaseAddr[instance];

    if (!dacConfigPtr)
    {
        return kStatus_CMP_InvalidArgument;
    }
    /* Configure the DAC Control Register. */
    CMP_HAL_SetDacCmd(baseAddr, true);
    CMP_HAL_SetDacRefVoltSrcMode(baseAddr, dacConfigPtr->refVoltSrcMode);
    CMP_HAL_SetDacValue(baseAddr, dacConfigPtr->dacValue);

    return kStatus_CMP_Success;
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_DisableDac
 * Description   : Disable the internal DAC in CMP module. It should be
 * called if the internal DAC is no longer used in application.
 *
 *END*************************************************************************/
void CMP_DRV_DisableDac(uint32_t instance)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);

    uint32_t baseAddr = g_cmpBaseAddr[instance];
    
    CMP_HAL_SetDacCmd(baseAddr, false);
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_ConfigSampleFilter
 * Description   : Configure the CMP working in Sample\Filter modes. These
 * modes are some advanced features beside the basic comparator. They may
 * be about Windowed Mode, Filter Mode and so on. See to 
 * "cmp_sample_filter_config_t"for detailed description.
 *
 *END*************************************************************************/
cmp_status_t CMP_DRV_ConfigSampleFilter(uint32_t instance, cmp_sample_filter_config_t *configPtr)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);

    uint32_t baseAddr = g_cmpBaseAddr[instance];
    
    if (!configPtr)
    {
        return kStatus_CMP_InvalidArgument;
    }
    
    /* Configure the comparator Window/Filter mode. */
    switch (configPtr->workMode)
    {
    case kCmpContinuousMode:
        /* Continuous Mode:
        * Both window control and filter blocks are completely bypassed.
        * The output of comparator is updated continuously. 
        */
#if FSL_FEATURE_CMP_HAS_WINDOW_MODE
        CMP_HAL_SetWindowModeCmd(baseAddr, false);
#endif /* FSL_FEATURE_CMP_HAS_WINDOW_MODE */
        CMP_HAL_SetSampleModeCmd(baseAddr, false);
        CMP_HAL_SetFilterCounterMode(baseAddr, kCmpFilterCountSampleOf0);
        CMP_HAL_SetFilterPeriodValue(baseAddr, 0U);
        break;
    case kCmpSampleWithNoFilteredMode:
        /* Sample, Non-Filtered Mode:
        * Windowing control is completely bypassed. The output of
        * comparator is sampled whenever a rising-edge is detected on
        * the filter block clock input. Of course, the filter clock
        * prescaler can be configured as the divider from bus clock.
        */
#if FSL_FEATURE_CMP_HAS_WINDOW_MODE
        CMP_HAL_SetWindowModeCmd(baseAddr, false);
#endif /* FSL_FEATURE_CMP_HAS_WINDOW_MODE */
        if (configPtr->useExtSampleOrWindow)
        {
            CMP_HAL_SetSampleModeCmd(baseAddr, true);
        }
        else
        {
            CMP_HAL_SetSampleModeCmd(baseAddr, false);
            CMP_HAL_SetFilterPeriodValue(baseAddr, configPtr->filterClkDiv);
        }
        CMP_HAL_SetFilterCounterMode(baseAddr, kCmpFilterCountSampleOf1);
        break;
    case kCmpSampleWithFilteredMode:
        /* Sample, Filtered Mode:
        * Similar to "Sample, Non-Filtered Mode", but the filter is
        * active in this mode. The filter counter value becomes
        * configurable as well.
        */
#if FSL_FEATURE_CMP_HAS_WINDOW_MODE
        CMP_HAL_SetWindowModeCmd(baseAddr, false);
#endif /* FSL_FEATURE_CMP_HAS_WINDOW_MODE */
        if (configPtr->useExtSampleOrWindow)
        {
            CMP_HAL_SetSampleModeCmd(baseAddr, true);
        }
        else
        {
            CMP_HAL_SetSampleModeCmd(baseAddr, false);
            CMP_HAL_SetFilterPeriodValue(baseAddr, configPtr->filterClkDiv);
        }
        CMP_HAL_SetFilterCounterMode(baseAddr, configPtr->filterCount);
        break;
    case kCmpWindowedMode:
        /* Windowed Mode:
        * In Windowed Mode, only output of analog comparator is passed
        * only when the WINDOW signal is high. The last latched value
        * is held when WINDOW signal is low.
        */
#if FSL_FEATURE_CMP_HAS_WINDOW_MODE
        CMP_HAL_SetWindowModeCmd(baseAddr, true);
#endif /* FSL_FEATURE_CMP_HAS_WINDOW_MODE */
        CMP_HAL_SetSampleModeCmd(baseAddr, false);
        CMP_HAL_SetFilterCounterMode(baseAddr, kCmpFilterCountSampleOf0);
        CMP_HAL_SetFilterPeriodValue(baseAddr, 0U);
        break;
    case kCmpWindowedFilteredMode:
        /* Window/Filtered Mode:
        * This mode is kind of complex, as it uses both windowing and
        * filtering features. It also has the highest latency of all
        * modes. This can be approximated: up to 1 bus clock
        * synchronization in the window function 
        * + ( ( filter counter * filter prescaler ) + 1) bus clock
        * for the filter function.
        */
#if FSL_FEATURE_CMP_HAS_WINDOW_MODE
        CMP_HAL_SetWindowModeCmd(baseAddr, true);
#endif /* FSL_FEATURE_CMP_HAS_WINDOW_MODE */
        CMP_HAL_SetSampleModeCmd(baseAddr, false);
        CMP_HAL_SetFilterCounterMode(baseAddr, configPtr->filterCount);
        CMP_HAL_SetFilterPeriodValue(baseAddr, configPtr->filterClkDiv);
        break;
    default:
        /* Default Mode:
        * Same as continuous mode. See to "kCmpContinuousMode". 
        */
#if FSL_FEATURE_CMP_HAS_WINDOW_MODE
        CMP_HAL_SetWindowModeCmd(baseAddr, false);
#endif /* FSL_FEATURE_CMP_HAS_WINDOW_MODE */
        CMP_HAL_SetSampleModeCmd(baseAddr, false);
        CMP_HAL_SetFilterCounterMode(baseAddr, kCmpFilterCountSampleOf0);
        CMP_HAL_SetFilterPeriodValue(baseAddr, 0U);
        break;
    }
    
    return kStatus_CMP_Success;
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_GetOutput
 * Description   : Get the output of CMP module.
 * The output source depends on the configuration when initializing the comparator.
 * When cmp_user_config_t.pinoutUnfilteredEnable = false, the output will be
 * processed by filter. Otherwise, the output would be the signal did not pass
 * the filter.
 *
 *END*************************************************************************/
bool CMP_DRV_GetOutput(uint32_t instance)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);

    uint32_t baseAddr = g_cmpBaseAddr[instance];
    
    return CMP_HAL_GetOutputLogic(baseAddr);
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_GetFlag
 * Description   : Get the state of CMP module. It will return if indicated
 * event has been detected.
 *
 *END*************************************************************************/
bool CMP_DRV_GetFlag(uint32_t instance, cmp_flag_t flag)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);

    bool bRet;
    uint32_t baseAddr = g_cmpBaseAddr[instance];

    switch(flag)
    {
    case kCmpFlagOfCoutRising:
        bRet = CMP_HAL_GetOutputRisingFlag(baseAddr);
        break;
    case kCmpFlagOfCoutFalling:
        bRet = CMP_HAL_GetOutputFallingFlag(baseAddr);
        break;
    default:
        bRet = false;
        break;
    }
    return bRet;
}

/*FUNCTION*********************************************************************
 *
 * Function Name : CMP_DRV_ClearFlag
 * Description   : Clear event record of CMP module. 
 *
 *END*************************************************************************/
void CMP_DRV_ClearFlag(uint32_t instance, cmp_flag_t flag)
{
    assert(instance < HW_CMP_INSTANCE_COUNT);

    uint32_t baseAddr = g_cmpBaseAddr[instance];

    switch(flag)
    {
    case kCmpFlagOfCoutRising:
        CMP_HAL_ClearOutputRisingFlag(baseAddr);
        break;
    case kCmpFlagOfCoutFalling:
        CMP_HAL_ClearOutputFallingFlag(baseAddr);
        break;
    default:
        break;
    }
}

/*******************************************************************************
 * EOF
 ******************************************************************************/

