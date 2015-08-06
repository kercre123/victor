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

#include "fsl_mcg_hal_modes.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*****************************************************************
 * MCG clock mode transition functions
 *
 *   FEI -> FEE
 *   FEI -> FBI
 *   FEI -> FBE
 *
 *   FEE -> FEI
 *   FEE -> FBI
 *   FEE -> FBE
 *
 *   FBI -> FEI
 *   FBI -> FEE
 *   FBI -> FBE
 *   FBI -> BLPI
 *
 *   BLPI -> FBI
 *
 *   FBE -> FEE
 *   FBE -> FEI
 *   FBE -> FBI
 *   FBE -> PBE
 *   FBE -> BLPE
 *
 *   PBE -> FBE
 *   PBE -> PEE
 *   PBE -> BLPE
 *
 *   BLPE -> FBE
 *   BLPE -> PBE
 *
 *   PEE -> PBE
 *
 *****************************************************************/

/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_HAL_GetMcgMode
 * Description  : This function will check the mcg registers and determine
 * current MCG mode.
 *
 * Return value : mcgMode or error code mcg_modes_t defined in fsl_mcg_hal_modes.h
 *END***********************************************************************************/
mcg_modes_t CLOCK_HAL_GetMcgMode(uint32_t baseAddr)
{
    mcg_modes_t               mode   = kMcgModeError;
    mcg_clk_stat_status_t     clkst  = CLOCK_HAL_GetClkStatMode(baseAddr);
    mcg_internal_ref_status_t irefst = CLOCK_HAL_GetInternalRefStatMode(baseAddr);
    mcg_low_power_select_t    lp     = CLOCK_HAL_GetLowPowerMode(baseAddr);
#if FSL_FEATURE_MCG_HAS_PLL
    mcg_pll_stat_status_t     pllst  = CLOCK_HAL_GetPllStatMode(baseAddr);
#endif

    /***********************************************************************
                           Mode and Registers
    ____________________________________________________________________

      Mode   |   CLKST    |   IREFST   |   PLLST   |      LP
    ____________________________________________________________________

      FEI    |  00(FLL)   |   1(INT)   |   0(FLL)  |      X
    ____________________________________________________________________

      FEE    |  00(FLL)   |   0(EXT)   |   0(FLL)  |      X
    ____________________________________________________________________

      FBE    |  10(EXT)   |   0(EXT)   |   0(FLL)  |   0(NORMAL)
    ____________________________________________________________________

      FBI    |  01(INT)   |   1(INT)   |   0(FLL)  |   0(NORMAL)
    ____________________________________________________________________

      BLPI   |  01(INT)   |   1(INT)   |   0(FLL)  |   1(LOW POWER)
    ____________________________________________________________________

      BLPE   |  10(EXT)   |   0(EXT)   |     X     |   1(LOW POWER)
    ____________________________________________________________________

      PEE    |  11(PLL)   |   0(EXT)   |   1(PLL)  |      X
    ____________________________________________________________________

      PBE    |  10(EXT)   |   0(EXT)   |   1(PLL)  |   O(NORMAL)
    ____________________________________________________________________

    ***********************************************************************/

    switch (clkst)
    {
        case kMcgClkStatFll:
#if FSL_FEATURE_MCG_HAS_PLL
        if (kMcgPllStatFll == pllst)
#endif
        {

            if (kMcgInternalRefStatExternal == irefst)
            {
                mode = kMcgModeFEE;
            }
            else
            {
                mode = kMcgModeFEI;
            }
        }
        break;
        case kMcgClkStatInternalRef:
        if ((kMcgInternalRefStatInternal == irefst)
#if FSL_FEATURE_MCG_HAS_PLL
            && (kMcgPllStatFll == pllst)
#endif
           )
        {
            if (kMcgLowPowerSelNormal == lp)
            {
                mode = kMcgModeFBI;
            }
            else
            {
                mode = kMcgModeBLPI;
            }
        }
        break;
        case kMcgClkStatExternalRef:
        if (kMcgInternalRefStatExternal == irefst)
        {
            if (kMcgLowPowerSelNormal == lp)
            {
#if FSL_FEATURE_MCG_HAS_PLL
                if (kMcgPllStatPllClkSel == pllst)
                {
                    mode = kMcgModePBE;
                }
                else
#endif
                {
                    mode = kMcgModeFBE;
                }
            }
            else
            {
                mode = kMcgModeBLPE;
            }
        }
        break;
#if FSL_FEATURE_MCG_HAS_PLL
        case kMcgClkStatPll:
        if ((kMcgInternalRefStatExternal == irefst) &&
            (kMcgPllStatPllClkSel == pllst))
        {
            mode = kMcgModePEE;
        }
        break;
#endif
        default:
        break;
    }

    return mode;
}   /* CLOCK_HAL_GetMcgMode */

