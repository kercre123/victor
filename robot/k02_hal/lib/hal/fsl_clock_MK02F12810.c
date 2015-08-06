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

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "fsl_device_registers.h"
#include "fsl_mcg_hal.h"
#include "fsl_sim_hal.h"
#include "fsl_osc_hal.h"
#include "fsl_lptmr_hal.h"
#include "fsl_clock_manager.h"

/*
 * README:
 * This file should provide these APIs:
 * 1. APIs to get the frequency of output clocks in Reference Manual ->
 * Chapter Clock Distribution -> Figure Clocking diagram.
 * 2. APIs for IP modules listed in Reference Manual -> Chapter Clock Distribution
 * -> Module clocks.
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
uint32_t g_ftmClkFreq[FTM_EXT_CLK_COUNT];          /* FTM_CLK          */

/* Default pre-defined clock configurations. */
clock_manager_user_config_t g_defaultClockConfigurations[CLOCK_CONFIG_NUM] =
{
    /* Configuration for enter VLPR mode. Core clock = 4MHz. */
    {
        .mcgConfig =
        {
            .mcg_mode           = kMcgModeBLPI,   // Work in BLPI mode.
            .irclkEnable        = true,  // MCGIRCLK enable.
            .irclkEnableInStop  = false, // MCGIRCLK disable in STOP mode.
            .ircs               = kMcgInternalRefClkSelFast, // Select IRC4M.
            .fcrdiv             = 0U,    // FCRDIV is 0.

            .frdiv   = 0U,
            .drs     = kMcgDcoRangeSelLow,  // Low frequency range
            .dmx32   = kMcgDmx32Default,    // DCO has a default range of 25%
            .oscsel  = kMcgOscselOsc,       // Select OSC

        },
        .simConfig =
        {
            .PllFllSel = kClockPllFllSelIrc48M, // PLLFLLSEL select IRC48MCLK.
            .er32kSrc  = kClockEr32kSrcLpo,     // ERCLK32K selection, use LPO.
            .outdiv1   = 0U,
            .outdiv2   = 0U,
            .outdiv4   = 4U,
        },
        .oscerConfig =
        {
            .Enable       = false, // OSCERCLK disable.
            .EnableInStop = false, // OSCERCLK disable in STOP mode.
        }
    },

    /* Configuration for enter RUN mode. Core clock = 60MHz. */
    {
        .mcgConfig =
        {
            .mcg_mode           = kMcgModeFEE,   // Work in FEE mode.
            .irclkEnable        = true,  // MCGIRCLK enable.
            .irclkEnableInStop  = false, // MCGIRCLK disable in STOP mode.
            .ircs               = kMcgInternalRefClkSelSlow, // Select IRC32k.
            .fcrdiv             = 0U,    // FCRDIV is 0.

            .frdiv   = 3U,
            .drs     = kMcgDcoRangeSelMidHigh,  // Mid high frequency range
            .dmx32   = kMcgDmx32Default,        // DCO has a default range of 25%
            .oscsel  = kMcgOscselOsc,           // Select OSC
        },
        .simConfig =
        {
            .PllFllSel = kClockPllFllSelFll,    // PLLFLLSEL select FLL.
            .er32kSrc  = kClockEr32kSrcLpo,     // ERCLK32K selection, use LPO.
            .outdiv1   = 0U,
            .outdiv2   = 1U,
            .outdiv4   = 2U,
        },
        .oscerConfig =
        {
            .Enable       = true,  // OSCERCLK enable.
            .EnableInStop = false, // OSCERCLK disable in STOP mode.
        }
    },

    /* Configuration for HSRUN mode. Core clock = 80MHz. */
    {
        .mcgConfig =
        {
            .mcg_mode           = kMcgModeFEE,   // Work in FEE mode.
            .irclkEnable        = true,  // MCGIRCLK enable.
            .irclkEnableInStop  = false, // MCGIRCLK disable in STOP mode.
            .ircs               = kMcgInternalRefClkSelSlow, // Select IRC32k.
            .fcrdiv             = 0U,    // FCRDIV is 0.

            .frdiv   = 3U,
            .drs     = kMcgDcoRangeSelHigh, // High frequency range
            .dmx32   = kMcgDmx32Default,    // DCO has a default range of 25%
            .oscsel  = kMcgOscselOsc,       // Select OSC
        },
        .simConfig =
        {
            .PllFllSel = kClockPllFllSelFll,    // PLLFLLSEL select FLL.
            .er32kSrc  = kClockEr32kSrcLpo,     // ERCLK32K selection, use LPO.
            .outdiv1   = 0U,
            .outdiv2   = 1U,
            .outdiv4   = 3U,
        },
        .oscerConfig =
        {
            .Enable       = true,  // OSCERCLK enable.
            .EnableInStop = false, // OSCERCLK disable in STOP mode.
        }
    },
};

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_FllStableDelay
 * Description   : This funtion is used to delay for FLL stable.
 * According to datasheet, every time the FLL reference source or reference
 * divider is changed, trim value is changed, DMX32 bit is changed, DRS bits
 * are changed, or changing from FLL disabled (BLPE, BLPI) to FLL enabled
 * (FEI, FEE, FBE, FBI), there should be 1ms delay for FLL stable. Please
 * check datasheet for t(fll_aquire).
 *
 *END**************************************************************************/
