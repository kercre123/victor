/*
* Copyright (c) 2014, Freescale Semiconductor, Inc.
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

#if !defined(__FSL_MCGLITE_HAL_H__)
#define __FSL_MCGLITE_HAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "fsl_device_registers.h"

/*! @addtogroup mcglite_hal*/
/*! @{*/

/*! @file fsl_mcglite_hal.h */

/*******************************************************************************
* Definitions
******************************************************************************/
extern uint32_t g_xtalRtcClkFreq;         /* EXTAL RTC clock */
/*! @brief MCG_Lite constant definitions. */
enum _mcglite_constant
{
    kMcgliteConst0   =          0U,
    
    kMcgliteConst2M  =    2000000U,
    kMcgliteConst8M  =    8000000U,
    kMcgliteConst48M =   48000000U,
};

/*! @brief MCG_Lite clock source selection. */
typedef enum _mcglite_mcgoutclk_source
{
    kMcgliteClkSrcHirc,      /* MCGOUTCLK source is HIRC */
    kMcgliteClkSrcLirc,      /* MCGOUTCLK source is LIRC */
    kMcgliteClkSrcExt,       /* MCGOUTCLK source is external clock source */
    kMcgliteClkSrcReserved
} mcglite_mcgoutclk_source_t;

/*! @brief MCG_Lite LIRC select. */
typedef enum _mcglite_lirc_select
{
    kMcgliteLircSel2M,          /* slow internal reference(LIRC) 2MHz clock selected */
    kMcgliteLircSel8M,          /* slow internal reference(LIRC) 8MHz clock selected */
} mcglite_lirc_select_t;

/*! @brief MCG_Lite external clock Select */
typedef enum _mcglite_ext_select
{
    kMcgliteExtInput,                /* Selects external input clock */
    kMcgliteExtOsc                   /* Selects Oscillator  */
} mcglite_ext_select_t;

/*! @brief MCG_Lite divider factor selection for clock source*/
typedef enum _mcglite_lirc_div
{
    kMcgliteLircDivBy1 = 0U,          /* divider is 1 */
    kMcgliteLircDivBy2 ,              /* divider is 2 */
    kMcgliteLircDivBy4 ,              /* divider is 4 */
    kMcgliteLircDivBy8 ,              /* divider is 8 */
    kMcgliteLircDivBy16,              /* divider is 16 */
    kMcgliteLircDivBy32,              /* divider is 32 */
    kMcgliteLircDivBy64,              /* divider is 64 */
    kMcgliteLircDivBy128              /* divider is 128 */
} mcglite_lirc_div_t;

/*! @brief MCG_Lite external oscillator status */
typedef enum _mcglite_ext_osc_status
{
    kMcgliteOscUnready = 0U,             /* osc clock is not ready */
    kMcgliteOscReady                     /* osc clock is ready     */
} mcglite_ext_osc_status_t;

/*! @brief MCG frequency range select */
typedef enum _mcg_freq_range_select
{
    kMcgFreqRangeSelLow,         /* Low frequency range selected for the crystal OSC */
    kMcgFreqRangeSelHigh,        /* High frequency range selected for the crystal OSC */
    kMcgFreqRangeSelVeryHigh,    /* Very High frequency range selected for the crystal OSC */
    kMcgFreqRangeSelVeryHigh1    /* Very High frequency range selected for the crystal OSC */
} mcg_freq_range_select_t;

/*! @brief MCG high gain oscillator select */
typedef enum _mcg_high_gain_osc_select
{
    kMcgHighGainOscSelLow,               /* Configure crystal oscillator for low-power operation */
    kMcgHighGainOscSelHigh               /* Configure crystal oscillator for high-gain operation */
} mcg_high_gain_osc_select_t;

extern uint32_t g_xtal0ClkFreq;           /* EXTAL0 clock */

/*******************************************************************************
* API
******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*! @name MCG_Lite output clock access API*/
/*@{*/

/*!
* @brief Gets the current MCG_Lite low internal reference clock(2MHz or 8MHz)
*
* This function returns the MCG_Lite LIRC frequency (Hertz) based
* on the current MCG_Lite configurations and settings. Please make sure LIRC
* has been properly configured to get the valid value.
*
* @param baseAddr MCG_Lite register base address.
*
* @return Frequency value in Hertz of the MCG_Lite LIRC.
*/
uint32_t CLOCK_HAL_GetLircClk(uint32_t baseAddr);

