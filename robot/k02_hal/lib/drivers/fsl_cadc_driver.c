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

#include <assert.h>
#include "fsl_cadc_driver.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_StructInitUserConfigDefault
 * Description   : Help user to fill the cadc_user_config_t structure with
 * default setting, which can be used in polling mode for ADC conversion.
 * These setting are:
 *
 *END**************************************************************************/
cadc_status_t CADC_DRV_StructInitUserConfigDefault(cadc_user_config_t *configPtr)
{
    if (!configPtr)
    {
        return kStatus_CADC_InvalidArgument;
    }
    configPtr->zeroCrossingIntEnable = false;
    configPtr->lowLimitIntEnable = false;
    configPtr->highLimitIntEnable = false;
    configPtr->scanMode = kCAdcScanOnceSequential;
    configPtr->parallelSimultModeEnable = false;
    configPtr->dmaSrcMode = kCAdcDmaTriggeredByEndOfScan;
    configPtr->autoStandbyEnable = false;
    configPtr->powerUpDelayCount = 0x2AU;
    configPtr->autoPowerDownEnable = false;
    
    return kStatus_CADC_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_Init
 * Description   : Configure the CyclicADC module for the global configuraion
 * which are shared by all the converter.
 *
 *END**************************************************************************/
cadc_status_t CADC_DRV_Init(uint32_t instance, cadc_user_config_t *configPtr)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];
    
    if (!configPtr)
    {
        return kStatus_CADC_InvalidArgument;
    }
    /* Ungate the clock for the ADC module. */
    CLOCK_SYS_EnableAdcClock(instance); /* BW_SIM_SCGC5_ADC(SIM_BASE,1U); */

    /* Configure the common setting for ADC module. */
    CADC_HAL_Init(baseAddr);

    CADC_HAL_SetZeroCrossingIntCmd(baseAddr, configPtr->zeroCrossingIntEnable);
    CADC_HAL_SetLowLimitIntCmd(baseAddr, configPtr->lowLimitIntEnable);
    CADC_HAL_SetHighLimitIntCmd(baseAddr, configPtr->highLimitIntEnable);
    if (   (configPtr->zeroCrossingIntEnable) || (configPtr->lowLimitIntEnable)
        || (configPtr->highLimitIntEnable) )
    {
        NVIC_EnableIRQ(g_cadcErrIrqId[instance]);
    }
    else
    {
        NVIC_DisableIRQ(g_cadcErrIrqId[instance]);
    }
    CADC_HAL_SetScanMode(baseAddr, configPtr->scanMode);
    CADC_HAL_SetParallelSimultCmd(baseAddr, configPtr->parallelSimultModeEnable);

    CADC_HAL_SetAutoStandbyCmd(baseAddr, configPtr->autoStandbyEnable);
    CADC_HAL_SetPowerUpDelayClk(baseAddr, configPtr->powerUpDelayCount);
    CADC_HAL_SetAutoPowerDownCmd(baseAddr, configPtr->autoPowerDownEnable);
    CADC_HAL_SetDmaTriggerSrcMode(baseAddr, configPtr->dmaSrcMode);
    
    return kStatus_CADC_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_Deinit
 * Description   : Help user to fill the cadc_conv_config_t structure with
 * default setting, which can be used in polling mode for ADC conversion.
 * These setting are:
 *
 *END**************************************************************************/
