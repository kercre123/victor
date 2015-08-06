/*
 * Copyright (c) 2013, Freescale Semiconductor, Inc.
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
#if !defined(__FSL_MCG_HAL_MODES_H__)
#define __FSL_MCG_HAL_MODES_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "fsl_mcg_hal.h"

//! @addtogroup mcg_hal
//! @{

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/*! @brief MCG mode definitions */
typedef enum _mcg_modes {
    kMcgModeFEI,   /* FEI   - FLL Engaged Internal         */
    kMcgModeFBI,   /* FBI   - FLL Bypassed Internal        */
    kMcgModeBLPI,  /* BLPI  - Bypassed Low Power Internal  */
    kMcgModeFEE,   /* FEE   - FLL Engaged External         */
    kMcgModeFBE,   /* FBE   - FLL Bypassed External        */
    kMcgModeBLPE,  /* BLPE  - Bypassed Low Power External  */
    kMcgModePBE,   /* PBE   - PLL Bypassed Enternal        */
    kMcgModePEE,   /* PEE   - PLL Engaged External         */
    kMcgModeSTOP,  /* STOP  - Stop                         */
    kMcgModeError  /* Unknown mode                         */
} mcg_modes_t;

/*! @brief MCG mode transition API error code definitions */
typedef enum McgModeError {

    kMcgModeErrNone            =   0x00,    /* - No error. */
    kMcgModeErrModeUnreachable =   0x01,    /* - Target mode is unreachable. */

    /* Oscillator error codes */
    kMcgModeErrOscFreqRange    =   0x21,    /* - OSC frequency is invalid. */

    /* IRC and FLL error codes */

    kMcgModeErrIrcSlowRange   = 0x31,    /* - slow IRC is outside allowed range */
    kMcgModeErrIrcFastRange   = 0x32,    /* - fast IRC is outside allowed range */
    kMcgModeErrFllRefRange    = 0x33,    /* - FLL reference frequency is outsice allowed range */
    kMcgModeErrFllFrdivRange  = 0x34,    /* - FRDIV outside allowed range */
    kMcgModeErrFllDrsRange    = 0x35,    /* - DRS is out of range */
    kMcgModeErrFllDmx32Range  = 0x36,    /* - DMX32 setting not allowed. */

    /* PLL error codes */

    kMcgModeErrPllPrdivRange =       0x41,    /* - PRDIV outside allowed range */
    kMcgModeErrPllVdivRange =        0x42,    /* - VDIV outside allowed range */
    kMcgModeErrPllRefClkRange =      0x43,    /* - PLL reference clock frequency, out of allowed range */
    kMcgModeErrPllLockBit =          0x44,    /* - LOCK or LOCK2 bit did not set */
    kMcgModeErrPllOutClkRange =      0x45,    /* - PLL output frequency is outside allowed range (NEED 
                                             TO ADD THIS CHECK TO fbe_pbe and blpe_pbe) only in 
                                             fei-pee at this time */
    kMcgModeErrMax = 0x1000
} mcg_mode_error_t;

////////////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

/*!
 * @brief Gets the current MCG mode.
 *
 * This function checks the MCG registers and determine current MCG mode.
 *
 * @param       baseAddr  Base address for current MCG instance.
 * @return      mcgMode     Current MCG mode or error code mcg_modes_t
 */
mcg_modes_t CLOCK_HAL_GetMcgMode(uint32_t baseAddr);

/*!
 * @brief Set MCG to FEI mode.
 *
 * This function sets MCG to FEI mode.
 *
 * @param       baseAddr  Base address for current MCG instance.
 * @param       drs       The DCO range selection.
 * @param       fllStableDelay Delay function to make sure FLL is stable.
 * @param       outClkFreq MCGCLKOUT frequency in new mode.
 * @return      value     Error code
 */
mcg_mode_error_t CLOCK_HAL_SetFeiMode(uint32_t baseAddr,
                              mcg_dco_range_select_t drs,
                              void (* fllStableDelay)(void),
                              uint32_t *outClkFreq);

/*!
 * @brief Set MCG to FEE mode.
 *
 * This function sets MCG to FEE mode.
 *
 * @param       baseAddr  Base address for current MCG instance.
 * @param       oscselval OSCSEL in FEE mode.
 * @param       frdivVal  FRDIV in FEE mode.
 * @param       dmx32     DMX32 in FEE mode.
 * @param       drs       The DCO range selection.
 * @param       fllStableDelay Delay function to make sure FLL is stable.
 * @param       outClkFreq MCGCLKOUT frequency in new mode.
 * @return      value     Error code
 */