/*!
* @brief Gets the current MCG_Lite LIRC_DIV1_CLK frequency.
*
* This function returns the MCG_Lite LIRC_DIV1_CLK frequency (Hertz) based
* on the current MCG_Lite configurations and settings. Please make sure LIRC
* has been properly configured to get the valid value.
*
* @param baseAddr MCG_Lite register base address.
*
* @return Frequency value in Hertz of the MCG_Lite LIRC_DIV1_CLK.
*/
uint32_t CLOCK_HAL_GetLircDiv1Clk(uint32_t baseAddr);

/*!
* @brief Gets the current MCGIRCLK frequency.
*
* This function returns the MCGIRCLK frequency (Hertz) based
* on the current MCG_Lite configurations and settings. Please make sure LIRC
* has been properly configured to get the valid value.
*
* @param baseAddr MCG_Lite register base address.
*
* @return Frequency value in Hertz of MCGIRCLK.
*/
uint32_t CLOCK_HAL_GetInternalRefClk(uint32_t baseAddr);

/*!
* @brief Gets the current MCGOUTCLK frequency.
*
* This function returns the MCGOUTCLK frequency (Hertz) based on
* the current MCG_Lite configurations and settings. The configuration should be
* properly done in order to get the valid value.
*
* @param baseAddr MCG_Lite register base address.
*
* @return Frequency value in Hertz of MCGOUTCLK.
*/
uint32_t CLOCK_HAL_GetOutClk(uint32_t baseAddr);

/*@}*/

/*! @name MCG_Lite control register access API*/
/*@{*/

/*!
* @brief Sets the Clock Source Select.
*
* This function selects the clock source for MCGOUTCLK.
*
* @param baseAddr  MCG_Lite register base address.
* @param select    Clock source selection
*                  - 00: HIRC clock is select.
*                  - 01: LIRC(low Internal reference clock) is selected.
*                  - 10: External reference clock is selected.
*                  - 11: Reserved.
*/
static inline void CLOCK_HAL_SetClkSrcMode(uint32_t baseAddr, mcglite_mcgoutclk_source_t select)
{
    BW_MCG_C1_CLKS(baseAddr, select);
}

/*!
* @brief Gets the Clock Source Select.
*
* This function checks the register MCG_C1 CLKS and returns the clock source
* selection for the MCGOUTCLK.
*
* @param baseAddr MCG_Lite register base address.
*
* @return Clock source selection
*/
static inline mcglite_mcgoutclk_source_t CLOCK_HAL_GetClkSrcMode(uint32_t baseAddr)
{
    return (mcglite_mcgoutclk_source_t)BR_MCG_C1_CLKS(baseAddr);
}

/*!
* @brief Sets the Low Internal Reference Select.
*
* This function sets the LIRC to work at 2MHz or 8MHz.
*
* @param baseAddr MCG_Lite register base address.
*
* @param Select 2MHz or 8MHz.
*/
static inline void CLOCK_HAL_SetLircSelMode(uint32_t baseAddr, mcglite_lirc_select_t select)
{
    BW_MCG_C2_IRCS(baseAddr, select);
}

/*!
* @brief Gets the Low Internal Reference Select.
*
* This function gets current LIRC is working at 2MHz or 8MHz.
*
* @param baseAddr MCG_Lite register base address.
*
* @return Current LIRC run status.
*/
static inline mcglite_lirc_select_t CLOCK_HAL_GetLircSelMode(uint32_t baseAddr)
{
    return (mcglite_lirc_select_t)BR_MCG_C2_IRCS(baseAddr);
}

/*!
* @brief Gets the low internal reference divider 1.
*
* This function gets the low internal reference divider 1, the register FCRDIV.
*
* @param baseAddr MCG_Lite register base address.
*
* @return Current LIRC divider 1 setting.
*/
static inline mcglite_lirc_div_t CLOCK_HAL_GetLircRefDiv(uint32_t baseAddr)
{
    return (mcglite_lirc_div_t)BR_MCG_SC_FCRDIV(baseAddr);
}