void CADC_DRV_Deinit(uint32_t instance)
{
    NVIC_DisableIRQ(g_cadcErrIrqId[instance]);
    NVIC_DisableIRQ(g_cadcConvAIrqId[instance]);
    NVIC_DisableIRQ(g_cadcConvBIrqId[instance]);
    
    /* Gate the access to ADC module. */
    CLOCK_SYS_DisableAdcClock(instance); /* BW_SIM_SCGC5_ADC(SIM_BASE,0U); */
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_StructInitConvConfigDefault
 * Description   : Configure each converter in CyclicADC module. However, when
 * the multiple converter are co-working, the setting for converters would
 * be related with each other. For detailed information, please see to SOC's
 * Reference Manual document.
 *
 *END**************************************************************************/
cadc_status_t CADC_DRV_StructInitConvConfigDefault(cadc_conv_config_t *configPtr)
{
    if (!configPtr)
    {
        return kStatus_CADC_InvalidArgument;
    }

    configPtr->dmaEnable = false;
    configPtr->stopEnable = false; /* Release the converter. */
    configPtr->syncEnable = false; /* No hardware trigger. */

    /* No interrupt. */
    configPtr->endOfScanIntEnable = false; 
    configPtr->convIRQEnable = false;

    configPtr->clkDivValue = 0x3FU;
    configPtr->powerOnEnable = true;
    configPtr->useChnInputAsVrefH = false;
    configPtr->useChnInputAsVrefL = false;
    configPtr->speedMode = kCAdcConvClkLimitBy25MHz;
    configPtr->sampleWindowCount = 0U;

    return kStatus_CADC_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_ConfigConverter
 * Description   : Configure each converter in CyclicADC module. However, when
 * the multiple converter are co-working, the setting for converters would
 * be related with each other. For detailed information, please see to SOC's
 * Reference Manual document.
 *
 *END**************************************************************************/
cadc_status_t CADC_DRV_ConfigConverter(uint32_t instance, cadc_conv_id_t convId,
    cadc_conv_config_t *configPtr)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];

    if (!configPtr)
    {
        return kStatus_CADC_InvalidArgument;
    }

    /* Configure the ADC converter. */
    CADC_HAL_SetConvDmaCmd(baseAddr, convId, configPtr->dmaEnable);
    CADC_HAL_SetConvStopCmd(baseAddr, convId, configPtr->stopEnable);
    CADC_HAL_SetConvSyncCmd(baseAddr, convId, configPtr->syncEnable);
    CADC_HAL_SetConvEndOfScanIntCmd(baseAddr, convId, configPtr->endOfScanIntEnable);

    if (configPtr->convIRQEnable) /* NVIC Interrupt. */
    {
        switch (convId)
        {
        case kCAdcConvA:
            NVIC_EnableIRQ(g_cadcConvAIrqId[instance]);
            break;
        case kCAdcConvB:
            NVIC_EnableIRQ(g_cadcConvBIrqId[instance]);
            break;
        default:
            break;
        }
    }
    else
    {
        switch (convId)
        {
        case kCAdcConvA:
            NVIC_DisableIRQ(g_cadcConvAIrqId[instance]);
            break;
        case kCAdcConvB:
            NVIC_DisableIRQ(g_cadcConvBIrqId[instance]);
            break;
        default:
            break;
        }
    }
    CADC_HAL_SetConvClkDiv(baseAddr, convId, configPtr->clkDivValue);
    CADC_HAL_SetConvPowerUpCmd(baseAddr, convId, configPtr->powerOnEnable);
    CADC_HAL_SetConvUseChnVrefHCmd(baseAddr, convId, configPtr->useChnInputAsVrefH);
    CADC_HAL_SetConvUseChnVrefLCmd(baseAddr, convId, configPtr->useChnInputAsVrefL);
    CADC_HAL_SetConvSpeedLimitMode(baseAddr, convId, configPtr->speedMode);
    CADC_HAL_SetConvSampleWindow(baseAddr, convId, configPtr->sampleWindowCount);

    return kStatus_CADC_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_ConfigSampleChn
 * Description   : Configure input channel for ADC conversion. The CyclicADC
 * module's input channels are gathered in pairs. Here the configuration can
 * be set for each of channel in the pair.
 *
 *END**************************************************************************/
cadc_status_t CADC_DRV_ConfigSampleChn(uint32_t instance,
    cadc_diff_chn_mode_t diffChns, cadc_chn_config_t *configPtr)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];
    uint32_t chnNum;
    
    if (!configPtr)
    {
        return kStatus_CADC_InvalidArgument;
    }

    CADC_HAL_SetChnDiffCmd(baseAddr, diffChns, configPtr->diffEnable);

    /* Configure gain mode for indicated input channel. */
    switch (configPtr->diffSelMode)
    {
    case kCAdcChnSelN:
        chnNum = (uint32_t)(diffChns) * 2U;
        CADC_HAL_SetChnGainMode(baseAddr, chnNum,  configPtr->gainMode);
        break;
    case kCAdcChnSelP:
        chnNum = 1U + ((uint32_t)(diffChns) * 2U);
        CADC_HAL_SetChnGainMode(baseAddr, chnNum,  configPtr->gainMode);
        break;
    case kCAdcChnSelBoth:
        chnNum = (uint32_t)(diffChns) * 2U;
        CADC_HAL_SetChnGainMode(baseAddr, chnNum,  configPtr->gainMode);
        CADC_HAL_SetChnGainMode(baseAddr, 1U+chnNum,  configPtr->gainMode);
        break;
    default:
        break;
    }
    return kStatus_CADC_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_ConfigSeqSlot
 * Description   : Configure each slot in ADC conversion sequence. ADC conversion
 * sequence is the basic execution unit in this CyclicADC module. However, the
 * sequence should be configured slot by slot. The end of the sequence is a
 * slot that is configured as disable.
 *
 *END**************************************************************************/