/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_HAL_SetFeiMode
 * Description  : This function sets MCG to FEI mode.
 *
 *END***********************************************************************************/
mcg_mode_error_t CLOCK_HAL_SetFeiMode(uint32_t baseAddr,
                              mcg_dco_range_select_t drs,
                              void (* fllStableDelay)(void),
                              uint32_t *outClkFreq)
{
    uint32_t mcgOut;

    mcg_modes_t mode = CLOCK_HAL_GetMcgMode(baseAddr);
    if ((kMcgModeFBI != mode) &&
        (kMcgModeFBE != mode) &&
        (kMcgModeFEE != mode))
    {
        return kMcgModeErrModeUnreachable;
    }

    /* Internal slow clock is used for FLL in FEI mode. */
    /* DMX32 is recommended to be 0 in FEI mode. */
    mcgOut = CLOCK_HAL_TestFllFreq(baseAddr,
                                   g_slowInternalRefClkFreq,
                                   kMcgDmx32Default,
                                   drs);
    if (0U == mcgOut)
    {
        return kMcgModeErrFllRefRange;
    }

    /* Set DMX32. DMX32 is recommended to be 0 in FEI mode. */
    CLOCK_HAL_SetDmx32(baseAddr, kMcgDmx32Default);

    /* Set CLKS and IREFS. */
    HW_MCG_C1_WR(baseAddr, (HW_MCG_C1_RD(baseAddr) & ~(BM_MCG_C1_CLKS | BM_MCG_C1_IREFS))
                  | (BF_MCG_C1_CLKS(kMcgClkSelOut)  /* CLKS = 0 */
                  | BF_MCG_C1_IREFS(kMcgInternalRefClkSrcSlow))); /* IREFS = 1 */

    /* Wait and check status. */
    while (CLOCK_HAL_GetInternalRefStatMode(baseAddr) != kMcgInternalRefStatInternal) {}

    while (CLOCK_HAL_GetClkStatMode(baseAddr) != kMcgClkStatFll) {}
    /* Set DRS. */
    CLOCK_HAL_SetDcoRangeMode(baseAddr, drs);

    /* Wait for DRS to be set. */
    while (CLOCK_HAL_GetDcoRangeMode(baseAddr) != drs) {}

    /* Wait for FLL stable time. */
    fllStableDelay();

    *outClkFreq = mcgOut;

    return kMcgModeErrNone;
}

/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_HAL_SetFeeMode
 * Description  : This function sets MCG to FEE mode.
 *
 *END***********************************************************************************/