/*!
* @brief Sets the low internal reference divider 1.
*
* This function sets the low internal reference divider 1, the register FCRDIV.
*
* @param baseAddr MCG_Lite register base address.
*
* @param setting LIRC divider 1 setting value.
*/
static inline void CLOCK_HAL_SetLircRefDiv(uint32_t baseAddr, mcglite_lirc_div_t setting)
{
    BW_MCG_SC_FCRDIV(baseAddr, setting);
}

/*!
* @brief Sets the low internal reference divider 2.
*
* This function sets the low internal reference divider 2.
*
* @param baseAddr MCG_Lite register base address.
*
* @param setting LIRC divider 2 setting value.
*/
static inline void CLOCK_HAL_SetLircDiv2(uint32_t baseAddr, mcglite_lirc_div_t setting)
{
    BW_MCG_MC_LIRC_DIV2(baseAddr, setting);
}

/*!
* @brief Gets the low internal reference divider 2.
*
* This function gets the low internal reference divider 2 setting.
*
* @param baseAddr MCG_Lite register base address.
*
* @return Current LIRC divider 2 setting.
*/
static inline mcglite_lirc_div_t CLOCK_HAL_GetLircDiv2(uint32_t baseAddr)
{
    return (mcglite_lirc_div_t)BR_MCG_MC_LIRC_DIV2(baseAddr);
}

/*!
* @brief Enables the Low Internal Reference Clock setting
*
* This function enables/disables the low internal reference clock.
*
* @param baseAddr MCG_Lite register base address.
*
* @param enable Enable or disable internal reference clock.
*                 - true: MCG_Lite Low IRCLK active
*                 - false: MCG_Lite Low IRCLK inactive
*/
static inline void CLOCK_HAL_SetLircCmd(uint32_t baseAddr, bool enable)
{
    BW_MCG_C1_IRCLKEN(baseAddr, enable ? 1 : 0);
}

/*!
* @brief Get the Low Internal Reference Clock enable or not.
*
* This function checks the low internal reference is enabled or disabled.
*
* @param baseAddr MCG_Lite register base address.
*
* @retval true  LIRC is enabled
* @retval false LIRC is disabled
*/
static inline bool CLOCK_HAL_GetLircCmd(uint32_t baseAddr)
{
    return BR_MCG_C1_IRCLKEN(baseAddr);
}

/*!
* @brief Sets the Low Internal Reference Clock disabled or not in STOP mode.
*
* This function controls whether or not the low internal reference clock remains
* enabled when the MCG_Lite enters STOP mode.
*
* @param baseAddr MCG_Lite register base address.
*
* @param enable Enable or disable low internal reference clock stop setting.
*                 - true: Internal reference clock is enabled in stop mode if IRCLKEN is set
before entering STOP mode.
*                 - false: Low internal reference clock is disabled in STOP mode
*/
static inline void CLOCK_HAL_SetLircStopCmd(uint32_t baseAddr, bool enable)
{
    BW_MCG_C1_IREFSTEN(baseAddr, enable ? 1 : 0);
}

/*!
* @brief Gets the Low Internal Reference Clock disabled or not in STOP mode.
*
* This function gets the Low Internal Reference Clock Stop Enable setting.
*
* @param baseAddr MCG_Lite register base address.
*
* @retval true  LIRC is enabled in STOP mode if IRCLKEN is set.
* @retval false LIRC is disabled in STOP mode.
*/
static inline bool CLOCK_HAL_GetLircStopCmd(uint32_t baseAddr)
{
    return BR_MCG_C1_IREFSTEN(baseAddr);
}

/*!
* @brief Enable or disable the High Internal Reference Clock setting.
*
* This function enables/disables the internal reference clock for use as MCGPCLK.
*
* @param baseAddr MCG_Lite register base address.
*
* @param enable Enable or disable HIRC.
*                 - true: MCG_Lite HIRC active
*                 - false: MCG_Lite HIRC inactive
*/
static inline void CLOCK_HAL_SetHircCmd(uint32_t baseAddr, bool enable)
{
    BW_MCG_MC_HIRCEN(baseAddr, enable ? 1 : 0);
}

/*!
* @brief Gets the High Internal Reference Clock is enabled or not.
*
* This function gets the high internal reference clock enable setting.
*
* @param baseAddr MCG_Lite register base address.
*
* @return True if high internal reference clock is enabled
*/
static inline bool CLOCK_HAL_GetHircCmd(uint32_t baseAddr)
{
    return BR_MCG_MC_HIRCEN(baseAddr);
}