cadc_status_t CADC_DRV_ConfigSeqSlot(uint32_t instance, uint32_t slotNum,
    cadc_slot_config_t *configPtr)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];
    
    if (!configPtr)
    {
        return kStatus_CADC_InvalidArgument;
    }
    CADC_HAL_SetSlotZeroCrossingMode(baseAddr, slotNum, configPtr->zeroCrossingMode);
    CADC_HAL_SetSlotSampleChn(baseAddr, slotNum, configPtr->diffChns, configPtr->diffSel);
    CADC_HAL_SetSlotSampleEnableCmd(baseAddr, slotNum, configPtr->slotEnable);
    CADC_HAL_SetSlotLowLimitValue(baseAddr, slotNum, configPtr->lowLimitValue);
    CADC_HAL_SetSlotHighLimitValue(baseAddr, slotNum, configPtr->highLimitValue);
    CADC_HAL_SetSlotOffsetValue(baseAddr, slotNum, configPtr->offsetValue);
    CADC_HAL_SetSlotSyncPointCmd(baseAddr, slotNum, configPtr->syncPointEnable);
    CADC_HAL_SetSlotScanIntCmd(baseAddr, slotNum, configPtr->scanIntEnable);
    
    return kStatus_CADC_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_SoftTriggerConv
 * Description   : trigger the ADC conversion by executing software command. It
 * will start the conversion if no other SYNC input (hardware trigger) is needed.
 *
 *END**************************************************************************/
void CADC_DRV_SoftTriggerConv(uint32_t instance, cadc_conv_id_t convId)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];

    CADC_HAL_SetConvStartCmd(baseAddr, convId);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_GetSeqSlotConvValueRAW
 * Description   : Read the conversion value from each slot in conversion sequence.
 * the return value would be just the absolute value without signed. 
 *
 *END**************************************************************************/
uint16_t CADC_DRV_GetSeqSlotConvValueRAW(uint32_t instance, uint32_t slotNum)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    assert(slotNum < HW_ADC_RSLTn_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];
    
    return CADC_HAL_GetSlotValueData(baseAddr, slotNum);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_GetSeqSlotConvValueSigned
 * Description   : Help to get the global flag of CyclicADC module. 
 *
 *END**************************************************************************/
int16_t CADC_DRV_GetSeqSlotConvValueSigned(uint32_t instance, uint32_t slotNum)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    assert(slotNum < HW_ADC_RSLTn_COUNT);
    
    uint32_t baseAddr = g_cadcBaseAddr[instance];
    int16_t ret = (int16_t)CADC_HAL_GetSlotValueData(baseAddr, slotNum);

    if (CADC_HAL_GetSlotValueNegativeSign(baseAddr, slotNum))
    {
        ret *= (-1);
    }
    return ret;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_GetFlag
 * Description   : Help to get the global flag of CyclicADC module.
 *
 *END**************************************************************************/
