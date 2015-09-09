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

#include "fsl_device_registers.h"
#include "fsl_clock_manager.h"
#include "fsl_os_abstraction.h"
#include <assert.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
static clock_manager_state_t g_clockState;

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_Init
 * Description   : Install pre-defined clock configurations.
 * This function installs the pre-defined clock configuration table to the
 * clock manager.
 *
 *END**************************************************************************/
clock_manager_error_code_t CLOCK_SYS_Init(clock_manager_user_config_t const *clockConfigsPtr,
                              uint8_t configsNumber,
                              clock_manager_callback_user_config_t *(*callbacksPtr)[],
                              uint8_t callbacksNumber)
{
    assert(NULL != clockConfigsPtr);
    assert(NULL != callbacksPtr);

    g_clockState.configTable     = clockConfigsPtr;
    g_clockState.clockConfigNum  = configsNumber;
    g_clockState.callbackConfig  = callbacksPtr;
    g_clockState.callbackNum     = callbacksNumber;

    /*
     * errorCallbackIndex is the index of the callback which returns error
     * during clock mode switch. If all callbacks return success, then the
     * errorCallbackIndex is callbacksnumber.
     */
    g_clockState.errorCallbackIndex = callbacksNumber;

    return kClockManagerSuccess;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_UpdateConfiguration
 * Description   : Send notification and change system clock configuration.
 * This function sends the notification to all callback functions, if all
 * callbacks return OK or forceful policy is used, this function will change
 * system clock configuration.
 *
 *END**************************************************************************/
clock_manager_error_code_t CLOCK_SYS_UpdateConfiguration(uint8_t targetConfigIndex,
                                                   clock_manager_policy_t policy)
{
    uint8_t callbackIdx;
    clock_manager_error_code_t ret = kClockManagerSuccess;

    clock_manager_callback_user_config_t* callbackConfig;

    clock_notify_struct_t notifyStruct = {
        .targetClockConfigIndex = targetConfigIndex,
        .policy                 = policy,
    };

    /* Clock configuration index is out of range. */
    if (targetConfigIndex >= g_clockState.clockConfigNum)
    {
        return kClockManagerErrorOutOfRange;
    }

    /* Set errorcallbackindex as callbackNum, which means no callback error now.*/
    g_clockState.errorCallbackIndex = g_clockState.callbackNum;

    /* First step: Send "BEFORE" notification. */
    notifyStruct.notifyType = kClockManagerNotifyBefore;

    /* Send notification to all callback. */
    for (callbackIdx=0; callbackIdx<g_clockState.callbackNum; callbackIdx++)
    {
        callbackConfig = (*g_clockState.callbackConfig)[callbackIdx];
        if ((NULL != callbackConfig) &&
            ((uint8_t)callbackConfig->callbackType & (uint8_t)kClockManagerNotifyBefore))
        {
            if (kClockManagerSuccess !=
                    (*callbackConfig->callback)(&notifyStruct,
                        callbackConfig->callbackData))
            {
                g_clockState.errorCallbackIndex = callbackIdx;
                /* Save the error callback index. */
                ret = kClockManagerErrorNotificationBefore;

                if (kClockManagerPolicyAgreement == policy)
                {
                    break;
                }
            }
        }
    }

    /* If all callback success or forceful policy is used. */
    if ((kClockManagerSuccess == ret) ||
        (policy == kClockManagerPolicyForcible))
    {
        /* clock mode switch. */
        //OSA_EnterCritical(kCriticalDisableInt);
        CLOCK_SYS_SetConfiguration(&g_clockState.configTable[targetConfigIndex]);

        g_clockState.curConfigIndex = targetConfigIndex;
        //OSA_ExitCritical(kCriticalDisableInt);

        notifyStruct.notifyType = kClockManagerNotifyAfter;

        for (callbackIdx=0; callbackIdx<g_clockState.callbackNum; callbackIdx++)
        {
            callbackConfig = (*g_clockState.callbackConfig)[callbackIdx];
            if ((NULL != callbackConfig) &&
                ((uint8_t)callbackConfig->callbackType & (uint8_t)kClockManagerNotifyAfter))
            {
                if (kClockManagerSuccess !=
                        (*callbackConfig->callback)(&notifyStruct,
                            callbackConfig->callbackData))
                {
                    g_clockState.errorCallbackIndex = callbackIdx;
                    /* Save the error callback index. */
                    ret = kClockManagerErrorNotificationAfter;

                    if (kClockManagerPolicyAgreement == policy)
                    {
                        break;
                    }
                }
            }
        }
    }
    else /* Error occurs, need to send "RECOVER" notification. */
    {
        notifyStruct.notifyType = kClockManagerNotifyRecover;
        while (callbackIdx--)
        {
            callbackConfig = (*g_clockState.callbackConfig)[callbackIdx];
            if (NULL != callbackConfig)
            {
                (*callbackConfig->callback)(&notifyStruct,
                        callbackConfig->callbackData);
            }
        }
    }

    return ret;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetCurrentConfiguration
 * Description   : Get current clock configuration index.
 *
 *END**************************************************************************/
uint8_t CLOCK_SYS_GetCurrentConfiguration(void)
{
    return g_clockState.curConfigIndex;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetErrorCallback
 * Description   : Get the callback which returns error in last clock switch.
 *
 *END**************************************************************************/
clock_manager_callback_user_config_t* CLOCK_SYS_GetErrorCallback(void)
{
    /* If all callbacks return success. */
    if (g_clockState.errorCallbackIndex >= g_clockState.clockConfigNum)
    {
        return NULL;
    }
    else
    {
        return (*g_clockState.callbackConfig)[g_clockState.errorCallbackIndex];
    }
}

#if (defined(FSL_FEATURE_MCGLITE_MCGLITE))
/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_SYS_SetMcgliteMode
 * Description  : Set MCG_Lite to some mode.
 * This function transitions the MCG_lite to some mode according to configuration
 * parameter.
 *
 *END***********************************************************************************/
mcglite_mode_error_t CLOCK_SYS_SetMcgliteMode(mcglite_config_t const *targetConfig)
{
    uint32_t outFreq = 0U;
    mcglite_mode_error_t ret = kMcgliteModeErrNone;

    assert(targetConfig->mcglite_mode < kMcgliteModeStop);

    /* MCG_LITE mode change. */
    switch (targetConfig->mcglite_mode)
    {
        case kMcgliteModeLirc8M:
            ret = CLOCK_HAL_SetLircMode(MCG_BASE,
                                        kMcgliteLircSel8M,
                                        targetConfig->fcrdiv,
                                        &outFreq);
            break;
        case kMcgliteModeLirc2M:
            ret = CLOCK_HAL_SetLircMode(MCG_BASE,
                                        kMcgliteLircSel2M,
                                        targetConfig->fcrdiv,
                                        &outFreq);
            break;
        case kMcgliteModeExt:
            ret = CLOCK_HAL_SetExtMode(MCG_BASE, &outFreq);
            break;
        default:
            ret = CLOCK_HAL_SetHircMode(MCG_BASE, &outFreq);
            break;
    }

    /* Set other registers. */
    if (kMcgliteModeErrNone == ret)
    {
        /* Enable HIRC when MCG_LITE is not in HIRC mode. */
        CLOCK_HAL_SetHircCmd(MCG_BASE, targetConfig->hircEnable);

        /* Enable IRCLK. */
        CLOCK_HAL_SetLircCmd(MCG_BASE, targetConfig->irclkEnable);
        CLOCK_HAL_SetLircStopCmd(MCG_BASE, targetConfig->irclkEnableInStop);

        /* Set IRCS. */
        CLOCK_HAL_SetLircSelMode(MCG_BASE, targetConfig->ircs);

        /* Set LIRC_DIV2. */
        CLOCK_HAL_SetLircDiv2(MCG_BASE, targetConfig->lircDiv2);
    }

    return ret;
}
#else

/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_SYS_SetMcgPeeToFbe
 * Description  : This function changes MCG from PEE mode to FBE mode, it is
 * only used internally.
 *
 *END***********************************************************************************/
#if FSL_FEATURE_MCG_HAS_PLL
static void CLOCK_SYS_SetMcgPeeToFbe(void)
{
    /* Change to use external clock first. */
    CLOCK_HAL_SetClkSrcMode(MCG_BASE, kMcgClkSelExternal);
    /* Wait for clock status bits to update */
    while (CLOCK_HAL_GetClkStatMode(MCG_BASE) != kMcgClkStatExternalRef) {}

    /* Set PLLS to select FLL. */
    CLOCK_HAL_SetPllSelMode(MCG_BASE, kMcgPllSelFll);
    // wait for PLLST status bit to set
    while ((CLOCK_HAL_GetPllStatMode(MCG_BASE) != kMcgPllStatFll)) {}
}
#endif

/*FUNCTION******************************************************************************
 *
 * Function name : CLOCK_SYS_SetMcgMode
 * Description  : This function sets MCG to some target mode defined by the configure
 * structure, if can not switch to target mode directly, this function will choose
 * the proper path. If external clock is used in the target mode, please make sure
 * it is enabled, for example, if the external oscillator is used, please setup EREFS/HGO
 * correctly and make sure OSCINIT is set.
 * This function is used by clock dynamic setting, it only supports some specific
 * mode transitions, including:
 * 1. FEI      ==>    FEE/PEE
 * 2. BLPI    <==>    FEE/PEE
 * 3. Reconfigure PLL in PEE mode.
 * 4. Reconfigure FLL in FEE mode.
 *
 *END***********************************************************************************/
mcg_mode_error_t CLOCK_SYS_SetMcgMode(mcg_config_t const *targetConfig,
                                      void (* fllStableDelay)(void))
{
    uint32_t outClkFreq;
#if FSL_FEATURE_MCG_HAS_PLL
    /* Current mode is only used for PEE mode transition. */
    mcg_modes_t curMode;  // Current MCG mode.
#endif

#if FSL_FEATURE_MCG_USE_OSCSEL
    mcg_oscsel_select_t oscsel = targetConfig->oscsel;
#else
    mcg_oscsel_select_t oscsel = kMcgOscselOsc;
#endif

#if FSL_FEATURE_MCG_HAS_PLL
#if FSL_FEATURE_MCG_HAS_PLL1
    mcg_pll_clk_select_t pllcs = targetConfig->plls;
#else
    mcg_pll_clk_select_t pllcs = kMcgPllClkSelPll0;
#endif
#endif

#if FSL_FEATURE_MCG_HAS_PLL
    curMode = CLOCK_HAL_GetMcgMode(MCG_BASE);
#endif

    if (kMcgModeBLPI == targetConfig->mcg_mode)
    {
#if FSL_FEATURE_MCG_HAS_PLL
        // If current mode is PEE mode, swith to FBE mode first.
        if (kMcgModePEE == curMode)
        {
            CLOCK_SYS_SetMcgPeeToFbe();
        }
#endif
        // Change to FBI mode.
        CLOCK_HAL_SetFbiMode(MCG_BASE,
                             targetConfig->drs,
                             targetConfig->ircs,
                             targetConfig->fcrdiv,
                             fllStableDelay,
                             &outClkFreq);

        // Enable low power mode to enter BLPI.
        CLOCK_HAL_SetLowPowerMode(MCG_BASE, kMcgLowPowerSelLowPower);
    }
    else if (kMcgModeFEE == targetConfig->mcg_mode)
    {
#if FSL_FEATURE_MCG_HAS_PLL
        // If current mode is PEE mode, swith to FBE mode first.
        if (kMcgModePEE == curMode)
        {
            CLOCK_SYS_SetMcgPeeToFbe();
        }
#endif
        // Disalbe low power mode.
        CLOCK_HAL_SetLowPowerMode(MCG_BASE, kMcgLowPowerSelNormal);
        // Configure FLL in FBE mode then switch to FEE mode.
        CLOCK_HAL_SetFbeMode(MCG_BASE,
                             oscsel,
                             targetConfig->frdiv,
                             targetConfig->dmx32,
                             targetConfig->drs,
                             fllStableDelay,
                             &outClkFreq);
        // Change CLKS to enter FEE mode.
        CLOCK_HAL_SetClkSrcMode(MCG_BASE, kMcgClkSelOut);
        while (CLOCK_HAL_GetClkStatMode(MCG_BASE) != kMcgClkStatFll) {}
    }
#if FSL_FEATURE_MCG_HAS_PLL
    else if (kMcgModePEE == targetConfig->mcg_mode)
    {
        /*
         * If current mode is FEI/FEE/BLPI, then swith to FBE mode first.
         * If current mode is PEE mode, which means need to reconfigure PLL,
         * fist switch to PBE mode and configure PLL, then switch to PEE.
         */

        if (kMcgModePEE != curMode)
        {
            // Disalbe low power mode.
            CLOCK_HAL_SetLowPowerMode(MCG_BASE, kMcgLowPowerSelNormal);

            // Change to FBE mode. Do not need to set FRDIV in FBE mode.
            CLOCK_HAL_SetClksFrdivInternalRefSelect(MCG_BASE,
                                                    kMcgClkSelExternal,
                                                    CLOCK_HAL_GetFllExternalRefDiv(MCG_BASE),
                                                    kMcgInternalRefClkSrcExternal);
            while (kMcgInternalRefStatExternal != CLOCK_HAL_GetInternalRefStatMode(MCG_BASE)) {}
            while (CLOCK_HAL_GetClkStatMode(MCG_BASE) != kMcgClkStatExternalRef) {}
        }

        // Change to PBE mode.
        CLOCK_HAL_SetPbeMode(MCG_BASE,
                             oscsel,
                             pllcs,
                             targetConfig->prdiv0,
                             targetConfig->vdiv0,
                             &outClkFreq);

        // Set CLKS to enter PEE mode.
        CLOCK_HAL_SetClkSrcMode(MCG_BASE, kMcgClkSelOut);
        while (CLOCK_HAL_GetClkStatMode(MCG_BASE) != kMcgClkStatPll) {}
    }
#endif
    else
    {
        return kMcgModeErrModeUnreachable;
    }

    /* Update FCRDIV if necessary. */
    CLOCK_HAL_UpdateFastClkInternalRefDiv(MCG_BASE, targetConfig->fcrdiv);

    /* Enable MCGIRCLK. */
    CLOCK_HAL_SetInternalClkCmd(MCG_BASE, targetConfig->irclkEnable);
    CLOCK_HAL_SetInternalRefStopCmd(MCG_BASE, targetConfig->irclkEnableInStop);

#if FSL_FEATURE_MCG_HAS_PLL
    /* Enable PLL0. */
    if (targetConfig->pll0Enable)
    {
        CLOCK_HAL_SetPllExternalRefDiv0(MCG_BASE, targetConfig->prdiv0);
        CLOCK_HAL_SetVoltCtrlOscDiv0(MCG_BASE, targetConfig->vdiv0);
        CLOCK_HAL_SetPllClk0Cmd(MCG_BASE, true);
        /* Check LOCK bit is set. */
        while ((CLOCK_HAL_GetLock0Mode(MCG_BASE) !=  kMcgLockLocked)) {}
    }
    else
    {
        CLOCK_HAL_SetPllClk0Cmd(MCG_BASE, false);
    }
    /* Enable/disable PLL0 in stop mode. */
    CLOCK_HAL_SetPll0StopEnableCmd(MCG_BASE, targetConfig->pll0EnableInStop);
#endif

    return kMcgModeErrNone;
}

#endif
/*******************************************************************************
 * EOF
 ******************************************************************************/

