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

#include "fsl_cadc_hal.h"


/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_Init
 * Description   : Initialize all the ADC registers to a known state.
 *
 *END**************************************************************************/
void CADC_HAL_Init(uint32_t baseAddr)
{
    uint32_t i;
    
    HW_ADC_CTRL1_WR(baseAddr, 0x6060U);
    HW_ADC_CTRL2_WR(baseAddr, 0x6044U);
    HW_ADC_ZXCTRL1_WR(baseAddr,0U);
    HW_ADC_ZXCTRL2_WR(baseAddr,0U);
    HW_ADC_CLIST1_WR(baseAddr, 0x3210U);
    HW_ADC_CLIST2_WR(baseAddr, 0x7654U);
    HW_ADC_CLIST3_WR(baseAddr, 0xBA98U);
    HW_ADC_CLIST4_WR(baseAddr, 0xFEDEU);
    HW_ADC_SDIS_WR(baseAddr, 0xF0F0U);
    HW_ADC_STAT_WR(baseAddr, 0x1800U);
    HW_ADC_LOLIMSTAT_WR(baseAddr, 0xFFFFU);
    HW_ADC_HILIMSTAT_WR(baseAddr, 0xFFFFU);
    HW_ADC_ZXSTAT_WR(baseAddr, 0xFFFFU);
    for (i = 0U; i < HW_ADC_RSLTn_COUNT; i++)
    {
        HW_ADC_RSLTn_WR(baseAddr, i, 0U);
    }
    for (i = 0U; i < HW_ADC_LOLIMn_COUNT; i++)
    {
        HW_ADC_LOLIMn_WR(baseAddr, i, 0U);
    }
    for (i = 0U; i < HW_ADC_HILIMn_COUNT; i++)
    {
        HW_ADC_HILIMn_WR(baseAddr, i, 0U);
    }
    for (i = 0U; i < HW_ADC_OFFSTn_COUNT; i++)
    {
        HW_ADC_OFFSTn_WR(baseAddr, i, 0U);
    }
    HW_ADC_PWR_WR(baseAddr, 0x1DA7U);
    HW_ADC_CAL_WR(baseAddr, 0U);
    HW_ADC_GC1_WR(baseAddr, 0U);
    HW_ADC_GC2_WR(baseAddr, 0U);
    HW_ADC_SCTRL_WR(baseAddr, 0U);
    HW_ADC_PWR2_WR(baseAddr, 0x0400U);
    HW_ADC_CTRL3_WR(baseAddr, 0U);
    HW_ADC_SCHLTEN_WR(baseAddr, 0U);
    
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvDmaCmd
 * Description   : Switch to enable DMA for ADC converter.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvDmaCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_CTRL1_DMAEN0(baseAddr, enable?1U:0U );
        break;
    case kCAdcConvB:
        BW_ADC_CTRL2_DMAEN1(baseAddr, enable?1U:0U );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvStopCmd
 * Description   : Switch to enable DMA for ADC converter.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvStopCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_CTRL1_STOP0(baseAddr, enable?1U:0U );
        break;
    case kCAdcConvB:
        BW_ADC_CTRL2_STOP1(baseAddr, enable?1U:0U );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvStartCmd
 * Description   : Trigger the ADC converter's conversion by software.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvStartCmd(uint32_t baseAddr, cadc_conv_id_t convId)
{
    switch (convId)
    {
    case kCAdcConvA:
        HW_ADC_CTRL1_SET(baseAddr, BF_ADC_CTRL1_START0(1U) );
        break;
    case kCAdcConvB:
        HW_ADC_CTRL2_SET(baseAddr, BF_ADC_CTRL2_START1(1U) );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvSyncCmd
 * Description   : Switch to enable input SYNC signal to trigger conversion.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvSyncCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_CTRL1_SYNC0(baseAddr, enable?1U:0U );
        break;
    case kCAdcConvB:
        BW_ADC_CTRL2_SYNC1(baseAddr, enable?1U:0U );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvEndOfScanIntCmd
 * Description   : Switch to enable interrupt caused by end of scan for each
 * converter.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvEndOfScanIntCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_CTRL1_EOSIE0(baseAddr, enable?1U:0U );
        break;
    case kCAdcConvB:
        BW_ADC_CTRL2_EOSIE1(baseAddr, enable?1U:0U );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetChnDiffCmd
 * Description   : Switch to enable differential sample mode for ADC conversion 
 * channel.
 *
 *END**************************************************************************/
void CADC_HAL_SetChnDiffCmd(uint32_t baseAddr, cadc_diff_chn_mode_t chns, bool enable)
{
    uint16_t tmp16;
    switch (chns)
    {
    case kCAdcDiffChnANB0_1:
    case kCAdcDiffChnANB2_3:
        /* chns -= 2U; */
    case kCAdcDiffChnANA0_1:
    case kCAdcDiffChnANA2_3:
        if ( (kCAdcDiffChnANB0_1 == chns) || (kCAdcDiffChnANB2_3 == chns) )
        {
            chns -= 2U;
        }
        tmp16 = HW_ADC_CTRL1_RD(baseAddr);
        tmp16 &= ~(1U<<(BP_ADC_CTRL1_CHNCFG_L+(uint16_t)chns) );
        if (enable)
        {
            tmp16 |= (1U<<(BP_ADC_CTRL1_CHNCFG_L+(uint16_t)chns) );
        }
        HW_ADC_CTRL1_WR(baseAddr, tmp16);
        break;
    case kCAdcDiffChnANB4_5:
    case kCAdcDiffChnANB6_7:
        /* chns -= 2U; */
    case kCAdcDiffChnANA4_5:
    case kCAdcDiffChnANA6_7:
        if ( (kCAdcDiffChnANB4_5 == chns) || (kCAdcDiffChnANB6_7 == chns) )
        {
            chns -= 2U;
        }
        chns -= 2U;
        tmp16 = HW_ADC_CTRL2_RD(baseAddr);
        tmp16 &= ~(1U<<(BP_ADC_CTRL2_CHNCFG_H + (uint16_t)chns) );
        if (enable)
        {
            tmp16 |= (1U<<(BP_ADC_CTRL2_CHNCFG_H + (uint16_t)chns) );
        }
        HW_ADC_CTRL2_WR(baseAddr, tmp16);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvClkDiv
 * Description   : Set divider for conversion clock from system clock.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvClkDiv(uint32_t baseAddr, cadc_conv_id_t convId, uint16_t divider)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_CTRL2_DIV0(baseAddr, divider );
        break;
    case kCAdcConvB:
        BW_ADC_PWR2_DIV1(baseAddr, divider );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetSlotZeroCrossingMode
 * Description   : Set zero crossing detection for each slot in conversion
 * sequence.
 *
 *END**************************************************************************/
void CADC_HAL_SetSlotZeroCrossingMode(uint32_t baseAddr, uint32_t slotNum, 
    cadc_zero_crossing_mode_t mode)
{
    uint16_t tmp16;
    
    if (slotNum < 8U) /* Slot 0 - 7. */
    {
        tmp16 = HW_ADC_ZXCTRL1_RD(baseAddr) & ~((0x3U)<<(slotNum*2U));
        tmp16 |= (uint16_t)( ((uint16_t)(mode))<<(slotNum*2U) );
        HW_ADC_ZXCTRL1_WR(baseAddr, tmp16);
    }
    else if (slotNum < 16U) /* Slot 8 - 15. */
    {
        slotNum -= 8U;
        tmp16 = HW_ADC_ZXCTRL2_RD(baseAddr) & ~((0x3U)<<(slotNum*2U));
        tmp16 |= (uint16_t)( ((uint16_t)(mode))<<(slotNum*2U) );
        HW_ADC_ZXCTRL2_WR(baseAddr, tmp16);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetSlotSampleChn
 * Description   : Set sample channel for each slot in conversion sequence.
 *
 *END**************************************************************************/
void CADC_HAL_SetSlotSampleChn(uint32_t baseAddr, uint32_t slotNum, 
    cadc_diff_chn_mode_t diffChns, cadc_chn_sel_mode_t selMode)
{
    uint16_t tmp16;
    
    if (slotNum < 4U) /* Slot 0 - 3. */
    {
        tmp16 = HW_ADC_CLIST1_RD(baseAddr) & (uint16_t)(~(uint16_t)(((uint16_t)(0xFU))<<(slotNum*4U)));
        switch (selMode)
        {       
        case kCAdcChnSelP:
        case kCAdcChnSelBoth:
            tmp16 |= (uint16_t)((((uint16_t)(diffChns))<<1)<<(slotNum*4U));
            break;
        case kCAdcChnSelN:
            tmp16 |= (uint16_t)(((((uint16_t)(diffChns))<<1)+1U)<<(slotNum*4U));
            break;
        default:
            break;
        }
        HW_ADC_CLIST1_WR(baseAddr, tmp16);
        
    }
    else if (slotNum < 8U) /* Slot 4 - 7. */
    {
        slotNum -= 4U;
        tmp16 = HW_ADC_CLIST2_RD(baseAddr) & (uint16_t)(~(uint16_t)((((uint16_t)(0xFU))<<(slotNum*4U))));
        switch (selMode)
        {       
        case kCAdcChnSelP:
        case kCAdcChnSelBoth:
            tmp16 |= (uint16_t)((((uint16_t)(diffChns))<<1)<<(slotNum*4U));
            break;
        case kCAdcChnSelN:
            tmp16 |= (uint16_t)(((((uint16_t)(diffChns))<<1)+1U)<<(slotNum*4U));
            break;
        default:
            break;
        }       
        HW_ADC_CLIST2_WR(baseAddr, tmp16);
    }
    else if (slotNum < 12U) /* Slot 8 - 11U. */
    {
        slotNum -= 8U;
        tmp16 = HW_ADC_CLIST3_RD(baseAddr) & (uint16_t)(~(uint16_t)((((uint16_t)(0xFU))<<(slotNum*4U))));
        switch (selMode)
        {       
        case kCAdcChnSelP:
        case kCAdcChnSelBoth:
            tmp16 |= (uint16_t)((((uint16_t)(diffChns))<<1)<<(slotNum*4U));
            break;
        case kCAdcChnSelN:
            tmp16 |= (uint16_t)(((((uint16_t)(diffChns))<<1)+1U)<<(slotNum*4U));
            break;
        default:
            break;
        }
        HW_ADC_CLIST3_WR(baseAddr, tmp16);

    }
    else if (slotNum < 16U) /* Slot 12 - 15U. */
    {
        slotNum -= 12U;
        tmp16 = HW_ADC_CLIST4_RD(baseAddr) & (uint16_t)(~(uint16_t)(((uint16_t)(0xFU))<<(slotNum*4U)));
        switch (selMode)
        {       
        case kCAdcChnSelP:
        case kCAdcChnSelBoth:
            tmp16 |= (uint16_t)((((uint16_t)(diffChns))<<1)<<(slotNum*4U));
            break;
        case kCAdcChnSelN:
            tmp16 |= (uint16_t)(((((uint16_t)(diffChns))<<1)+1U)<<(slotNum*4U));
            break;
        default:
            break;
        }
        HW_ADC_CLIST4_WR(baseAddr, tmp16);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetSlotSampleEnableCmd
 * Description   : Switch to enable sample for each slot in conversion sequence.
 *
 *END**************************************************************************/
void CADC_HAL_SetSlotSampleEnableCmd(uint32_t baseAddr, uint32_t slotNum, bool enable)
{
    uint16_t tmp16;
    tmp16 = HW_ADC_SDIS_RD(baseAddr) & ~(1U<<slotNum) | (enable?0U:(1U<<slotNum));
    HW_ADC_SDIS_WR(baseAddr, tmp16);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_GetConvInProgressFlag
 * Description   : Check if the conversion is in process for each converter.
 *
 *END**************************************************************************/
bool CADC_HAL_GetConvInProgressFlag(uint32_t baseAddr, cadc_conv_id_t convId)
{
    bool bRet = false;
    switch (convId)
    {
    case kCAdcConvA:
        bRet = (1U==BR_ADC_STAT_CIP0(baseAddr) );
        break;
    case kCAdcConvB:
        bRet = (1U==BR_ADC_STAT_CIP1(baseAddr) );
        break;
    default:
        bRet = false;
        break;
    }
    return bRet;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_GetConvEndOfScanIntFlag
 * Description   : Check if the end of scan event is asserted.
 *
 *END**************************************************************************/
bool CADC_HAL_GetConvEndOfScanIntFlag(uint32_t baseAddr, cadc_conv_id_t convId)
{
    bool bRet = false;
    switch (convId)
    {
    case kCAdcConvA:
        bRet = (1U==BR_ADC_STAT_EOSI0(baseAddr) );
        break;
    case kCAdcConvB:
        bRet = (1U==BR_ADC_STAT_EOSI1(baseAddr) );
        break;
    default:
        bRet = false;
        break;
    }
    return bRet;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_ClearConvEndOfScanIntFlag
 * Description   : Clear the end of scan flag.
 *
 *END**************************************************************************/
void CADC_HAL_ClearConvEndOfScanIntFlag(uint32_t baseAddr, cadc_conv_id_t convId)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_STAT_EOSI0(baseAddr, 1U);
        break;
    case kCAdcConvB:
        BW_ADC_STAT_EOSI1(baseAddr, 1U);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_GetConvPowerUpFlag
 * Description   : Check if the ADC converter is powered up.
 *
 *END**************************************************************************/
bool CADC_HAL_GetConvPowerUpFlag(uint32_t baseAddr, cadc_conv_id_t convId)
{
    bool bRet = false;
    switch (convId)
    {
    case kCAdcConvA:
        bRet = (0U == BR_ADC_PWR_PSTS0(baseAddr) );
        break;
    case kCAdcConvB:
        bRet = (0U == BR_ADC_PWR_PSTS1(baseAddr) );
        break;
    default:
        break;
    }
    return bRet;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvPowerUpCmd
 * Description   : Switch to enable power up for each ADC converter.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvPowerUpCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_PWR_PD0(baseAddr, enable?0U:1U );
        break;
    case kCAdcConvB:
        BW_ADC_PWR_PD1(baseAddr, enable?0U:1U );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvUseChnVrefHCmd
 * Description   : Switch to use channel 3 as VrefH for each ADC converter.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvUseChnVrefHCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_CAL_SEL_VREFH_A(baseAddr, enable?1U:0U );
        break;
    case kCAdcConvB:
        BW_ADC_CAL_SEL_VREFH_B(baseAddr, enable?1U:0U );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvUseChnVrefLCmd
 * Description   : Switch to use channel 2 as VrefL for each ADC converter.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvUseChnVrefLCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable)\
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_CAL_SEL_VREFLO_A(baseAddr, enable?1U:0U );
        break;
    case kCAdcConvB:
        BW_ADC_CAL_SEL_VREFLO_B(baseAddr, enable?1U:0U );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetSlotGainMode
 * Description   : Set the amplification for each slot in sequence.
 *
 *END**************************************************************************/
void CADC_HAL_SetChnGainMode(uint32_t baseAddr, uint32_t chnNum, cadc_gain_mode_t mode)
{
    uint16_t tmp16;
    
    if (chnNum < 8U) /* Slot 0 - 7. */
    {
        tmp16 = HW_ADC_GC1_RD(baseAddr) & ~(0x3U<<(chnNum*2U));
        tmp16 |=  (uint16_t)(((uint16_t)(mode))<<(chnNum*2U));
        HW_ADC_GC1_WR(baseAddr, tmp16);
    }
    else if (chnNum < 16U) /* Slot 8 - 15. */
    {
        chnNum -= 8U;
        tmp16 = HW_ADC_GC2_RD(baseAddr) & ~(0x3U<<(chnNum*2U));
        tmp16 |=  (uint16_t)(((uint16_t)(mode))<<(chnNum*2U));
        HW_ADC_GC2_WR(baseAddr, tmp16);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetSlotSyncPointCmd
 * Description   : Switch to enable the sync point for each slot in sequence.
 *
 *END**************************************************************************/
void CADC_HAL_SetSlotSyncPointCmd(uint32_t baseAddr, uint32_t slotNum, bool enable)

{
    uint16_t tmp16;

    tmp16 = BR_ADC_SCTRL_SC(baseAddr) & ~(1U<<slotNum);
    if (enable)
    {
        tmp16 |= (1U<<slotNum);
    }
    BW_ADC_SCTRL_SC(baseAddr, tmp16);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvSpeedLimitMode
 * Description   : Set the conversion speed control mode for each ADC converter.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvSpeedLimitMode(uint32_t baseAddr, cadc_conv_id_t convId,
    cadc_conv_speed_mode_t mode)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_PWR2_SPEEDA(baseAddr, (uint16_t)mode );
        break;
    case kCAdcConvB:
        BW_ADC_PWR2_SPEEDB(baseAddr, (uint16_t)mode );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetConvSampleWindow
 * Description   : Set the conversion speed control mode for each ADC converter.
 *
 *END**************************************************************************/
void CADC_HAL_SetConvSampleWindow(uint32_t baseAddr, cadc_conv_id_t convId, uint16_t val)
{
    switch (convId)
    {
    case kCAdcConvA:
        BW_ADC_CTRL3_SCNT0(baseAddr, (uint16_t)val );
        break;
    case kCAdcConvB:
        BW_ADC_CTRL3_SCNT1(baseAddr, (uint16_t)val );
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CADC_HAL_SetSlotScanIntCmd
 * Description   : Switch to enable scan interrupt for each slot in sequence.
 *
 *END**************************************************************************/
void CADC_HAL_SetSlotScanIntCmd(uint32_t baseAddr, uint32_t slotNum, bool enable)
{
    uint16_t tmp16;

    tmp16 = BR_ADC_SCHLTEN_SCHLTEN(baseAddr) & ~(1U<<slotNum);
    if (enable)
    {
        tmp16 |= (1U<<slotNum);
    }
    BW_ADC_SCHLTEN_SCHLTEN(baseAddr, tmp16);
    
}

/*******************************************************************************
 * EOF
 ******************************************************************************/