mcg_mode_error_t CLOCK_HAL_SetFeeMode(uint32_t baseAddr,
                              mcg_oscsel_select_t oscselVal,
                              uint8_t frdivVal,
                              mcg_dmx32_select_t dmx32,
                              mcg_dco_range_select_t drs,
                              void (* fllStableDelay)(void),
                              uint32_t *outClkFreq)
{
    uint32_t mcgOut, extFreq;

    mcg_modes_t mode = CLOCK_HAL_GetMcgMode(baseAddr);
    if ((kMcgModeFBI != mode) &&
        (kMcgModeFBE != mode) &&
        (kMcgModeFEI != mode))
    {
        return kMcgModeErrModeUnreachable;
    }

    extFreq = CLOCK_HAL_TestOscFreq(baseAddr, oscselVal);

    if (0U == extFreq)
    {
        return kMcgModeErrOscFreqRange;
    }

    /* Check whether FRDIV value is OK for current MCG setting. */
    extFreq = CLOCK_HAL_TestFllExternalRefFreq(baseAddr,
                                               extFreq,
                                               frdivVal,
                                               CLOCK_HAL_GetRange0Mode(baseAddr),
                                               oscselVal);
    if ((extFreq < kMcgConstant31250) || (extFreq > kMcgConstant39063))
    {
        return kMcgModeErrFllFrdivRange;
    }

    /* Check resulting FLL frequency  */
    mcgOut = CLOCK_HAL_TestFllFreq(baseAddr, extFreq, dmx32, drs);
    if (0U == mcgOut)
    {
        return kMcgModeErrFllRefRange;
    }

    /* Set OSCSEL */
#if FSL_FEATURE_MCG_USE_OSCSEL
    CLOCK_HAL_SetOscselMode(baseAddr, oscselVal);
#endif

    /* Set CLKS and IREFS. */
    HW_MCG_C1_WR(baseAddr, (HW_MCG_C1_RD(baseAddr) & ~(BM_MCG_C1_CLKS | BM_MCG_C1_FRDIV | BM_MCG_C1_IREFS))
                  | (BF_MCG_C1_CLKS(kMcgClkSelOut)  /* CLKS = 0 */
                  | BF_MCG_C1_FRDIV(frdivVal)  /* FRDIV */
                  | BF_MCG_C1_IREFS(kMcgInternalRefClkSrcExternal))); /* IREFS = 0 */

    /* Wait and check status. */
    while (CLOCK_HAL_GetInternalRefStatMode(baseAddr) != kMcgInternalRefStatExternal) {}

    while (CLOCK_HAL_GetClkStatMode(baseAddr) != kMcgClkStatFll) {}

    /* Set DRS and DMX32. */
    CLOCK_HAL_SetDmx32(baseAddr, dmx32);
    CLOCK_HAL_SetDcoRangeMode(baseAddr, drs);

    /* Wait for DRS to be set. */
    while (CLOCK_HAL_GetDcoRangeMode(baseAddr) != drs) {}

    /* Wait for FLL stable time. */
    fllStableDelay();

    *outClkFreq = mcgOut;

    return kMcgModeErrNone;
}

/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_HAL_SetFbiMode
 * Description  : This function sets MCG to FBI mode.
 *
 *END***********************************************************************************/
