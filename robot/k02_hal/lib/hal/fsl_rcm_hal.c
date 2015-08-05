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

#include "fsl_rcm_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : RCM_HAL_GetSrcStatusCmd
 * Description   : Get the reset source status
 * 
 * This function will get the current reset source status for specified source
 *
 *END**************************************************************************/
bool RCM_HAL_GetSrcStatusCmd(uint32_t baseAddr, rcm_source_names_t srcName)
{
    bool retValue = false;

    assert(srcName < kRcmSrcNameMax);

    switch (srcName)
    {
    case kRcmWakeup:                     /* low-leakage wakeup reset */
        retValue = (bool)BR_RCM_SRS0_WAKEUP(baseAddr);
        break;
    case kRcmLowVoltDetect:              /* low voltage detect reset */
        retValue = (bool)BR_RCM_SRS0_LVD(baseAddr);
        break;
#if FSL_FEATURE_RCM_HAS_LOC
    case kRcmLossOfClk:                  /* loss of clock reset */
        retValue = (bool)BR_RCM_SRS0_LOC(baseAddr);
        break;
#endif
#if FSL_FEATURE_RCM_HAS_LOL
    case kRcmLossOfLock:                 /* loss of lock reset */
        retValue = (bool)BR_RCM_SRS0_LOL(baseAddr);
        break;
#endif
    case kRcmWatchDog:                   /* watch dog reset */
        retValue = (bool)BR_RCM_SRS0_WDOG(baseAddr);
        break;
    case kRcmExternalPin:                /* external pin reset */
        retValue = (bool)BR_RCM_SRS0_PIN(baseAddr);
        break;
    case kRcmPowerOn:                    /* power on reset */
        retValue = (bool)BR_RCM_SRS0_POR(baseAddr);
        break;
#if FSL_FEATURE_RCM_HAS_JTAG
    case kRcmJtag:                       /* JTAG generated reset */
        retValue = (bool)BR_RCM_SRS1_JTAG(baseAddr);
        break;
#endif
    case kRcmCoreLockup:                 /* core lockup reset */
        retValue = (bool)BR_RCM_SRS1_LOCKUP(baseAddr);
        break;
    case kRcmSoftware:                   /* software reset */
        retValue = (bool)BR_RCM_SRS1_SW(baseAddr);
        break;
    case kRcmMdmAp:                     /* MDM-AP system reset */
        retValue = (bool)BR_RCM_SRS1_MDM_AP(baseAddr);
        break;
#if FSL_FEATURE_RCM_HAS_EZPORT
    case kRcmEzport:                     /* EzPort reset */
        retValue = (bool)BR_RCM_SRS1_EZPT(baseAddr);
        break;
#endif
    case kRcmStopModeAckErr:             /* stop mode ack error reset */
        retValue = (bool)BR_RCM_SRS1_SACKERR(baseAddr);
        break;
    default:
        retValue = false;
        break;
    }

    return retValue;
}

#if FSL_FEATURE_RCM_HAS_SSRS
/*FUNCTION**********************************************************************
 *
 * Function Name : RCM_HAL_GetStickySrcStatusCmd
 * Description   : Get the sticy reset source status
 *
 * This function gets the current reset source status that have not been cleared
 * by software for a specified source.
 *
 *END**************************************************************************/
bool RCM_HAL_GetStickySrcStatusCmd(uint32_t baseAddr, rcm_source_names_t srcName)
{
    bool retValue = false;

    assert(srcName < kRcmSrcNameMax);

    switch (srcName)
    {
    case kRcmWakeup:                     /* low-leakage wakeup reset */
        retValue = (bool)BR_RCM_SSRS0_SWAKEUP(baseAddr);
        break;
    case kRcmLowVoltDetect:              /* low voltage detect reset */
        retValue = (bool)BR_RCM_SSRS0_SLVD(baseAddr);
        break;
#if FSL_FEATURE_RCM_HAS_LOC
    case kRcmLossOfClk:                  /* loss of clock reset */
        retValue = (bool)BR_RCM_SSRS0_SLOC(baseAddr);
        break;
#endif
#if FSL_FEATURE_RCM_HAS_LOL
    case kRcmLossOfLock:                 /* loss of lock reset */
        retValue = (bool)BR_RCM_SSRS0_SLOL(baseAddr);
        break;
#endif
    case kRcmWatchDog:                   /* watch dog reset */
        retValue = (bool)BR_RCM_SSRS0_SWDOG(baseAddr);
        break;
    case kRcmExternalPin:                /* external pin reset */
        retValue = (bool)BR_RCM_SSRS0_SPIN(baseAddr);
        break;
    case kRcmPowerOn:                    /* power on reset */
        retValue = (bool)BR_RCM_SSRS0_SPOR(baseAddr);
        break;
#if FSL_FEATURE_RCM_HAS_JTAG
    case kRcmJtag:                       /* JTAG generated reset */
        retValue = (bool)BR_RCM_SSRS1_SJTAG(baseAddr);
        break;
#endif
    case kRcmCoreLockup:                 /* core lockup reset */
        retValue = (bool)BR_RCM_SSRS1_SLOCKUP(baseAddr);
        break;
    case kRcmSoftware:                   /* software reset */
        retValue = (bool)BR_RCM_SSRS1_SSW(baseAddr);
        break;
    case kRcmMdmAp:                     /* MDM-AP system reset */
        retValue = (bool)BR_RCM_SSRS1_SMDM_AP(baseAddr);
        break;
#if FSL_FEATURE_RCM_HAS_EZPORT
    case kRcmEzport:                     /* EzPort reset */
        retValue = (bool)BR_RCM_SSRS1_SEZPT(baseAddr);
        break;
#endif
    case kRcmStopModeAckErr:             /* stop mode ack error reset */
        retValue = (bool)BR_RCM_SSRS1_SSACKERR(baseAddr);
        break;
    default:
        retValue = false;
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RCM_HAL_ClearStickySrcStatus
 * Description   : Clear the sticy reset source status
 *
 * This function clears all the sticky system reset flags.
 *
 *END**************************************************************************/
void RCM_HAL_ClearStickySrcStatus(uint32_t baseAddr)
{
    uint8_t status;

    status = HW_RCM_SSRS0_RD(baseAddr);
    HW_RCM_SSRS0_WR(baseAddr, status);
    status = HW_RCM_SSRS1_RD(baseAddr);
    HW_RCM_SSRS1_WR(baseAddr, status);
}
#endif

/*******************************************************************************
 * EOF
 ******************************************************************************/