mcg_mode_error_t CLOCK_HAL_SetFeeMode(uint32_t baseAddr,
                              mcg_oscsel_select_t oscselVal,
                              uint8_t frdivVal,
                              mcg_dmx32_select_t dmx32,
                              mcg_dco_range_select_t drs,
                              void (* fllStableDelay)(void),
                              uint32_t *outClkFreq);

/*!
 * @brief Set MCG to FBI mode.
 *
 * This function sets MCG to FBI mode.
 *
 * @param       baseAddr  Base address for current MCG instance.
 * @param       drs       The DCO range selection.
 * @param       ircselect The internal reference clock to select.
 * @param       fllStableDelay Delay function to make sure FLL is stable.
 * @param       outClkFreq MCGCLKOUT frequency in new mode.
 * @return      value     Error code
 */
mcg_mode_error_t CLOCK_HAL_SetFbiMode(uint32_t baseAddr,
                              mcg_dco_range_select_t drs,
                              mcg_internal_ref_clock_select_t ircSelect,
                              uint8_t fcrdivVal,
                              void (* fllStableDelay)(void),
                              uint32_t *outClkFreq);

/*!
 * @brief Set MCG to FBE mode.
 *
 * This function sets MCG to FBE mode.
 *
 * @param       baseAddr  Base address for current MCG instance.
 * @param       oscselval OSCSEL in FEE mode.
 * @param       frdivVal  FRDIV in FEE mode.
 * @param       dmx32     DMX32 in FEE mode.
 * @param       drs       The DCO range selection.
 * @param       fllStableDelay Delay function to make sure FLL is stable.
 * @param       outClkFreq MCGCLKOUT frequency in new mode.
 * @return      value     Error code
 */
mcg_mode_error_t CLOCK_HAL_SetFbeMode(uint32_t baseAddr,
                             mcg_oscsel_select_t oscselVal,
                             uint8_t frdivVal,
                             mcg_dmx32_select_t dmx32,
                             mcg_dco_range_select_t drs,
                             void (* fllStableDelay)(void),
                             uint32_t *outClkFreq);

/*!
 * @brief Set MCG to BLPI mode.
 *
 * This function sets MCG to BLPI mode.
 *
 * @param       baseAddr  Base address for current MCG instance.
 * @param       ircselect The internal reference clock to select.
 * @param       outClkFreq MCGCLKOUT frequency in new mode.
 * @return      value     Error code
 */
mcg_mode_error_t CLOCK_HAL_SetBlpiMode(uint32_t baseAddr,
                               uint8_t fcrdivVal,
                               mcg_internal_ref_clock_select_t ircSelect,
                               uint32_t *outClkFreq);

/*!
 * @brief Set MCG to BLPE mode.
 *
 * This function sets MCG to BLPE mode.
 *
 * @param       baseAddr  Base address for current MCG instance.
 * @param       oscselval OSCSEL in FEE mode.
 * @param       outClkFreq MCGCLKOUT frequency in new mode.
 * @return      value     Error code
 */
mcg_mode_error_t CLOCK_HAL_SetBlpeMode(uint32_t baseAddr,
                               mcg_oscsel_select_t oscselVal,
                               uint32_t *outClkFreq);

/*!
 * @brief Set MCG to PBE mode.
 *
 * This function sets MCG to PBE mode.
 *
 * @param       baseAddr  Base address for current MCG instance.
 * @param       oscselval OSCSEL in FBE mode.
 * @param       pllcsselect  PLLCS in PBE mode.
 * @param       prdivval  PRDIV in PBE mode.
 * @param       vdivVal   VDIV in PBE mode.
 * @param       outClkFreq MCGCLKOUT frequency in new mode.
 * @return      value     Error code
 */
mcg_mode_error_t CLOCK_HAL_SetPbeMode(uint32_t baseAddr,
                              mcg_oscsel_select_t oscselVal,
                              mcg_pll_clk_select_t pllcsSelect,
                              uint8_t prdivVal,
                              uint8_t vdivVal,
                              uint32_t *outClkFreq);

/*!
 * @brief Set MCG to PBE mode.
 *
 * This function sets MCG to PBE mode.
 *
 * @param       baseAddr  Base address for current MCG instance.
 * @param       outClkFreq MCGCLKOUT frequency in new mode.
 * @return      value     Error code
 * @note        This function only change CLKS to use PLL/FLL output. If the
 *              PRDIV/VDIV are different from PBE mode, please setup these
 *              settings in PBE mode and wait for stable then switch to PEE mode.
 */
mcg_mode_error_t CLOCK_HAL_SetPeeMode(uint32_t baseAddr,
                                      uint32_t *outClkFreq);


#if defined(__cplusplus)
}
#endif // __cplusplus

//! @}

#endif // __FSL_MCG_HAL_MODES_H__
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