mcg_mode_error_t CLOCK_HAL_SetFbiMode(uint32_t baseAddr,
                              mcg_dco_range_select_t drs,
                              mcg_internal_ref_clock_select_t ircSelect,
                              uint8_t fcrdivVal,
                              void (* fllStableDelay)(void),
                              uint32_t *outClkFreq)
{
    mcg_modes_t mode = CLOCK_HAL_GetMcgMode(baseAddr);

    if ((kMcgModeFEI  != mode) &&
        (kMcgModeFBE  != mode) &&
        (kMcgModeFEE  != mode) &&
        (kMcgModeBLPI != mode))
    {
        return kMcgModeErrModeUnreachable;
    }

    /* Update FCRDIV if necessary. */
    CLOCK_HAL_UpdateFastClkInternalRefDiv(baseAddr, fcrdivVal);

    /* In FBI and FEI modes, setting C4[DMX32] bit is not recommended. */
    CLOCK_HAL_SetDmx32(baseAddr, kMcgDmx32Default);

    /* Set LP bit to enable the FLL */
    CLOCK_HAL_SetLowPowerMode(baseAddr, kMcgLowPowerSelNormal);

    /* Set CLKS and IREFS. */
    HW_MCG_C1_WR(baseAddr, (HW_MCG_C1_RD(baseAddr) & ~(BM_MCG_C1_CLKS | BM_MCG_C1_IREFS))
                  | (BF_MCG_C1_CLKS(kMcgClkSelInternal)  /* CLKS = 1 */
                  | BF_MCG_C1_IREFS(kMcgInternalRefClkSrcSlow))); /* IREFS = 1 */

    /* Wait for state change. */
    while (CLOCK_HAL_GetInternalRefStatMode(baseAddr) != kMcgInternalRefStatInternal) {}

    while (CLOCK_HAL_GetClkStatMode(baseAddr) != kMcgClkStatInternalRef){}

    CLOCK_HAL_SetDcoRangeMode(baseAddr, drs);

    /* Wait for DRS to be set. */
    while (CLOCK_HAL_GetDcoRangeMode(baseAddr) != drs) {}

    /* Wait for FLL stable time. */
    fllStableDelay();

    /* Select the desired IRC */
    CLOCK_HAL_SetInternalRefClkSelMode(baseAddr, ircSelect);

    /* wait until internal reference switches to requested irc. */
    if (ircSelect == kMcgInternalRefClkSelSlow)
    {
        while (CLOCK_HAL_GetInternalRefClkStatMode(baseAddr) != kMcgInternalRefClkStatSlow){}

        *outClkFreq = g_slowInternalRefClkFreq; /* MCGOUT frequency equals slow IRC frequency */
    }
    else
    {
        while (CLOCK_HAL_GetInternalRefClkStatMode(baseAddr) != kMcgInternalRefClkStatFast){}

        *outClkFreq = (g_fastInternalRefClkFreq >> fcrdivVal);
    }

    return kMcgModeErrNone;
}

/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_HAL_SetFbeMode
 * Description  : This function sets MCG to FBE mode.
 *
 *END***********************************************************************************/