static void CLOCK_SYS_FllStableDelay(void)
{
    uint32_t coreClk = CLOCK_SYS_GetCoreClockFreq();

    coreClk /= 3000U;

    // Delay 1 ms.
    while (coreClk--)
    {
        __asm ("nop");
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_SetSimConfigration
 * Description   : This funtion sets the SIM registers for clock transitiom.
 *
 *END**************************************************************************/
static void CLOCK_SYS_SetSimConfigration(sim_config_t const *simConfig)
{
    CLOCK_HAL_SetOutDiv(SIM_BASE,
                        simConfig->outdiv1,
                        simConfig->outdiv2,
                        0U,
                        simConfig->outdiv4);

    CLOCK_HAL_SetPllfllSel(SIM_BASE, simConfig->PllFllSel);

    CLOCK_HAL_SetExternalRefClock32kSrc(SIM_BASE, simConfig->er32kSrc);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_SetOscerConfigration
 * Description   : This funtion sets the OSCERCLK for clock transition.
 *
 *END**************************************************************************/
static void CLOCK_SYS_SetOscerConfigration(uint32_t instance,
                                           oscer_config_t const *oscerConfig)
{
    OSC_HAL_SetExternalRefClkCmd(g_oscBaseAddr[instance],
                                 oscerConfig->Enable);

    OSC_HAL_SetExternalRefClkInStopModeCmd(g_oscBaseAddr[instance],
                                           oscerConfig->EnableInStop);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_SetConfiguration
 * Description   : This funtion sets the system to target configuration, it
 * only sets the clock modules registers for clock mode change, but not send
 * notifications to drivers.
 *
 *END**************************************************************************/
clock_manager_error_code_t CLOCK_SYS_SetConfiguration(clock_manager_user_config_t const* config)
{
    assert(NULL != config);

    /* Set outdiv for safe output clock frequency. */

    CLOCK_HAL_SetOutDiv(SIM_BASE, 1U, 1U, 0U, 4U);

    /* Set MCG mode. */
    CLOCK_SYS_SetMcgMode(&config->mcgConfig, CLOCK_SYS_FllStableDelay);

    /* Set SIM setting. */
    CLOCK_SYS_SetSimConfigration(&config->simConfig);

    /* Set OSCERCLK setting. */
    CLOCK_SYS_SetOscerConfigration(0, &config->oscerConfig);

    return kClockManagerSuccess;
}

clock_manager_error_code_t CLOCK_SYS_GetFreq(clock_names_t clockName,
                                                  uint32_t *frequency)
{
    clock_manager_error_code_t returnCode = kClockManagerSuccess;

    switch (clockName)
    {
        case kCoreClock:
        case kSystemClock:
            *frequency = CLOCK_SYS_GetCoreClockFreq();
            break;
        case kPlatformClock:
            *frequency = CLOCK_SYS_GetSystemClockFreq();
            break;
        case kBusClock:
            *frequency = CLOCK_SYS_GetBusClockFreq();
            break;
        case kFlashClock:
            *frequency = CLOCK_SYS_GetFlashClockFreq();
            break;
        case kOsc32kClock:
            *frequency = CLOCK_SYS_GetExternalRefClock32kFreq();
            break;
        case kOsc0ErClock:
            *frequency = CLOCK_SYS_GetOsc0ExternalRefClockFreq();
            break;
        case kOsc0ErClockUndiv:
            *frequency = CLOCK_SYS_GetOsc0ExternalRefClockUndivFreq();
            break;
        case kMcgFfClock:
            *frequency = CLOCK_SYS_GetFixedFreqClockFreq();
            break;
        case kMcgFllClock:
            *frequency = CLOCK_HAL_GetFllClk(MCG_BASE);
            break;
        case kMcgOutClock:
            *frequency = CLOCK_HAL_GetOutClk(MCG_BASE);
            break;
        case kMcgIrClock:
            *frequency = CLOCK_HAL_GetInternalRefClk(MCG_BASE);
            break;
        case kLpoClock:
            *frequency = CLOCK_SYS_GetLpoClockFreq();
            break;
        default:
            *frequency = 0U;
            returnCode = kClockManagerNoSuchClockName;
            break;
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetCoreClockFreq
 * Description   : Gets the core clock frequency.
 * This function gets the core clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetCoreClockFreq(void)
{
    return CLOCK_HAL_GetOutClk(MCG_BASE) / (CLOCK_HAL_GetOutDiv1(SIM_BASE) + 1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetSystemClockFreq
 * Description   : Gets the system clock frequency.
 * This function gets the system clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetSystemClockFreq(void)
{
    return CLOCK_HAL_GetOutClk(MCG_BASE) / (CLOCK_HAL_GetOutDiv1(SIM_BASE) + 1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetBusClockFreq
 * Description   : Gets the bus clock frequency.
 * This function gets the bus clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetBusClockFreq(void)
{
    return CLOCK_HAL_GetOutClk(MCG_BASE) / (CLOCK_HAL_GetOutDiv2(SIM_BASE) + 1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetFlashClockFreq
 * Description   : Gets the flash clock frequency.
 * This function gets the flash clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetFlashClockFreq(void)
{
    return CLOCK_HAL_GetOutClk(MCG_BASE) / (CLOCK_HAL_GetOutDiv4(SIM_BASE) + 1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetPllFllClockFreq
 * Description   : Gets the MCGPLLCLK/MCGFLLCLK/IRC48MCLK.
 * This function gets the frequency of the MCGPLLCLK/MCGFLLCLK/IRC48MCLK.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetPllFllClockFreq(void)
{
    uint32_t freq;
    clock_pllfll_sel_t src;

    src = CLOCK_HAL_GetPllfllSel(SIM_BASE);

    switch (src)
    {
        case kClockPllFllSelFll:
            freq = CLOCK_HAL_GetFllClk(MCG_BASE);
            break;
        case kClockPllFllSelIrc48M:
            freq = CPU_INTERNAL_IRC_48M;
            break;
        default:
            freq = 0U;
            break;
    }
    return freq;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetExternalRefClockFreq
 * Description   : Gets the ERCLK32K clock frequency.
 * This function gets the external reference (ERCLK32K) clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetExternalRefClock32kFreq(void)
{
    clock_er32k_src_t src;
    uint32_t freq;

    src = CLOCK_HAL_GetExternalRefClock32kSrc(SIM_BASE);

    switch (src)
    {
        case kClockEr32kSrcOsc0:      /* OSC 32k clock  */
            freq = (32768U == g_xtal0ClkFreq) ? 32768U : 0U;
            break;
        case kClockEr32kSrcLpo:         /* LPO clock      */
            freq = CLOCK_SYS_GetLpoClockFreq();
            break;
        default:
            freq = 0U;
            break;
    }

    return freq;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetOsc0ExternalRefClockUndivFreq
 * Description   : Gets OSC0ERCLKUDIV.
 * This function gets the OSC0 external reference undivided frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetOsc0ExternalRefClockUndivFreq(void)
{
    if (OSC_HAL_GetExternalRefClkCmd(g_oscBaseAddr[0]))
    {
        return g_xtal0ClkFreq;
    }
    else
    {
        return 0U;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetOsc0ExternalRefClockFreq
 * Description   : Gets OSC0ERCLK.
 * This function gets the OSC0 external reference frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetOsc0ExternalRefClockFreq(void)
{
    return CLOCK_SYS_GetOsc0ExternalRefClockUndivFreq()
        >> OSC_HAL_GetExternalRefClkDivCmd(g_oscBaseAddr[0]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetWdogFreq
 * Description   : Gets watch dog clock frequency.
 * This function gets the watch dog clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetWdogFreq(uint32_t instance, clock_wdog_src_t wdogSrc)
{
    if (kClockWdogSrcLpoClk == wdogSrc)
    {
        return CLOCK_SYS_GetLpoClockFreq();
    }
    else
    {
        return CLOCK_SYS_GetBusClockFreq();
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetTraceFreq
 * Description   : Gets debug trace clock frequency.
 * This function gets the debug trace clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetTraceFreq(uint32_t instance)
{
    clock_trace_src_t src = CLOCK_HAL_GetTraceClkSrc(SIM_BASE);

    if (kClockTraceSrcMcgoutClk == src)
    {
        return CLOCK_HAL_GetOutClk(MCG_BASE);
    }
    else
    {
        return CLOCK_SYS_GetCoreClockFreq();
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetPortFilterFreq
 * Description   : Gets PORTx digital input filter clock frequency.
 * This function gets the PORTx digital input filter clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetPortFilterFreq(uint32_t instance, clock_port_filter_src_t src)
{
    if (kClockPortFilterSrcBusClk == src)
    {
        return CLOCK_SYS_GetBusClockFreq();
    }
    else
    {
        return CLOCK_SYS_GetLpoClockFreq();
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetLptmrFreq
 * Description   : Gets LPTMRx pre-scaler/glitch filter clock frequency.
 * This function gets the LPTMRx pre-scaler/glitch filter clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetLptmrFreq(uint32_t instance, clock_lptmr_src_t lptmrSrc)
{
    uint32_t freq;

    switch (lptmrSrc)
    {
        case kClockLptmrSrcMcgIrClk:            /* MCG out clock  */
            freq = CLOCK_HAL_GetInternalRefClk(MCG_BASE);
            break;
        case kClockLptmrSrcLpoClk:              /* LPO clock     */
            freq = CLOCK_SYS_GetLpoClockFreq();
            break;
        case kClockLptmrSrcEr32kClk:            /* ERCLK32K clock */
            freq = CLOCK_SYS_GetExternalRefClock32kFreq();
            break;
        case kClockLptmrSrcOsc0erClkUndiv:      /* OSC0ERCLKUDIV clock */
            freq = CLOCK_SYS_GetOsc0ExternalRefClockUndivFreq();
            break;
        default:
            freq = 0U;
            break;
    }

    return freq;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetUartFreq
 * Description   : Gets the clock frequency for UART module.
 * This function gets the clock frequency for UART module.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetUartFreq(uint32_t instance)
{
    uint32_t freq;

    switch (instance)
    {
        case 0:
        case 1:
            freq = CLOCK_SYS_GetSystemClockFreq();
            break;
        default:
            freq = 0U;
            break;
    }

    return freq;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetFtmExternalFreq
 * Description   : Gets FTM external clock frequency.
 *
 *END**************************************************************************/
uint32_t CLOCK_SYS_GetFtmExternalFreq(uint32_t instance)
{
    sim_ftm_clk_sel_t sel = SIM_HAL_GetFtmExternalClkPinMode(SIM_BASE, instance);

    if (kSimFtmClkSel0 == sel)
    {
        return g_ftmClkFreq[0];
    }
    else
    {
        return g_ftmClkFreq[1];
    }
}

/* PORT instance table. */
static const sim_clock_gate_name_t portGateTable[] =
{
    kSimClockGatePortA,
    kSimClockGatePortB,
    kSimClockGatePortC,
    kSimClockGatePortD,
    kSimClockGatePortE
};

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_EnablePortClock
 * Description   : Enable the clock for PORT module
 * This function enables the clock for PORT module
 *
 *END**************************************************************************/
void CLOCK_SYS_EnablePortClock(uint32_t instance)
{
    assert(instance < sizeof(portGateTable)/sizeof(portGateTable[0]));

    SIM_HAL_EnableClock(SIM_BASE, portGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_DisablePortClock
 * Description   : Disable the clock for PORT module
 * This function disables the clock for PORT module
 *
 *END**************************************************************************/
void CLOCK_SYS_DisablePortClock(uint32_t instance)
{
    assert(instance < sizeof(portGateTable)/sizeof(portGateTable[0]));

    SIM_HAL_DisableClock(SIM_BASE, portGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetPortGateCmd
 * Description   : Get the the clock gate state for PORT module
 * This function will get the clock gate state for PORT module.
 *
 *END**************************************************************************/
bool CLOCK_SYS_GetPortGateCmd(uint32_t instance)
{
    assert(instance < sizeof(portGateTable)/sizeof(portGateTable[0]));

    return SIM_HAL_GetGateCmd(SIM_BASE, portGateTable[instance]);
}

/* ADC instance table. */
static const sim_clock_gate_name_t adcGateTable[] =
{
    kSimClockGateAdc0
};

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_EnableAdcClock
 * Description   : Enable the clock for ADC module
 * This function enables the clock for ADC module
 *
 *END**************************************************************************/
void CLOCK_SYS_EnableAdcClock(uint32_t instance)
{
    assert(instance < sizeof(adcGateTable)/sizeof(adcGateTable[0]));

    SIM_HAL_EnableClock(SIM_BASE, adcGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_DisableAdcClock
 * Description   : Disable the clock for ADC module
 * This function disables the clock for ADC module
 *
 *END**************************************************************************/
void CLOCK_SYS_DisableAdcClock(uint32_t instance)
{
    assert(instance < sizeof(adcGateTable)/sizeof(adcGateTable[0]));

    SIM_HAL_DisableClock(SIM_BASE, adcGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetAdcGateCmd
 * Description   : Get the the clock gate state for ADC module
 * This function will get the clock gate state for ADC module.
 *
 *END**************************************************************************/
bool CLOCK_SYS_GetAdcGateCmd(uint32_t instance)
{
    assert(instance < sizeof(adcGateTable)/sizeof(adcGateTable[0]));

    return SIM_HAL_GetGateCmd(SIM_BASE, adcGateTable[instance]);
}

/* DAC instance table. */
static const sim_clock_gate_name_t dacGateTable[] =
{
    kSimClockGateDac0
};

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_EnableDacClock
 * Description   : Enable the clock for DAC module
 * This function enables the clock for DAC module
 *
 *END**************************************************************************/
void CLOCK_SYS_EnableDacClock(uint32_t instance)
{
    assert(instance < sizeof(dacGateTable)/sizeof(dacGateTable[0]));

    SIM_HAL_EnableClock(SIM_BASE, dacGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_DisableDacClock
 * Description   : Disable the clock for DAC module
 * This function disables the clock for DAC module
 *
 *END**************************************************************************/
void CLOCK_SYS_DisableDacClock(uint32_t instance)
{
    assert(instance < sizeof(dacGateTable)/sizeof(dacGateTable[0]));

    SIM_HAL_DisableClock(SIM_BASE, dacGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetDacGateCmd
 * Description   : Get the the clock gate state for DAC module
 * This function will get the clock gate state for DAC module.
 *
 *END**************************************************************************/
bool CLOCK_SYS_GetDacGateCmd(uint32_t instance)
{
    assert(instance < sizeof(dacGateTable)/sizeof(dacGateTable[0]));

    return SIM_HAL_GetGateCmd(SIM_BASE, dacGateTable[instance]);
}

/* FTM instance table. */
static const sim_clock_gate_name_t ftmGateTable[] =
{
    kSimClockGateFtm0,
    kSimClockGateFtm1,
    kSimClockGateFtm2
};

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_EnableFtmClock
 * Description   : Enable the clock for FTM module
 * This function enables the clock for FTM module
 *
 *END**************************************************************************/
void CLOCK_SYS_EnableFtmClock(uint32_t instance)
{
    assert(instance < sizeof(ftmGateTable)/sizeof(ftmGateTable[0]));

    SIM_HAL_EnableClock(SIM_BASE, ftmGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_DisableFtmClock
 * Description   : Disable the clock for FTM module
 * This function disables the clock for FTM module
 *
 *END**************************************************************************/
void CLOCK_SYS_DisableFtmClock(uint32_t instance)
{
    assert(instance < sizeof(ftmGateTable)/sizeof(ftmGateTable[0]));

    SIM_HAL_DisableClock(SIM_BASE, ftmGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetFtmGateCmd
 * Description   : Get the the clock gate state for FTM module
 * This function will get the clock gate state for FTM module.
 *
 *END**************************************************************************/
bool CLOCK_SYS_GetFtmGateCmd(uint32_t instance)
{
    assert(instance < sizeof(ftmGateTable)/sizeof(ftmGateTable[0]));

    return SIM_HAL_GetGateCmd(SIM_BASE, ftmGateTable[instance]);
}

/* SPI instance table. */
static const sim_clock_gate_name_t spiGateTable[] =
{
    kSimClockGateSpi0
};

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_EnableSpiClock
 * Description   : Enable the clock for SPI module
 * This function enables the clock for SPI module
 *
 *END**************************************************************************/
void CLOCK_SYS_EnableSpiClock(uint32_t instance)
{
    assert(instance < sizeof(spiGateTable)/sizeof(spiGateTable[0]));

    SIM_HAL_EnableClock(SIM_BASE, spiGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_DisableSpiClock
 * Description   : Disable the clock for SPI module
 * This function disables the clock for SPI module
 *
 *END**************************************************************************/
void CLOCK_SYS_DisableSpiClock(uint32_t instance)
{
    assert(instance < sizeof(spiGateTable)/sizeof(spiGateTable[0]));

    SIM_HAL_DisableClock(SIM_BASE, spiGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetSpiGateCmd
 * Description   : Get the the clock gate state for SPI module
 * This function will get the clock gate state for SPI module.
 *
 *END**************************************************************************/
bool CLOCK_SYS_GetSpiGateCmd(uint32_t instance)
{
    assert(instance < sizeof(spiGateTable)/sizeof(spiGateTable[0]));

    return SIM_HAL_GetGateCmd(SIM_BASE, spiGateTable[instance]);
}

/* I2C instance table. */
static const sim_clock_gate_name_t i2cGateTable[] =
{
    kSimClockGateI2c0
};

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_EnableI2cClock
 * Description   : Enable the clock for I2C module
 * This function enables the clock for I2C module
 *
 *END**************************************************************************/
void CLOCK_SYS_EnableI2cClock(uint32_t instance)
{
    assert(instance < sizeof(i2cGateTable)/sizeof(i2cGateTable[0]));

    SIM_HAL_EnableClock(SIM_BASE, i2cGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_DisableI2cClock
 * Description   : Disable the clock for I2C module
 * This function disables the clock for I2C module
 *
 *END**************************************************************************/
void CLOCK_SYS_DisableI2cClock(uint32_t instance)
{
    assert(instance < sizeof(i2cGateTable)/sizeof(i2cGateTable[0]));

    SIM_HAL_DisableClock(SIM_BASE, i2cGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetI2cGateCmd
 * Description   : Get the the clock gate state for I2C module
 * This function will get the clock gate state for I2C module.
 *
 *END**************************************************************************/
bool CLOCK_SYS_GetI2cGateCmd(uint32_t instance)
{
    assert(instance < sizeof(i2cGateTable)/sizeof(i2cGateTable[0]));

    return SIM_HAL_GetGateCmd(SIM_BASE, i2cGateTable[instance]);
}

/* Uart instance table. */
static const sim_clock_gate_name_t uartGateTable[] =
{
    kSimClockGateUart0,
    kSimClockGateUart1
};

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_EnableUartClock
 * Description   : Enable the clock for UART module
 * This function enables the clock for UART module
 *
 *END**************************************************************************/
void CLOCK_SYS_EnableUartClock(uint32_t instance)
{
    assert(instance < sizeof(uartGateTable)/sizeof(uartGateTable[0]));

    SIM_HAL_EnableClock(SIM_BASE, uartGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_DisableUartClock
 * Description   : Disable the clock for UART module
 * This function enables the clock for UART module
 *
 *END**************************************************************************/
void CLOCK_SYS_DisableUartClock(uint32_t instance)
{
    assert(instance < sizeof(uartGateTable)/sizeof(uartGateTable[0]));

    SIM_HAL_DisableClock(SIM_BASE, uartGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_GetUartGateCmd
 * Description   : Get the the clock gate state for UART module
 * This function will get the clock gate state for UART module.
 *
 *END**************************************************************************/
bool CLOCK_SYS_GetUartGateCmd(uint32_t instance)
{
    assert(instance < sizeof(uartGateTable)/sizeof(uartGateTable[0]));

    return SIM_HAL_GetGateCmd(SIM_BASE, uartGateTable[instance]);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_Osc0Init
 * Description   : Initialize OSC0.
 *
 * This function initializes OSC0 according to configuration.
 *
 *END**************************************************************************/
clock_manager_error_code_t CLOCK_SYS_Osc0Init(osc_user_config_t *config)
{
    mcg_freq_range_select_t range = (mcg_freq_range_select_t)0U;
    uint32_t freq = config->freq;

    if (kMcgExternalRefClkSelOsc == config->erefs) /* oscillator is used. */
    {
        if ((freq < kMcgConstant1000) ||
           ((freq > kMcgConstant32768) && (freq < kMcgConstant3000000)) ||
            (freq > kMcgConstant32000000))
        {
            return kClockManagerInvalidParam;
        }
        else if (freq < kMcgConstant32768)
        {
            range = kMcgFreqRangeSelLow;
        }
        else if (freq < kMcgConstant8000000)
        {
            range = kMcgFreqRangeSelHigh;
        }
        else
        {
            range = kMcgFreqRangeSelVeryHigh;
        }
    }

    CLOCK_HAL_SetOsc0Mode(MCG_BASE, range, config->hgo, config->erefs);

    if (kMcgExternalRefClkSelOsc == config->erefs) /* oscillator is used. */
    {
        while(CLOCK_HAL_GetOscInit0(MCG_BASE)){}
    }

    if (config->enableExternalRef)
    {
        OSC_HAL_SetExternalRefClkCmd(g_oscBaseAddr[0], true);
        if (config->enableExternalRefInStop)
        {
            OSC_HAL_SetExternalRefClkInStopModeCmd(g_oscBaseAddr[0], true);
        }
    }

    g_xtal0ClkFreq = freq;

    return kClockManagerSuccess;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_SYS_Osc0Deinit
 * Description   : De-initialize OSC0.
 *
 *END**************************************************************************/
void CLOCK_SYS_Osc0Deinit(void)
{
    OSC_HAL_SetExternalRefClkInStopModeCmd(g_oscBaseAddr[0], false);
    OSC_HAL_SetExternalRefClkCmd(g_oscBaseAddr[0], false);
    CLOCK_HAL_SetRange0Mode(MCG_BASE, kMcgFreqRangeSelLow);
    CLOCK_HAL_SetHighGainOsc0Mode(MCG_BASE, kMcgHighGainOscSelLow);
    CLOCK_HAL_SetExternalRefSel0Mode(MCG_BASE, kMcgExternalRefClkSelExternal);

    g_xtal0ClkFreq = 0U;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