bool CADC_DRV_GetFlag(uint32_t instance, cadc_flag_t flag)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];
    bool bRet = false;

    switch(flag)
    {
    case kCAdcZeroCrossingInt:
        bRet = CADC_HAL_GetZeroCrossingIntFlag(baseAddr);
        break;
    case kCAdcLowLimitInt:
        bRet = CADC_HAL_GetLowLimitIntFlag(baseAddr);
        break;
    case kCAdcHighLimitInt:
        bRet = CADC_HAL_GetHighLimitIntFlag(baseAddr);
        break;
    default:
        break;
    }
    return bRet;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_ClearFlag
 * Description   : Clear the global flag of CyclicADC module.
 *
 *END**************************************************************************/
void CADC_DRV_ClearFlag(uint32_t instance, cadc_flag_t flag)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];

    switch(flag)
    {
    case kCAdcZeroCrossingInt:
        CADC_HAL_ClearAllZeorCrossingFlag(baseAddr);
        break;
    case kCAdcLowLimitInt:
        CADC_HAL_ClearAllLowLimitFlag(baseAddr);
        break;
    case kCAdcHighLimitInt:
        CADC_HAL_ClearAllHighLimitFlag(baseAddr);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_GetConvFlag
 * Description   : Help to get the flag of each converter's event.
 *
 *END**************************************************************************/
bool CADC_DRV_GetConvFlag(uint32_t instance, cadc_conv_id_t convId, cadc_flag_t flag)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];
    bool bRet = false;

    switch (flag)
    {
    case kCAdcConvInProgress:
        bRet = CADC_HAL_GetConvInProgressFlag(baseAddr, convId);
        break;
    case kCAdcConvEndOfScanInt:
        bRet = CADC_HAL_GetConvEndOfScanIntFlag(baseAddr, convId);
        break;
    case kCAdcConvPowerUp:
        bRet = CADC_HAL_GetConvPowerUpFlag(baseAddr, convId);
        break;
    default:
        break;
    }
    return bRet;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_ClearConvFlag
 * Description   : help to clear the flag of each converter's event.
 *
 *END**************************************************************************/
void CADC_DRV_ClearConvFlag(uint32_t instance, cadc_conv_id_t convId, cadc_flag_t flag)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];

    switch (flag)
    {
    case kCAdcConvEndOfScanInt:
        CADC_HAL_ClearConvEndOfScanIntFlag(baseAddr, convId);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_GetSlotFlag
 * Description   : Help to get the flag of each slot's event in conversion in 
 * sequence. 
 *
 *END**************************************************************************/
bool CADC_DRV_GetSlotFlag(uint32_t instance, uint32_t slotNum, cadc_flag_t flag)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];

    bool bRet = false;
    switch (flag)
    {
    case kCAdcSlotReady:
        bRet = CADC_HAL_GetSlotReadyFlag(baseAddr, slotNum);
        break;
    case kCAdcSlotLowLimitEvent:
        bRet = CADC_HAL_GetSlotLowLimitFlag(baseAddr, slotNum);
        break;
    case kCAdcSlotHighLimitEvent:
        bRet = CADC_HAL_GetSlotHighLimitFlag(baseAddr, slotNum);
        break;
    case kCAdcSlotCrossingEvent:
        bRet = CADC_HAL_GetSlotZeroCrossingFlag(baseAddr, slotNum);
        break;
    default:
        break;
    }
    return bRet;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_DRV_ClearSlotFlag
 * Description   : Help to clear the flag of each slot's event in conversion in 
 * sequence.
 *
 *END**************************************************************************/
void CADC_DRV_ClearSlotFlag(uint32_t instance, uint32_t slotNum, cadc_flag_t flag)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);
    uint32_t baseAddr = g_cadcBaseAddr[instance];

    switch (flag)
    {
    case kCAdcSlotLowLimitEvent:
        CADC_HAL_ClearSlotLowLimitFlag(baseAddr, slotNum);
        break;
    case kCAdcSlotHighLimitEvent:
        CADC_HAL_ClearSlotHighLimitFlag(baseAddr, slotNum);
        break;
    case kCAdcSlotCrossingEvent:
        CADC_HAL_ClearSlotZeroCrossingFlag(baseAddr, slotNum);
        break;
    default:
        break;
    }
}

/******************************************************************************
 * EOF
 *****************************************************************************/

