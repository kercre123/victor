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

#include "fsl_mcg_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Code
 ******************************************************************************/

/* This frequency values should be set by different boards. */
uint32_t g_xtal0ClkFreq;           /* EXTAL0 clock */
#if FSL_FEATURE_MCG_HAS_OSC1
uint32_t g_xtal1ClkFreq;           /* EXTAL1 clock */
#endif
uint32_t g_xtalRtcClkFreq;         /* EXTAL RTC clock */

uint32_t g_fastInternalRefClkFreq = 4000000U;
uint32_t g_slowInternalRefClkFreq = 32768U;


/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_TestOscFreq
 * Description   : This function checks MCG external OSC clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_TestOscFreq(uint32_t baseAddr, mcg_oscsel_select_t oscselVal)
{
    uint32_t extFreq;

#if FSL_FEATURE_MCG_USE_OSCSEL
    switch (oscselVal)
    {
        case kMcgOscselOsc:         /* Selects System Oscillator (OSCCLK) */
            extFreq = g_xtal0ClkFreq;
            break;
#if FSL_FEATURE_MCG_HAS_RTC_32K
        case kMcgOscselRtc:         /* Selects 32 kHz RTC Oscillator */
            extFreq = g_xtalRtcClkFreq;
            break;
#endif
#if FSL_FEATURE_MCG_HAS_IRC_48M
        case kMcgOscselIrc:         /* Selects 48 MHz IRC Oscillator */
            extFreq = CPU_INTERNAL_IRC_48M;
            break;
#endif
        default:
            extFreq = 0U;
            break;
    }
#else
    extFreq = g_xtal0ClkFreq;
#endif
    return extFreq;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetMcgExternalClkFreq
 * Description   : This is an internal function to get the MCG external clock
 * frequency. MCG external clock could be OSC0, RTC or IRC48M, choosed by
 * register OSCSEL.
 *
 *END**************************************************************************/
static uint32_t CLOCK_HAL_GetMcgExternalClkFreq(uint32_t baseAddr)
{
#if FSL_FEATURE_MCG_USE_OSCSEL
    /* OSC frequency selected by OSCSEL. */
    return CLOCK_HAL_TestOscFreq(baseAddr, CLOCK_HAL_GetOscselMode(baseAddr));
#else
    /* Use default osc0*/
    return g_xtal0ClkFreq;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_TestFllExternalRefFreq
 * Description   : Calculates the Fll external reference clock frequency based
 * on input parameters.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_TestFllExternalRefFreq(uint32_t baseAddr,
                                          uint32_t extFreq,
                                          uint8_t  frdivVal,
                                          mcg_freq_range_select_t range0,
                                          mcg_oscsel_select_t oscsel)
{
    extFreq >>= frdivVal;

    if ((kMcgFreqRangeSelLow != range0)
#if FSL_FEATURE_MCG_USE_OSCSEL
      && (kMcgOscselRtc != oscsel)
#endif
        )
    {
        switch (frdivVal)
        {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                extFreq >>= 5U;
                break;
#if FSL_FEATURE_MCG_FRDIV_SUPPORT_1280
            case 6:
                extFreq /= 20U; /* 64*20=1280 */
                break;
#endif
#if FSL_FEATURE_MCG_FRDIV_SUPPORT_1536
            case 7:
                extFreq /= 12U; /* 128*12=1536 */
                break;
#endif
            default:
                extFreq = 0U; /* Reserved. */
                break;
        }
    }
    return extFreq;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetFllRefclk
 * Description   : Internal function to find the fll reference clock
 * This is an internal function to get the fll reference clock. The returned
 * value will be used for other APIs to calculate teh fll and other clock value.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_GetFllRefClk(uint32_t baseAddr)
{
    uint32_t mcgffclk;

    if (CLOCK_HAL_GetInternalRefSelMode(baseAddr) == kMcgInternalRefClkSrcExternal)
    {
        /* External reference clock is selected */
        mcgffclk = CLOCK_HAL_GetMcgExternalClkFreq(baseAddr);

#if FSL_FEATURE_MCG_USE_OSCSEL
        mcgffclk = CLOCK_HAL_TestFllExternalRefFreq(baseAddr, mcgffclk,
                                CLOCK_HAL_GetFllExternalRefDiv(baseAddr),
                                CLOCK_HAL_GetRange0Mode(baseAddr),
                                CLOCK_HAL_GetOscselMode(baseAddr));
#else
        mcgffclk = CLOCK_HAL_TestFllExternalRefFreq(baseAddr, mcgffclk,
                                CLOCK_HAL_GetFllExternalRefDiv(baseAddr),
                                CLOCK_HAL_GetRange0Mode(baseAddr),
                                kMcgOscselOsc);
#endif

    }
    else
    {
        /* The slow internal reference clock is selected */
        mcgffclk = g_slowInternalRefClkFreq;
    }
    return mcgffclk;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_TestFllFreq
 * Description   : Calculate the Fll frequency based on input parameters.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_TestFllFreq(uint32_t baseAddr,
                               uint32_t fllRef,
                               mcg_dmx32_select_t dmx32,
                               mcg_dco_range_select_t drs)
{
    static const uint16_t fllFactorTable[4][2] = {
        {640,  732},
        {1280, 1464},
        {1920, 2197},
        {2560, 2929}
    };

    /* if DMX32 set */
    if (dmx32)
    {
        if (fllRef > kMcgConstant32768)
        {
            return 0U;
        }
    }
    else
    {
        if ((fllRef < kMcgConstant31250) || (fllRef > kMcgConstant39063))
        {
            return 0U;
        }
    }

    return fllRef * fllFactorTable[drs][dmx32];
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetFllclk
 * Description   : Get the current mcg fll clock
 * This function will return the mcgfllclk value in frequency(hz) based on
 * current mcg configurations and settings. Fll should be properly configured
 * in order to get the valid value.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_GetFllClk(uint32_t baseAddr)
{
    uint32_t mcgfllclk;

    mcgfllclk = CLOCK_HAL_GetFllRefClk(baseAddr);

    if (0U == mcgfllclk)
    {
        return 0U;
    }

    mcgfllclk = CLOCK_HAL_TestFllFreq(baseAddr, mcgfllclk,
                    CLOCK_HAL_GetDmx32(baseAddr),
                    CLOCK_HAL_GetDcoRangeMode(baseAddr));

    return mcgfllclk;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_UpdateFastClkInternalRefDiv
 * Description   : This fucntion sets FCRDIV to a new value. FCRDIV can not be
 * changed when fast internal reference is enabled, this function checks the
 * status, if it is enabled, disable it first, then set FCRDIV, at last reenable
 * it. If you can make sure fast internal reference is not enabled, call
 * CLOCK_HAL_SetFastClkInternalRefDiv() will be more effective.
 *
 *END**************************************************************************/
void CLOCK_HAL_UpdateFastClkInternalRefDiv(uint32_t baseAddr, uint8_t fcrdiv)
{
    /* If new value equals current value, do not update. */
    if (CLOCK_HAL_GetFastClkInternalRefDiv(baseAddr) != fcrdiv)
    {
        /* If fast internal reference clock is not used, change directly. */
        if (kMcgInternalRefClkSelSlow == CLOCK_HAL_GetInternalRefClkSelMode(baseAddr))
        {
            CLOCK_HAL_SetFastClkInternalRefDiv(baseAddr, fcrdiv);
        }
        else /* If it is used, swith to slow IRC, then change FCRDIV. */
        {
            /* Switch to slow IRC. */
            CLOCK_HAL_SetInternalRefClkSelMode(baseAddr, kMcgInternalRefClkSelSlow);
            while (kMcgInternalRefClkStatSlow != CLOCK_HAL_GetInternalRefClkStatMode(baseAddr)) {}
            /* Set new value. */
            CLOCK_HAL_SetFastClkInternalRefDiv(baseAddr, fcrdiv);
            /* Switch to fast IRC. */
            CLOCK_HAL_SetInternalRefClkSelMode(baseAddr, kMcgInternalRefClkSelFast);
            while (kMcgInternalRefClkStatFast != CLOCK_HAL_GetInternalRefClkStatMode(baseAddr)) {}
        }
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetAvailableFrdiv
 * Description   : This fucntion calculates the proper FRDIV setting according
 * to FLL reference clock.
 *
 *END**************************************************************************/
mcg_status_t CLOCK_HAL_GetAvailableFrdiv(mcg_freq_range_select_t range0,
                                    mcg_oscsel_select_t oscsel,
                                    uint32_t inputFreq,
                                    uint8_t  *frdiv)
{
    *frdiv = 0U;
    static const uint16_t freq_kHz[] = {
        1250U, 2500U, 5000U, 10000U, 20000U, 40000U,
#if FSL_FEATURE_MCG_FRDIV_SUPPORT_1280
        50000U,
#endif
#if FSL_FEATURE_MCG_FRDIV_SUPPORT_1536
        60000U
#endif
    };

    if ((kMcgFreqRangeSelLow != range0)
#if FSL_FEATURE_MCG_USE_OSCSEL
        && (kMcgOscselRtc != oscsel)
#endif
       )
    {
        inputFreq /= 1000U;
        while (*frdiv < (sizeof(freq_kHz)/sizeof(freq_kHz[0])))
        {
            if (inputFreq < freq_kHz[*frdiv])
            {
                return kStatus_MCG_Success;
            }
            (*frdiv)++;
        }
    }
    else
    {
        while (inputFreq > 39063U)
        {
            inputFreq >>= 1U;
            (*frdiv)++;
        }
        if ((*frdiv) < 8U)
        {
            return kStatus_MCG_Success;
        }
    }
    return kStatus_MCG_Fail;
}

#if FSL_FEATURE_MCG_HAS_PLL

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_CalculatePllDiv
 * Description   : Calculates the PLL PRVID and VDIV.
 * This function calculates the proper PRDIV and VDIV to generate desired PLL
 * output frequency with input reference clock frequency. It returns the closest
 * frequency PLL could generate, the corresponding PRDIV/VDIV are returned from
 * parameters. If desire frequency is not valid, this funtion returns 0.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_CalculatePllDiv(uint32_t refFreq,
                                   uint32_t desireFreq,
                                   uint8_t *prdiv,
                                   uint8_t *vdiv)
{
    uint8_t ret_prdiv, ret_vdiv;
    uint8_t prdiv_min, prdiv_max, prdiv_cur;
    uint8_t vdiv_cur;
    uint32_t ret_freq = 0U;
    uint32_t diff = 0xFFFFFFFFU; // Difference between desireFreq and return frequency.
    uint32_t ref_div; // Reference frequency after PRDIV.

    /* Reference frequency is out of range. */
    if ((refFreq < FSL_FEATURE_MCG_PLL_REF_MIN) ||
        (refFreq > (FSL_FEATURE_MCG_PLL_REF_MAX *
                   (FSL_FEATURE_MCG_PLL_PRDIV_MAX + FSL_FEATURE_MCG_PLL_PRDIV_BASE))))
    {
        return 0U;
    }

    /* refFreq/PRDIV must in a range. First get the allowed PRDIV range. */
    prdiv_max = refFreq / FSL_FEATURE_MCG_PLL_REF_MIN;
    prdiv_min = (refFreq + FSL_FEATURE_MCG_PLL_REF_MAX - 1U) / FSL_FEATURE_MCG_PLL_REF_MAX;

#if FSL_FEATURE_MCG_HAS_PLL_INTERNAL_DIV
    desireFreq *= 2U;
#endif

    /* PRDIV traversal. */
    for (prdiv_cur=prdiv_max; prdiv_cur>=prdiv_min; prdiv_cur--)
    {
        // Reference frequency after PRDIV.
        ref_div = refFreq / prdiv_cur;

        vdiv_cur = desireFreq / ref_div;

        if ((vdiv_cur < FSL_FEATURE_MCG_PLL_VDIV_BASE-1U) ||
            (vdiv_cur > FSL_FEATURE_MCG_PLL_VDIV_BASE+31U))
        {
            /* No VDIV is available with this PRDIV. */
            continue;
        }

        ret_freq = vdiv_cur * ref_div;

        if (vdiv_cur >= FSL_FEATURE_MCG_PLL_VDIV_BASE)
        {
            if (ret_freq == desireFreq) // If desire frequency is got.
            {
                *prdiv = prdiv_cur  - FSL_FEATURE_MCG_PLL_PRDIV_BASE;
                *vdiv  = vdiv_cur   - FSL_FEATURE_MCG_PLL_VDIV_BASE;
#if FSL_FEATURE_MCG_HAS_PLL_INTERNAL_DIV
                return ret_freq / 2U;
#else
                return ret_freq;
#endif
            }
            if (diff > desireFreq - ret_freq) // New PRDIV/VDIV is closer.
            {
                diff = desireFreq - ret_freq;
                ret_prdiv = prdiv_cur;
                ret_vdiv  = vdiv_cur;
            }
        }
        vdiv_cur ++;
        if (vdiv_cur <= (FSL_FEATURE_MCG_PLL_VDIV_BASE+31U))
        {
            ret_freq += ref_div;
            if (diff > ret_freq - desireFreq) // New PRDIV/VDIV is closer.
            {
                diff = ret_freq - desireFreq;
                ret_prdiv = prdiv_cur;
                ret_vdiv  = vdiv_cur;
            }
        }
    }

    if (0xFFFFFFFFU != diff)
    {
        /* PRDIV/VDIV found. */
        *prdiv = ret_prdiv - FSL_FEATURE_MCG_PLL_PRDIV_BASE;
        *vdiv  = ret_vdiv  - FSL_FEATURE_MCG_PLL_VDIV_BASE;
#if FSL_FEATURE_MCG_HAS_PLL_INTERNAL_DIV
        return ret_freq / 2U;
#else
        return ret_freq;
#endif
    }
    else
    {
        return 0U; // No proper PRDIV/VDIV found.
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetPll0RefFreq
 * Description   : Get PLL0 external reference clock frequency, it could be
 * selected by OSCSEL or PLLREFSEL, according to chip design.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_GetPll0RefFreq(uint32_t baseAddr)
{
#if FSL_FEATURE_MCG_HAS_PLL1
    /* Use dedicate source. */
    if (kMcgPllExternalRefClkSelOsc0 == CLOCK_HAL_GetPllRefSel0Mode(baseAddr))
    {
        return g_xtal0ClkFreq;
    }
    else
    {
        return g_xtal1ClkFreq;
    }
#else
    /* Use OSCSEL frequency. */
    return CLOCK_HAL_GetMcgExternalClkFreq(baseAddr);
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetPll0clk
 * Description   : Get the current mcg pll/pll0 clock
 * This function will return the mcgpllclk/mcgpll0 value in frequency(hz) based
 * on current mcg configurations and settings. PLL/PLL0 should be properly
 * configured in order to get the valid value.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_GetPll0Clk(uint32_t baseAddr)
{
    uint32_t mcgpll0clk;
    uint8_t  divider;

    mcgpll0clk = CLOCK_HAL_GetPll0RefFreq(baseAddr);

    divider = (FSL_FEATURE_MCG_PLL_PRDIV_BASE + CLOCK_HAL_GetPllExternalRefDiv0(baseAddr));

    /* Calculate the PLL reference clock*/
    mcgpll0clk /= divider;
    divider = (CLOCK_HAL_GetVoltCtrlOscDiv0(baseAddr) + FSL_FEATURE_MCG_PLL_VDIV_BASE);

    /* Calculate the MCG output clock*/
    mcgpll0clk = (mcgpll0clk * divider);

#if FSL_FEATURE_MCG_HAS_PLL_INTERNAL_DIV
    mcgpll0clk >>= 1U;
#endif
    return mcgpll0clk;
}
#endif

#if FSL_FEATURE_MCG_HAS_PLL1
/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetPll1RefFreq
 * Description   : Get PLL1 external reference clock frequency, it is
 * selected by PLLREFSEL.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_GetPll1RefFreq(uint32_t baseAddr)
{
    if (kMcgPllExternalRefClkSelOsc0 == CLOCK_HAL_GetPllRefSel1Mode(baseAddr))
    {
        return g_xtal0ClkFreq;
    }
    else
    {
        return g_xtal1ClkFreq;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetPll1Clk
 * Description   : Get the current mcg pll1 clock
 * This function will return the mcgpll1clk value in frequency(hz) based
 * on current mcg configurations and settings. PLL1 should be properly configured
 * in order to get the valid value.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_GetPll1Clk(uint32_t baseAddr)
{
    uint32_t mcgpll1clk;
    uint8_t  divider;

    mcgpll1clk = CLOCK_HAL_GetPll1RefFreq(baseAddr);

    divider = (FSL_FEATURE_MCG_PLL_PRDIV_BASE + CLOCK_HAL_GetPllExternalRefDiv1(baseAddr));

    /* Calculate the PLL reference clock*/
    mcgpll1clk /= divider;
    divider = (CLOCK_HAL_GetVoltCtrlOscDiv1(baseAddr) + FSL_FEATURE_MCG_PLL_VDIV_BASE);

    /* Calculate the MCG output clock*/
    mcgpll1clk = (mcgpll1clk * divider); /* divided by 2*/
#if FSL_FEATURE_MCG_HAS_PLL_INTERNAL_DIV
    mcgpll1clk >>= 1U;
#endif
    return mcgpll1clk;
}
#endif

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetInternalRefClk
 * Description   : Get the current mcg internal reference clock (MCGIRCLK)
 * This function will return the mcgirclk value in frequency(hz) based
 * on current mcg configurations and settings. It will not check if the
 * mcgirclk is enabled or not, just calculate and return the value.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_GetInternalRefClk(uint32_t baseAddr)
{
    uint32_t mcgirclk;

    if (!CLOCK_HAL_GetInternalClkCmd(baseAddr))
    {
        return 0U;
    }

    if (CLOCK_HAL_GetInternalRefClkSelMode(baseAddr) == kMcgInternalRefClkSelSlow)
    {
        /* Slow internal reference clock selected*/
        mcgirclk = g_slowInternalRefClkFreq;
    }
    else
    {
        mcgirclk = g_fastInternalRefClkFreq >> CLOCK_HAL_GetFastClkInternalRefDiv(baseAddr);
    }
    return mcgirclk;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetFixedFreqClk
 * Description   : Get the MCGFFCLK frequency.
 * This function get the MCGFFCLK, it is only valid when its frequency is not
 * more than MCGOUTCLK/8. If MCGFFCLK is invalid, this function returns 0.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_GetFixedFreqClk(uint32_t baseAddr)
{
    uint32_t freq = CLOCK_HAL_GetFllRefClk(baseAddr);

    /* MCGFFCLK must be no more than MCGOUTCLK/8. */
    if ((!freq) && (freq <= (CLOCK_HAL_GetOutClk(baseAddr)/8U)))
    {
        return freq;
    }
    else
    {
        return 0U;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetOutclk
 * Description   : Get the current mcg out clock
 * This function will return the mcgoutclk value in frequency(hz) based on
 * current mcg configurations and settings. The configuration should be
 * properly done in order to get the valid value.
 *
 *END**************************************************************************/
uint32_t CLOCK_HAL_GetOutClk(uint32_t baseAddr)
{
    uint32_t mcgoutclk;
    mcg_clock_select_t src = CLOCK_HAL_GetClkSrcMode(baseAddr);

    switch (src)
    {
        case kMcgClkSelOut: /* PLL or FLL. */
#if FSL_FEATURE_MCG_HAS_PLL
            if (CLOCK_HAL_GetPllSelMode(baseAddr) == kMcgPllSelPllClkSel)
            {
#if FSL_FEATURE_MCG_HAS_PLL1
                if (CLOCK_HAL_GetPllClkSelMode(baseAddr) == kMcgPllClkSelPll1)
                {
                    mcgoutclk = CLOCK_HAL_GetPll1Clk(baseAddr);
                }
                else
#endif
                {
                    mcgoutclk = CLOCK_HAL_GetPll0Clk(baseAddr);
                }
            }
            else
#endif
            {
                mcgoutclk = CLOCK_HAL_GetFllClk(baseAddr);
            }
            break;
        case kMcgClkSelInternal:  /* Internal clock. */
            mcgoutclk = CLOCK_HAL_GetInternalRefClk(baseAddr);
            break;
        case kMcgClkSelExternal:  /* External clock. */
            mcgoutclk = CLOCK_HAL_GetMcgExternalClkFreq(baseAddr);
            break;
        default:
            mcgoutclk = 0U;
            break;
    }
    return mcgoutclk;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_SetOsc0Mode
 * Description   : Set the OSC0 work mode.
 * This function set OSC0 work mode, include frequency range select, high gain
 * oscillator select and external reference select.
 *
 *END**************************************************************************/
void CLOCK_HAL_SetOsc0Mode(uint32_t baseAddr,
                           mcg_freq_range_select_t range,
                           mcg_high_gain_osc_select_t hgo,
                           mcg_external_ref_clock_select_t erefs)
{
    CLOCK_HAL_SetRange0Mode(baseAddr, range);
    CLOCK_HAL_SetHighGainOsc0Mode(baseAddr, hgo);
    CLOCK_HAL_SetExternalRefSel0Mode(baseAddr, erefs);
}

#if FSL_FEATURE_MCG_HAS_OSC1
/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_SetOsc1Mode
 * Description   : Set the OSC1 work mode.
 * This function set OSC1 work mode, include frequency range select, high gain
 * oscillator select and external reference select.
 *
 *END**************************************************************************/
void CLOCK_HAL_SetOsc1Mode(uint32_t baseAddr,
                           mcg_freq_range_select_t range,
                           mcg_high_gain_osc_select_t hgo,
                           mcg_external_ref_clock_select_t erefs)
{
    CLOCK_HAL_SetRange1Mode(baseAddr, range);
    CLOCK_HAL_SetHighGainOsc1Mode(baseAddr, hgo);
    CLOCK_HAL_SetExternalRefSel1Mode(baseAddr, erefs);
}
#endif

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_SetAutoTrimMachineCmpVal
 * Description   : This function sets the auto trim machine compare value.
 *
 *END**************************************************************************/
void CLOCK_HAL_SetAutoTrimMachineCmpVal(uint32_t baseAddr, uint16_t value)
{
    BW_MCG_ATCVL_ATCVL(baseAddr, (value & 0xFFU));
    BW_MCG_ATCVH_ATCVH(baseAddr, ((value >> 8U) & 0xFFU));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetAutoTrimMachineCmpVal
 * Description   : This function gets the auto trim machine compare value.
 *
 *END**************************************************************************/
uint16_t CLOCK_HAL_GetAutoTrimMachineCompVal(uint32_t baseAddr)
{
    uint16_t ret;

    ret = BR_MCG_ATCVH_ATCVH(baseAddr);
    ret <<= 8U;
    ret |= BR_MCG_ATCVL_ATCVL(baseAddr);
    return ret;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_TrimInternalRefClk
 * Description   : Trim internal reference clock to a desire frequency.
 * The external frequency is the BUS clock frequency and must be in the range
 * of 8MHz to 16MHz.
 *
 *END**************************************************************************/
mcg_atm_error_t CLOCK_HAL_TrimInternalRefClk(uint32_t  baseAddr,
                                             uint32_t  extFreq,
                                             uint32_t  desireFreq,
                                             uint32_t* actualFreq,
                                             mcg_atm_select_t atms)
{
    uint32_t multi;    /* extFreq / desireFreq */
    uint32_t actv;     /* Auto trim value. */

    if ((extFreq > kMcgConstant16000000) ||
        (extFreq < kMcgConstant8000000))
    {
        return kMcgAtmErrorBusClockRange;
    }

    /*
     * Make sure internal reference clock is not used to generate bus clock.
     * Just need to check C1[IREFS].
     */
    if (kMcgInternalRefClkSrcSlow == CLOCK_HAL_GetInternalRefSelMode(baseAddr))
    {
        return kMcgAtmErrorIrcUsed;
    }

    multi  = extFreq / desireFreq;
    actv = multi * 21U;

    if (kMcgAtmSel4m == atms)
    {
        actv *= 128U;
    }

    /* Now begin to start trim. */
    CLOCK_HAL_SetAutoTrimMachineCmpVal(baseAddr, (uint16_t)actv);

    CLOCK_HAL_SetAutoTrimMachineSelMode(baseAddr, atms);

    CLOCK_HAL_SetAutoTrimMachineCmd(baseAddr, true);

    while (CLOCK_HAL_GetAutoTrimMachineCmd(baseAddr)) {}

    /* Error occurs? */
    if (kMcgAutoTrimMachineNormal != CLOCK_HAL_GetAutoTrimMachineFailStatus(baseAddr))
    {
        return kMcgAtmErrorHardwareFail;
    }

    *actualFreq = extFreq / multi;

    if (kMcgAtmSel4m == atms)
    {
        g_fastInternalRefClkFreq = *actualFreq;
    }
    else
    {
        g_slowInternalRefClkFreq = *actualFreq;
    }

    return kMcgAtmErrorNone;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/

