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
#include "fsl_tsi_hal.h"
#include <assert.h>
#if defined(FSL_FEATURE_TSI_VERSION) && (FSL_FEATURE_TSI_VERSION < 3)

static int32_t TSI_HAL_MeasurementBlocking(uint32_t baseAddr);

/*FUNCTION**********************************************************************
*
* Function Name : TSI_HAL_EnableLowPower
* Description   : Function enables low power
*END**************************************************************************/
void TSI_HAL_EnableLowPower(uint32_t baseAddr)
{
    TSI_HAL_EnableStop(baseAddr);
}

/*FUNCTION**********************************************************************
*
* Function Name : TSI_HAL_Init
* Description   : Function resets the TSI peripheral to default state
*
*END**************************************************************************/
void TSI_HAL_Init(uint32_t baseAddr)
{
    HW_TSI_GENCS_WR(baseAddr, 0);
    HW_TSI_SCANC_WR(baseAddr, 0);
}

/*FUNCTION**********************************************************************
*
* Function Name : TSI_HAL_SetConfiguration
* Description   : Function inits the whole TSI peripheral by the handled configuration
*
*END**************************************************************************/
void TSI_HAL_SetConfiguration(uint32_t baseAddr, tsi_config_t *config)
{
    assert(config != NULL);

    TSI_HAL_SetPrescaler(baseAddr, config->ps);
    TSI_HAL_SetNumberOfScans(baseAddr, config->nscn);
    TSI_HAL_SetLowPowerScanInterval(baseAddr, config->lpscnitv);
    TSI_HAL_SetLowPowerClock(baseAddr, config->lpclks);
    TSI_HAL_SetActiveModeSource(baseAddr, config->amclks);
    TSI_HAL_SetActiveModePrescaler(baseAddr, config->ampsc);

#if (FSL_FEATURE_TSI_VERSION == 1)
    TSI_HAL_SetActiveModePrescaler(baseAddr, config->amclkdiv);
    TSI_HAL_SetDeltaVoltage(baseAddr, config->delvol);
    TSI_HAL_SetInternalCapacitanceTrim(baseAddr, config->captrm);
#endif

    TSI_HAL_SetReferenceChargeCurrent(baseAddr, config->refchrg);
    TSI_HAL_SetElectrodeChargeCurrent(baseAddr, config->extchrg);

#if (FSL_FEATURE_TSI_VERSION == 1) /* TODO HAL for VER 1 */
    for (uint32_t i = 0U; i < 16U; i++) {
        tsi->threshold[i] = TSI_THRESHOLD_HTHH(config->thresh) |
                            TSI_THRESHOLD_LTHH(config->thresl);
    }
#elif (FSL_FEATURE_TSI_VERSION == 2)
    TSI_HAL_SetLowThreshold(baseAddr, config->thresl);
    TSI_HAL_SetHighThreshold(baseAddr, config->thresh);
#endif
}

/*FUNCTION**********************************************************************
*
* Function Name : TSI_HAL_Recalibrate
* Description   : Function do recalibration process of TSI main parameters to
*                 find the gold cut on all enabled electrodes
*
*END**************************************************************************/
#define TSI_RECALIBRATE_MAX_SIGNAL_VAL (65535U)

uint32_t TSI_HAL_Recalibrate(uint32_t baseAddr, tsi_config_t *config, const uint32_t electrodes, const tsi_parameter_limits_t *parLimits)
{
    assert(config != NULL);
    
    uint32_t is_enabled = TSI_HAL_IsModuleEnabled(baseAddr);
    uint32_t is_int_enabled = TSI_HAL_IsInterruptEnabled(baseAddr);
    uint32_t lowest_signal = TSI_RECALIBRATE_MAX_SIGNAL_VAL;
    
    if (is_enabled) {
        TSI_HAL_DisableModule(baseAddr);
    }
    if (is_int_enabled) {
        TSI_HAL_DisableInterrupt(baseAddr);
    }

    TSI_HAL_SetNumberOfScans(baseAddr, config->nscn);
    TSI_HAL_SetPrescaler(baseAddr, config->ps);
    TSI_HAL_SetElectrodeChargeCurrent(baseAddr, config->extchrg);
    TSI_HAL_SetReferenceChargeCurrent(baseAddr, config->refchrg);

    TSI_HAL_EnableModule(baseAddr);

    if (TSI_HAL_MeasurementBlocking(baseAddr) == 0) {
        for (uint32_t i = 0U; i < 16U; i++) {
            if (TSI_HAL_GetEnabledChannel(baseAddr, i)) {
                int32_t counter = TSI_HAL_GetCounter(baseAddr, i);
                if (counter < lowest_signal) {
                    lowest_signal = counter;
                }
            }
        }
    }

    if (!is_enabled) {
        TSI_HAL_EnableModule(baseAddr);
    }
    if (is_int_enabled) {
        TSI_HAL_EnableInterrupt(baseAddr);
    }
    if (lowest_signal == TSI_RECALIBRATE_MAX_SIGNAL_VAL) {
        lowest_signal = 0U;  /* not valid */
    }

    return lowest_signal;
}

/*FUNCTION**********************************************************************
*
* Function Name : TSI_HAL_MeasurementBlocking
* Description   : Function do blocking measurement of enabled electrodes
*                 It used just for recalibration process
*END**************************************************************************/
static int32_t TSI_HAL_MeasurementBlocking(uint32_t baseAddr)
{
  int32_t result = -1;
  uint32_t timeout = 10000000; /* Big timeout */ 
  
  if (HW_TSI_PEN_RD(baseAddr)) {

    /* measure only if at least one electrode is enabled */
    TSI_HAL_EnableSoftwareTriggerScan(baseAddr);
    TSI_HAL_EnableModule(baseAddr);
    TSI_HAL_StartSoftwareTrigger(baseAddr);

    while((TSI_HAL_GetEndOfScanFlag(baseAddr) == 0U) && (timeout--))
    {
      /* Do nothing, just to meet MISRA C 2004 rule 14.3 . */
    }
    TSI_HAL_ClearEndOfScanFlag(baseAddr);
    TSI_HAL_DisableModule(baseAddr);

    if(timeout)
    {
      result = 0;
    }
  }
  return result;
}

#endif