mcg_mode_error_t CLOCK_HAL_SetFbeMode(uint32_t baseAddr,
                             mcg_oscsel_select_t oscselVal,
                             uint8_t frdivVal,
                             mcg_dmx32_select_t dmx32,
                             mcg_dco_range_select_t drs,
                             void (* fllStableDelay)(void),
                             uint32_t *outClkFreq)
{
    uint32_t extFreq, fllRefFreq;

    mcg_modes_t mode = CLOCK_HAL_GetMcgMode(baseAddr);
    if ((kMcgModeFBI  != mode) &&
        (kMcgModeFEI  != mode) &&
        (kMcgModeFBE  != mode) &&  // Support FLL reconfigure in FBE mode.
        (kMcgModeFEE  != mode) &&
        (kMcgModeBLPE != mode)
#if FSL_FEATURE_MCG_HAS_PLL
        && (kMcgModePBE  != mode)
#endif
        )
    {
        return kMcgModeErrModeUnreachable;
    }

    extFreq = CLOCK_HAL_TestOscFreq(baseAddr, oscselVal);

    if (0U == extFreq)
    {
        return kMcgModeErrOscFreqRange;
    }

    /* Check whether FRDIV value is OK for current MCG setting. */
    fllRefFreq = CLOCK_HAL_TestFllExternalRefFreq(baseAddr,
                                                  extFreq,
                                                  frdivVal,
                                                  CLOCK_HAL_GetRange0Mode(baseAddr),
                                                  oscselVal);

    if ((fllRefFreq < kMcgConstant31250) || (fllRefFreq > kMcgConstant39063))
    {
        return kMcgModeErrFllFrdivRange;
    }

#if FSL_FEATURE_MCG_HAS_PLL
    /* Set PLLS. */
    CLOCK_HAL_SetPllSelMode(baseAddr, kMcgPllSelFll);
    while ((CLOCK_HAL_GetPllStatMode(baseAddr) != kMcgPllStatFll)) {}
#endif

    /* Set LP bit to enable the FLL */
    CLOCK_HAL_SetLowPowerMode(baseAddr, kMcgLowPowerSelNormal);

    /* Set CLKS and IREFS. */
    HW_MCG_C1_WR(baseAddr, (HW_MCG_C1_RD(baseAddr) & ~(BM_MCG_C1_CLKS | BM_MCG_C1_FRDIV | BM_MCG_C1_IREFS))
                  | (BF_MCG_C1_CLKS(kMcgClkSelExternal)  /* CLKS = 2 */
                  | BF_MCG_C1_FRDIV(frdivVal)            /* FRDIV = frdivVal */
                  | BF_MCG_C1_IREFS(kMcgInternalRefClkSrcExternal))); /* IREFS = 0 */

    /* Wait for Reference clock Status bit to clear */
    while (kMcgInternalRefStatExternal != CLOCK_HAL_GetInternalRefStatMode(baseAddr)) {}

    /* Wait for clock status bits to show clock source is ext ref clk */
    while (CLOCK_HAL_GetClkStatMode(baseAddr) != kMcgClkStatExternalRef) {}

    /* Set OSCSEL */
#if FSL_FEATURE_MCG_USE_OSCSEL
    CLOCK_HAL_SetOscselMode(baseAddr, oscselVal);
#endif

    /* Set DRST_DRS and DMX32. */
    CLOCK_HAL_SetDcoRangeMode(baseAddr, drs);
    CLOCK_HAL_SetDmx32(baseAddr, dmx32);

    /* Wait for DRS to be set. */
    while (CLOCK_HAL_GetDcoRangeMode(baseAddr) != drs) {}

    /* Wait for fll stable time. */
    fllStableDelay();

    /* MCGOUT frequency equals external clock frequency */
    *outClkFreq = extFreq;

    return kMcgModeErrNone;
}

/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_HAL_SetBlpiMode
 * Description  : This function sets MCG to BLPI mode.
 *
 *END***********************************************************************************/
mcg_mode_error_t CLOCK_HAL_SetBlpiMode(uint32_t baseAddr,
                               uint8_t fcrdivVal,
                               mcg_internal_ref_clock_select_t ircSelect,
                               uint32_t *outClkFreq)
{
    mcg_modes_t mode = CLOCK_HAL_GetMcgMode(baseAddr);
    if ((kMcgModeFBI != mode))
    {
        return kMcgModeErrModeUnreachable;
    }

    /* Update FCRDIV if necessary. */
    CLOCK_HAL_UpdateFastClkInternalRefDiv(baseAddr, fcrdivVal);

    /* Set LP bit to disable the FLL and enter BLPI */
    CLOCK_HAL_SetLowPowerMode(baseAddr, kMcgLowPowerSelLowPower);

    /* Now in BLPI */
    if (ircSelect == kMcgInternalRefClkSelFast)
    {
        *outClkFreq = (g_fastInternalRefClkFreq >> fcrdivVal);
    }
    else
    {
        *outClkFreq = g_slowInternalRefClkFreq;
    }

    return kMcgModeErrNone;
}

/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_HAL_SetBlpeMode
 * Description  : This function sets MCG to BLPE mode.
 *
 *END***********************************************************************************/
