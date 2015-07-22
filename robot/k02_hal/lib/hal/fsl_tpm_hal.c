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
#include "fsl_tpm_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

void TPM_HAL_EnablePwmMode(uint32_t baseAddr, tpm_pwm_param_t *config, uint8_t channel)
{
    uint32_t val;

    val = ((uint32_t)(config->edgeMode ? 1 : 2)) << BP_TPM_CnSC_ELSA;
    val |= (2 << BP_TPM_CnSC_MSA);
    TPM_HAL_SetChnMsnbaElsnbaVal(baseAddr, channel, val);

    /* Wait till mode change is acknowledged */
    while (!(TPM_HAL_GetChnMsnbaVal(baseAddr, channel))) { }

    switch(config->mode)
    {
        case kTpmEdgeAlignedPWM:
            TPM_HAL_SetCpwms(baseAddr, 0);
            break;
        case kTpmCenterAlignedPWM:
            TPM_HAL_SetCpwms(baseAddr, 1);
            break;
        default:
            assert(0);
            break;
    }
}

void TPM_HAL_DisableChn(uint32_t baseAddr, uint8_t channel)
{

    TPM_HAL_SetChnCountVal(baseAddr, channel, 0);
    TPM_HAL_SetChnMsnbaElsnbaVal(baseAddr, channel, 0);
    TPM_HAL_SetCpwms(baseAddr, 0);
}

/*see fsl_tpm_hal.h for documentation of this function*/
void TPM_HAL_Reset(uint32_t baseAddr, uint32_t instance)
{
    uint8_t chan = FSL_FEATURE_TPM_CHANNEL_COUNTn(instance);
    int i;

    HW_TPM_SC_WR(baseAddr, 0);
    TPM_HAL_SetMod(baseAddr, 0xffff);

    /*instance 1 and 2 only has two channels,0 and 1*/
    for(i = 0; i < chan; i++)
    {
        HW_TPM_CnSC_WR(baseAddr, i, 0);
        HW_TPM_CnV_WR(baseAddr, i, 0);
    }

    HW_TPM_STATUS_WR(baseAddr, BM_TPM_STATUS_CH0F | BM_TPM_STATUS_CH1F | BM_TPM_STATUS_TOF);
    HW_TPM_CONF_WR(baseAddr, 0);
}


/*******************************************************************************
 * EOF
 ******************************************************************************/