/*!
* @brief Sets the External Reference Select.
*
* This function selects the source for the external reference clock.
* Refer to the Oscillator (OSC) for more details.
*
* @param baseAddr MCG_Lite register base address.
*
* @param select  External Reference Select.
*                 - 0: External input clock requested
*                 - 1: Crystal requested
*/
static inline void CLOCK_HAL_SetExtRefSelMode0(uint32_t baseAddr, mcglite_ext_select_t select)
{
    BW_MCG_C2_EREFS0(baseAddr, select);
}

/*!
* @brief Gets the External Reference Select.
*
* This function gets the External Reference Select.
*
* @param baseAddr MCG_Lite register base address.
*
* @return External Reference Select.
*/
static inline mcglite_ext_select_t CLOCK_HAL_GetExtRefSelMode0(uint32_t baseAddr)
{
    return (mcglite_ext_select_t)BR_MCG_C2_EREFS0(baseAddr);
}

/*!
* @brief Gets the Clock Mode Status.
*
* This function gets the Clock Mode Status. These bits indicate the current clock mode.
* The CLKST bits do not update immediately after a write to the CLKS bits due to
* internal synchronization between clock domains.
*
* @param baseAddr MCG_Lite register base address.
*
* @return status  Clock Mode Status
*                  - 00: HIRC clock is select.
*                  - 01: LIRC(low Internal reference clock) is selected.
*                  - 10: External reference clock is selected.
*                  - 11: Reserved.
*/
static inline mcglite_mcgoutclk_source_t CLOCK_HAL_GetClkSrcStat(uint32_t baseAddr)
{
    return (mcglite_mcgoutclk_source_t)BR_MCG_S_CLKST(baseAddr);
}

/*!
* @brief Gets the OSC Initialization Status.
*
* This function gets the OSC Initialization Status OSCINIT0. This bit,
* which resets to 0, is set to 1 after the initialization cycles of
* the crystal oscillator clock have completed. After being set, the bit
* is cleared to 0 if the OSC is subsequently disabled. See
* the OSC module's detailed description for more information.
*
* @param baseAddr MCG_Lite register base address.
*
* @return OSC initialization status
*/
static inline mcglite_ext_osc_status_t CLOCK_HAL_GetOscInit0(uint32_t baseAddr)
{
    return (mcglite_ext_osc_status_t)BR_MCG_S_OSCINIT0(baseAddr);
}

#if FSL_FEATURE_MCGLITE_HAS_RANGE0
/*!
 * @brief Sets the Frequency Range0 Select Setting.
 *
 * This function  selects the frequency range for the OSC crystal oscillator
 * or an external clock source. See the Oscillator chapter for more details and
 * the device data sheet for the frequency ranges used.
 *
 * @param baseAddr MCG_Lite register base address.
 * @params setting  Frequency Range0 Select Setting
 *                  - 00: Low frequency range selected for the crystal oscillator.
 *                  - 01: High frequency range selected for the crystal oscillator.
 *                  - 1X: Very high frequency range selected for the crystal oscillator.
 */
static inline void CLOCK_HAL_SetRange0Mode(uint32_t baseAddr, mcg_freq_range_select_t setting)
{
    BW_MCG_C2_RANGE0(baseAddr, setting);
}

/*!
 * @brief Gets the Frequency Range0 Select Setting.
 *
 * This function  gets the Frequency Range0 Select Setting.
 *
 * @param baseAddr MCG_Lite register base address.
 * @return setting  Frequency Range0 Select Setting
 */
static inline mcg_freq_range_select_t CLOCK_HAL_GetRange0Mode(uint32_t baseAddr)
{
    return (mcg_freq_range_select_t)BR_MCG_C2_RANGE0(baseAddr);
}
#endif

#if FSL_FEATURE_MCGLITE_HAS_HGO0
/*!
 * @brief Sets the High Gain Oscillator0 Select Setting.
 *
 * This function controls the OSC0 crystal oscillator mode of operation.
 * See the Oscillator chapter for more details.
 *
 * @param baseAddr MCG_Lite register base address.
 * @params setting  High Gain Oscillator0 Select Setting
 *                  - 0: Configure crystal oscillator for low-power operation.
 *                  - 1: Configure crystal oscillator for high-gain operation.
 */