mcg_mode_error_t CLOCK_HAL_SetBlpeMode(uint32_t baseAddr,
                               mcg_oscsel_select_t oscselVal,
                               uint32_t *outClkFreq)
{
    uint32_t extFreq;

    mcg_modes_t mode = CLOCK_HAL_GetMcgMode(baseAddr);
    if ((kMcgModeFBE != mode)
#if FSL_FEATURE_MCG_HAS_PLL
        && (kMcgModePBE != mode)
#endif
       )
    {
        return kMcgModeErrModeUnreachable;
    }

    /* External OSC must be initialized before this function. */
    extFreq = CLOCK_HAL_TestOscFreq(baseAddr, oscselVal);

    if (0U == extFreq)
    {
        return kMcgModeErrOscFreqRange;
    }

    /* Set LP bit to enter BLPE mode. */
    CLOCK_HAL_SetLowPowerMode(baseAddr, kMcgLowPowerSelLowPower);

    /* Set OSCSEL */
#if FSL_FEATURE_MCG_USE_OSCSEL
    CLOCK_HAL_SetOscselMode(baseAddr, oscselVal);
#endif

    *outClkFreq = extFreq;
    return kMcgModeErrNone;
}

#if FSL_FEATURE_MCG_HAS_PLL
/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_HAL_SetPbeMode
 * Description  : This function sets MCG to PBE mode.
 *
 *END***********************************************************************************/
mcg_mode_error_t CLOCK_HAL_SetPbeMode(uint32_t baseAddr,
                              mcg_oscsel_select_t oscselVal,
                              mcg_pll_clk_select_t pllcsSelect,
                              uint8_t prdivVal,
                              uint8_t vdivVal,
                              uint32_t *outClkFreq)