static inline void CLOCK_HAL_SetHighGainOsc0Mode(uint32_t baseAddr,
                                                 mcg_high_gain_osc_select_t setting)
{
    BW_MCG_C2_HGO0(baseAddr, setting);
}

/*!
 * @brief Gets the High Gain Oscillator0 Select Setting.
 *
 * This function  gets the High Gain Oscillator0 Select Setting.
 *
 * @param baseAddr MCG_Lite register base address.
 * @return setting  High Gain Oscillator0 Select Setting
 */
static inline mcg_high_gain_osc_select_t CLOCK_HAL_GetHighGainOsc0Mode(uint32_t baseAddr)
{
    return (mcg_high_gain_osc_select_t)BR_MCG_C2_HGO0(baseAddr);
}
#endif

#if FSL_FEATURE_MCGLITE_HAS_HCTRIM
/*!
 * @brief Gets the High-frequency IRC coarse trim value.
 *
 * This function gets the High-frequency IRC coarse trim value.
 *
 * @param baseAddr MCG_Lite register base address.
 */
static inline uint8_t CLOCK_HAL_GetHircCoarseTrim(uint32_t baseAddr)
{
    return BR_MCG_HCTRIM_COARSE_TRIM(baseAddr);
}
#endif

#if FSL_FEATURE_MCGLITE_HAS_HTTRIM
/*!
 * @brief Gets the High-frequency IRC tempco trim value.
 *
 * This function gets the High-frequency IRC tempco trim value.
 *
 * @param baseAddr MCG_Lite register base address.
 */
static inline uint8_t CLOCK_HAL_GetHircTempcoTrim(uint32_t baseAddr)
{
    return BR_MCG_HTTRIM_TEMPCO_TRIM(baseAddr);
}
#endif

#if FSL_FEATURE_MCGLITE_HAS_HFTRIM
/*!
 * @brief Gets the High-frequency IRC fine trim value.
 *
 * This function gets the High-frequency IRC fine trim value.
 *
 * @param baseAddr MCG_Lite register base address.
 */
static inline uint8_t CLOCK_HAL_GetHircFineTrim(uint32_t baseAddr)
{
    return BR_MCG_HFTRIM_FINE_TRIM(baseAddr);
}
#endif

#if FSL_FEATURE_MCGLITE_HAS_LTRIMRNG
/*!
 * @brief Gets the LIRC 8M TRIM RANGE value.
 *
 * This function gets the LIRC 8M RANGE value.
 *
 * @param baseAddr MCG_Lite register base address.
 */
static inline uint8_t CLOCK_HAL_GetLirc8MTrimRange(uint32_t baseAddr)
{
    return BR_MCG_LTRIMRNG_FTRIMRNG(baseAddr);
}

/*!
 * @brief Gets the LIRC 2M TRIM RANGE value.
 *
 * This function gets the LIRC 2M RANGE value.
 *
 * @param baseAddr MCG_Lite register base address.
 */
static inline uint8_t CLOCK_HAL_GetLirc2MTrimRange(uint32_t baseAddr)
{
    return BR_MCG_LTRIMRNG_STRIMRNG(baseAddr);
}
#endif

#if FSL_FEATURE_MCGLITE_HAS_LFTRIM
/*!
 * @brief Gets the LIRC 8M trim value.
 *
 * This function gets the LIRC 8M trim value.
 *
 * @param baseAddr MCG_Lite register base address.
 */
static inline uint8_t CLOCK_HAL_GetLirc8MTrim(uint32_t baseAddr)
{
    return BR_MCG_LFTRIM_LIRC_FTRIM(baseAddr);
}
#endif

#if FSL_FEATURE_MCGLITE_HAS_LSTRIM
/*!
 * @brief Gets the LIRC 2M trim value.
 *
 * This function gets the LIRC 2M trim value.
 *
 * @param baseAddr MCG_Lite register base address.
 */
static inline uint8_t CLOCK_HAL_GetLirc2MTrim(uint32_t baseAddr)
{
    return BR_MCG_LSTRIM_LIRC_STRIM(baseAddr);
}
#endif


/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*! @}*/

#endif /* __FSL_MCGLITE_HAL_H__*/
/*******************************************************************************
* EOF
******************************************************************************/