#if FSL_FEATURE_MCG_HAS_PLL1
{
    uint32_t extFreq, pllFreq;

    mcg_modes_t mode = CLOCK_HAL_GetMcgMode(baseAddr);
    if ((kMcgModeFBE  != mode) &&
        (kMcgModePEE  != mode) &&
        (kMcgModePBE  != mode) && // Support PLL reconfigure in PBE mode.
        (kMcgModeBLPE != mode))
    {
        return kMcgModeErrModeUnreachable;
    }

    /* External OSC must be initialized before this function. */
    extFreq = CLOCK_HAL_TestOscFreq(baseAddr, oscselVal);

    if (0U == extFreq)
    {
        return kMcgModeErrOscFreqRange;
    }

    pllFreq = CLOCK_HAL_GetPll0RefFreq(baseAddr);

    pllFreq = extFreq / (prdivVal + FSL_FEATURE_MCG_PLL_PRDIV_BASE);

    if ((pllRefFreq < FSL_FEATURE_MCG_PLL_REF_MIN) ||
        (pllRefFreq > FSL_FEATURE_MCG_PLL_REF_MAX))
    {
        return kMcgErrPllPrdivRange;
    }

    /* Set LP bit to enable the FLL */
    CLOCK_HAL_SetLowPowerMode(baseAddr, kMcgLowPowerSelNormal);

    /* Change to use external clock first. */
    CLOCK_HAL_SetClkSrcMode(baseAddr, kMcgClkSelExternal);

    /* Wait for clock status bits to update */
    while (CLOCK_HAL_GetClkStatMode(baseAddr) != kMcgClkStatExternalRef) {}

    /* Set OSCSEL */
#if FSL_FEATURE_MCG_USE_OSCSEL
    CLOCK_HAL_SetOscselMode(baseAddr, oscselVal);
#endif

    /* Set PLLS to select PLL. */
    CLOCK_HAL_SetPllSelMode(baseAddr, kMcgPllSelPllClkSel);
    // wait for PLLST status bit to set
    while ((CLOCK_HAL_GetPllStatMode(baseAddr) != kMcgPllStatPllClkSel)) {}

    CLOCK_HAL_SetPllcs(pllcsSelect);
    while (pllcsSelect != CLOCK_HAL_GetPllClkSelStatMode(baseAddr)) {}

    /* Set PRDIV and VDIV. */
    if (pllcsSelect == kMcgPllcsSelectPll1)
    {
        CLOCK_HAL_SetPllExternalRefDiv1(baseAddr, prdivVal);
        CLOCK_HAL_SetVoltCtrlOscDiv1(baseAddr, vdivVal);
        /* Check lock. */
        while ((CLOCK_HAL_GetLock1Mode(baseAddr) !=  kMcgLockLocked)) {}
    }
    else
    {
        CLOCK_HAL_SetPllExternalRefDiv0(baseAddr, prdivVal);
        CLOCK_HAL_SetVoltCtrlOscDiv0(baseAddr, vdivVal);
        /* Check lock. */
        while ((CLOCK_HAL_GetLock0Mode(baseAddr) !=  kMcgLockLocked)) {}
    }

    *outClkFreq = extFreq;
    return kMcgModeErrNone;
}
#else
{
    uint32_t extFreq, pllRefFreq;

    mcg_modes_t mode = CLOCK_HAL_GetMcgMode(baseAddr);
    if ((kMcgModeFBE  != mode) &&
        (kMcgModePEE  != mode) &&
        (kMcgModeBLPE != mode))
    {
        return kMcgModeErrModeUnreachable;
    }

    /* External OSC must be initialized before this function. */
    extFreq = CLOCK_HAL_TestOscFreq(baseAddr, oscselVal);

    if (0U == extFreq)
    {
        return kMcgModeErrOscFreqRange;
    }

    /* Check whether PRDIV value is OK. */
    pllRefFreq = extFreq / (prdivVal + FSL_FEATURE_MCG_PLL_PRDIV_BASE);

    if ((pllRefFreq < FSL_FEATURE_MCG_PLL_REF_MIN) ||
        (pllRefFreq > FSL_FEATURE_MCG_PLL_REF_MAX))
    {
        return kMcgModeErrPllPrdivRange;
    }

    /* Set LP bit to enable the FLL */
    CLOCK_HAL_SetLowPowerMode(baseAddr, kMcgLowPowerSelNormal);

    /* Change to use external clock first. */
    CLOCK_HAL_SetClkSrcMode(baseAddr, kMcgClkSelExternal);

    /* Wait for clock status bits to update */
    while (CLOCK_HAL_GetClkStatMode(baseAddr) != kMcgClkStatExternalRef) {}

    /* Set OSCSEL */
#if FSL_FEATURE_MCG_USE_OSCSEL
    CLOCK_HAL_SetOscselMode(baseAddr, oscselVal);
#endif

    CLOCK_HAL_SetPllSelMode(baseAddr, kMcgPllSelPllClkSel);

    // wait for PLLST status bit to set
    while ((CLOCK_HAL_GetPllStatMode(baseAddr) != kMcgPllStatPllClkSel)) {}

    CLOCK_HAL_SetPllExternalRefDiv0(baseAddr, prdivVal);

    CLOCK_HAL_SetVoltCtrlOscDiv0(baseAddr, vdivVal);

    /* Wait for LOCK bit to set */
    while ((CLOCK_HAL_GetLock0Mode(baseAddr) !=  kMcgLockLocked)) {}

    *outClkFreq = extFreq;
    return kMcgModeErrNone;
}
#endif

/*FUNCTION******************************************************************************
 *
 * Functon name : CLOCK_HAL_SetPeeMode
 * Description  : This function sets MCG to PEE mode.
 *
 *END***********************************************************************************/
mcg_mode_error_t CLOCK_HAL_SetPeeMode(uint32_t baseAddr,
                                      uint32_t *outClkFreq)
{
    mcg_modes_t mode = CLOCK_HAL_GetMcgMode(baseAddr);
    if (kMcgModePBE != mode)
    {
        return kMcgModeErrModeUnreachable;
    }

    /* Change to use PLL/FLL output clock first. */
    CLOCK_HAL_SetClkSrcMode(baseAddr, kMcgClkSelOut);

    /* Wait for clock status bits to update */
    while (CLOCK_HAL_GetClkStatMode(baseAddr) != kMcgClkStatPll) {}

    *outClkFreq = CLOCK_HAL_GetOutClk(baseAddr);

    return kMcgModeErrNone;
}

#endif

