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
#include "fsl_device_registers.h"
#include "fsl_sim_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * APIs
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_SetOutDiv
 * Description   : Set all clock out dividers setting at the same time
 * This function will set the setting for all clock out dividers.
 *
 *END**************************************************************************/
void CLOCK_HAL_SetOutDiv(uint32_t baseAddr,
                         uint8_t outdiv1,
                         uint8_t outdiv2,
                         uint8_t outdiv3,
                         uint8_t outdiv4)
{
    uint32_t clkdiv1 = 0;

    clkdiv1 |= BF_SIM_CLKDIV1_OUTDIV1(outdiv1);
    clkdiv1 |= BF_SIM_CLKDIV1_OUTDIV2(outdiv2);
    clkdiv1 |= BF_SIM_CLKDIV1_OUTDIV4(outdiv4);

    HW_SIM_CLKDIV1_WR(baseAddr, clkdiv1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CLOCK_HAL_GetOutDiv
 * Description   : Get all clock out dividers setting at the same time
 * This function will get the setting for all clock out dividers.
 *
 *END**************************************************************************/
void CLOCK_HAL_GetOutDiv(uint32_t baseAddr,
                         uint8_t *outdiv1,
                         uint8_t *outdiv2,
                         uint8_t *outdiv3,
                         uint8_t *outdiv4)
{
    *outdiv1 = BR_SIM_CLKDIV1_OUTDIV1(baseAddr);
    *outdiv2 = BR_SIM_CLKDIV1_OUTDIV2(baseAddr);
    *outdiv3 = 0U;
    *outdiv4 = BR_SIM_CLKDIV1_OUTDIV4(baseAddr);
}


/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetAdcAlternativeTriggerCmd
 * Description   : Set ADCx alternate trigger enable setting
 * This function will enable/disable alternative conversion triggers for ADCx.
 *
 *END**************************************************************************/
void SIM_HAL_SetAdcAlternativeTriggerCmd(uint32_t baseAddr,
                                         uint32_t instance,
                                         bool enable)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        BW_SIM_SOPT7_ADC0ALTTRGEN(baseAddr, enable ? 1 : 0);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetAdcAlternativeTriggerCmd
 * Description   : Get ADCx alternate trigger enable setting
 * This function will get ADCx alternate trigger enable setting.
 *
 *END**************************************************************************/
bool SIM_HAL_GetAdcAlternativeTriggerCmd(uint32_t baseAddr, uint32_t instance)
{
    bool retValue = false;

    assert(instance < HW_ADC_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        retValue = BR_SIM_SOPT7_ADC0ALTTRGEN(baseAddr);
        break;
    default:
        retValue = false;
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetAdcPreTriggerMode
 * Description   : Set ADCx pre-trigger select setting
 * This function will select the ADCx pre-trigger source when alternative
 * triggers are enabled through ADCxALTTRGEN
 *
 *END**************************************************************************/
void SIM_HAL_SetAdcPreTriggerMode(uint32_t baseAddr,
                                  uint32_t instance,
                                  sim_adc_pretrg_sel_t select)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        BW_SIM_SOPT7_ADC0PRETRGSEL(baseAddr, select);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetAdcPreTriggerMode
 * Description   : Get ADCx pre-trigger select setting
 * This function will get ADCx pre-trigger select setting.
 *
 *END**************************************************************************/
sim_adc_pretrg_sel_t SIM_HAL_GetAdcPreTriggerMode(uint32_t baseAddr,
                                                  uint32_t instance)
{
    sim_adc_pretrg_sel_t retValue = (sim_adc_pretrg_sel_t)0;

    assert(instance < HW_ADC_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        retValue = (sim_adc_pretrg_sel_t)BR_SIM_SOPT7_ADC0PRETRGSEL(baseAddr);
        break;
    default:
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetAdcTriggerMode
 * Description   : Set ADCx trigger select setting
 * This function will select the ADCx trigger source when alternative triggers
 * are enabled through ADCxALTTRGEN
 *
 *END**************************************************************************/
void SIM_HAL_SetAdcTriggerMode(uint32_t baseAddr,
                               uint32_t instance,
                               sim_adc_trg_sel_t select)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        BW_SIM_SOPT7_ADC0TRGSEL(baseAddr, select);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetAdcTriggerMode
 * Description   : Get ADCx trigger select setting
 * This function will get ADCx trigger select setting.
 *
 *END**************************************************************************/
sim_adc_trg_sel_t SIM_HAL_GetAdcTriggerMode(uint32_t baseAddr, uint32_t instance)
{
    sim_adc_trg_sel_t retValue = (sim_adc_trg_sel_t)0;

    assert(instance < HW_ADC_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        retValue = (sim_adc_trg_sel_t)BR_SIM_SOPT7_ADC0TRGSEL(baseAddr);
        break;
    default:
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetAdcTriggerModeOneStep
 * Description   : Set ADCx trigger setting.
 * This function sets ADC alternate trigger, pre-trigger mode and trigger mode.
 *
 *END**************************************************************************/
void SIM_HAL_SetAdcTriggerModeOneStep(uint32_t baseAddr,
                                      uint32_t instance,
                                      bool    altTrigEn,
                                      sim_adc_pretrg_sel_t preTrigSel,
                                      sim_adc_trg_sel_t trigSel)
{
    assert(instance < HW_ADC_INSTANCE_COUNT);

    switch (instance)
    {
        case 0:
            BW_SIM_SOPT7_ADC0ALTTRGEN(baseAddr, altTrigEn ? 1 : 0);
            BW_SIM_SOPT7_ADC0PRETRGSEL(baseAddr, preTrigSel);
            break;
    default:
        break;
    }

    if (altTrigEn)
    {
        switch (instance)
        {
            case 0:
                BW_SIM_SOPT7_ADC0TRGSEL(baseAddr, trigSel);
                break;
            default:
                break;
        }
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetUartRxSrcMode
 * Description   : Set UARTx receive data source select setting
 * This function will select the source for the UART1 receive data.
 *
 *END**************************************************************************/
void SIM_HAL_SetUartRxSrcMode(uint32_t baseAddr,
                              uint32_t instance,
                              sim_uart_rxsrc_t select)
{
    assert(instance < FSL_FEATURE_SIM_OPT_UART_COUNT);

    switch (instance)
    {
    case 0:
        BW_SIM_SOPT5_UART0RXSRC(baseAddr, select);
        break;
    case 1:
        BW_SIM_SOPT5_UART1RXSRC(baseAddr, select);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetUartRxSrcMode
 * Description   : Get UARTx receive data source select setting
 * This function will get UARTx receive data source select setting.
 *
 *END**************************************************************************/
sim_uart_rxsrc_t SIM_HAL_GetUartRxSrcMode(uint32_t baseAddr, uint32_t instance)
{
    sim_uart_rxsrc_t retValue = (sim_uart_rxsrc_t)0;

    assert(instance < FSL_FEATURE_SIM_OPT_UART_COUNT);

    switch (instance)
    {
    case 0:
        retValue = (sim_uart_rxsrc_t)BR_SIM_SOPT5_UART0RXSRC(baseAddr);
        break;
    case 1:
        retValue = (sim_uart_rxsrc_t)BR_SIM_SOPT5_UART1RXSRC(baseAddr);
        break;
    default:
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetUartTxSrcMode
 * Description   : Set UARTx transmit data source select setting
 * This function will select the source for the UARTx transmit data.
 *
 *END**************************************************************************/
void SIM_HAL_SetUartTxSrcMode(uint32_t baseAddr,
                              uint32_t instance,
                              sim_uart_txsrc_t select)
{
    assert(instance < FSL_FEATURE_SIM_OPT_UART_COUNT);

    switch (instance)
    {
    case 0:
        BW_SIM_SOPT5_UART0TXSRC(baseAddr, select);
        break;
    case 1:
        BW_SIM_SOPT5_UART1TXSRC(baseAddr, select);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetUartTxSrcMode
 * Description   : Get UARTx transmit data source select setting
 * This function will get UARTx transmit data source select setting.
 *
 *END**************************************************************************/
sim_uart_txsrc_t SIM_HAL_GetUartTxSrcMode(uint32_t baseAddr, uint32_t instance)
{
    sim_uart_txsrc_t retValue =(sim_uart_txsrc_t)0;

    assert(instance < FSL_FEATURE_SIM_OPT_UART_COUNT);

    switch (instance)
    {
    case 0:
        retValue = (sim_uart_txsrc_t)BR_SIM_SOPT5_UART0TXSRC(baseAddr);
        break;
    case 1:
        retValue = (sim_uart_txsrc_t)BR_SIM_SOPT5_UART1TXSRC(baseAddr);
        break;
    default:
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetFtmTriggerSrcMode
 * Description   : Set FlexTimer x hardware trigger y source select setting
 * This function will select the source of FTMx hardware trigger y.
 *
 *END**************************************************************************/
void SIM_HAL_SetFtmTriggerSrcMode(uint32_t baseAddr,
                                  uint32_t instance,
                                  uint8_t  trigger,
                                  sim_ftm_trg_src_t select)
{
    assert (instance < HW_FTM_INSTANCE_COUNT);
    assert (trigger < FSL_FEATURE_SIM_OPT_FTM_TRIGGER_COUNT);

    switch (instance)
    {
    case 0:
        switch (trigger)
        {
        case 0:
            BW_SIM_SOPT4_FTM0TRG0SRC(baseAddr, select);
            break;
        case 1:
            BW_SIM_SOPT4_FTM0TRG1SRC(baseAddr, select);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetFtmTriggerSrcMode
 * Description   : Get FlexTimer x hardware trigger y source select setting
 * This function will get FlexTimer x hardware trigger y source select setting.
 *
 *END**************************************************************************/
sim_ftm_trg_src_t SIM_HAL_GetFtmTriggerSrcMode(uint32_t baseAddr,
                                               uint32_t instance,
                                               uint8_t trigger)
{
    sim_ftm_trg_src_t retValue = (sim_ftm_trg_src_t)0;

    assert (instance < HW_FTM_INSTANCE_COUNT);
    assert (trigger < FSL_FEATURE_SIM_OPT_FTM_TRIGGER_COUNT);

    switch (instance)
    {
    case 0:
        switch (trigger)
        {
        case 0:
            retValue = (sim_ftm_trg_src_t)BR_SIM_SOPT4_FTM0TRG0SRC(baseAddr);
            break;
        case 1:
            retValue = (sim_ftm_trg_src_t)BR_SIM_SOPT4_FTM0TRG1SRC(baseAddr);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetFtmExternalClkPinMode
 * Description   : Set FlexTimer x external clock pin select setting
 * This function will select the source of FTMx external clock pin select
 *
 *END**************************************************************************/
void SIM_HAL_SetFtmExternalClkPinMode(uint32_t baseAddr,
                                      uint32_t instance,
                                      sim_ftm_clk_sel_t select)
{
    assert (instance < HW_FTM_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        BW_SIM_SOPT4_FTM0CLKSEL(baseAddr, select);
        break;
    case 1:
        BW_SIM_SOPT4_FTM1CLKSEL(baseAddr, select);
        break;
    case 2:
        BW_SIM_SOPT4_FTM2CLKSEL(baseAddr, select);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetFtmExternalClkPinMode
 * Description   : Get FlexTimer x external clock pin select setting
 * This function will get FlexTimer x external clock pin select setting.
 *
 *END**************************************************************************/
sim_ftm_clk_sel_t SIM_HAL_GetFtmExternalClkPinMode(uint32_t baseAddr,
                                                   uint32_t instance)
{
    sim_ftm_clk_sel_t retValue = (sim_ftm_clk_sel_t)0;

    assert (instance < HW_FTM_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        retValue = (sim_ftm_clk_sel_t)BR_SIM_SOPT4_FTM0CLKSEL(baseAddr);
        break;
    case 1:
        retValue = (sim_ftm_clk_sel_t)BR_SIM_SOPT4_FTM1CLKSEL(baseAddr);
        break;
    case 2:
        retValue = (sim_ftm_clk_sel_t)BR_SIM_SOPT4_FTM2CLKSEL(baseAddr);
        break;
    default:
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetFtmChSrcMode
 * Description   : FlexTimer x channel y input capture source select setting
 * This function will select FlexTimer x channel y input capture source
 *
 *END**************************************************************************/
void SIM_HAL_SetFtmChSrcMode(uint32_t baseAddr,
                             uint32_t instance,
                             uint8_t  channel,
                             sim_ftm_ch_src_t select)
{
    assert (instance < HW_FTM_INSTANCE_COUNT);

    switch (instance)
    {
    case 1:
        switch (channel)
        {
        case 0:
            BW_SIM_SOPT4_FTM1CH0SRC(baseAddr, select);
            break;
        default:
            break;
        }
        break;
    case 2:
        switch (channel)
        {
        case 0:
            BW_SIM_SOPT4_FTM2CH0SRC(baseAddr, select);
            break;
        case 1:
            BW_SIM_SOPT4_FTM2CH1SRC(baseAddr, select);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetFtmChSrcMode
 * Description   : Get FlexTimer x channel y input capture source select setting
 * This function will get FlexTimer x channel y input capture source select
 * setting.
 *
 *END**************************************************************************/
sim_ftm_ch_src_t SIM_HAL_GetFtmChSrcMode(uint32_t baseAddr,
                                         uint32_t instance,
                                         uint8_t channel)
{
    sim_ftm_ch_src_t retValue = (sim_ftm_ch_src_t)0;

    assert (instance < HW_FTM_INSTANCE_COUNT);

    switch (instance)
    {
    case 1:
        switch (channel)
        {
        case 0:
            retValue = (sim_ftm_ch_src_t)BR_SIM_SOPT4_FTM1CH0SRC(baseAddr);
            break;
        default:
            break;
        }
        break;
    case 2:
        switch (channel)
        {
        case 0:
            retValue = (sim_ftm_ch_src_t)BR_SIM_SOPT4_FTM2CH0SRC(baseAddr);
            break;
        case 1:
            retValue = (sim_ftm_ch_src_t)BR_SIM_SOPT4_FTM2CH1SRC(baseAddr);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return retValue;
}



/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetFtmFaultSelMode
 * Description   : Set FlexTimer x fault y select setting
 * This function will set the FlexTimer x fault y select setting.
 *
 *END**************************************************************************/
void SIM_HAL_SetFtmFaultSelMode(uint32_t baseAddr,
                                uint32_t instance,
                                uint8_t  fault,
                                sim_ftm_flt_sel_t select)
{
    assert (instance < HW_FTM_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        switch (fault)
        {
        case 0:
            BW_SIM_SOPT4_FTM0FLT0(baseAddr, select);
            break;
        case 1:
            BW_SIM_SOPT4_FTM0FLT1(baseAddr, select);
            break;
        default:
            break;
        }
        break;
    case 1:
        BW_SIM_SOPT4_FTM1FLT0(baseAddr, select);
        break;
    case 2:
        BW_SIM_SOPT4_FTM2FLT0(baseAddr, select);
        break;
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetFtmFaultSelMode
 * Description   : Get FlexTimer x fault y select setting
 * This function will get FlexTimer x fault y select setting.
 *
 *END**************************************************************************/
sim_ftm_flt_sel_t SIM_HAL_GetFtmFaultSelMode(uint32_t baseAddr,
                                             uint32_t instance,
                                             uint8_t fault)
{
    sim_ftm_flt_sel_t retValue = (sim_ftm_flt_sel_t)0;

    assert (instance < HW_FTM_INSTANCE_COUNT);

    switch (instance)
    {
    case 0:
        switch (fault)
        {
        case 0:
            retValue = (sim_ftm_flt_sel_t)BR_SIM_SOPT4_FTM0FLT0(baseAddr);
            break;
        case 1:
            retValue = (sim_ftm_flt_sel_t)BR_SIM_SOPT4_FTM0FLT1(baseAddr);
            break;
        default:
            break;
        }
        break;
    case 1:
        retValue = (sim_ftm_flt_sel_t)BR_SIM_SOPT4_FTM1FLT0(baseAddr);
        break;
    case 2:
        retValue = (sim_ftm_flt_sel_t)BR_SIM_SOPT4_FTM2FLT0(baseAddr);
        break;
    default:
        break;
    }

    return retValue;
}

/* Macro for FTMxOCHySRC. */
#define FTM_CH_OUT_SRC_MASK(instance, channel) \
    (1U << (((instance)>>1U) + channel + 16U))

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetFtmChOutSrcMode
 * Description   : FlexTimer x channel y output source select setting.
 * This function will select FlexTimer x channel y output source
 *
 *END**************************************************************************/
void SIM_HAL_SetFtmChOutSrcMode(uint32_t baseAddr,
                                uint32_t instance,
                                uint8_t channel,
                                sim_ftm_ch_out_src_t select)
{
    assert (0U==instance);
    assert (6U>channel);

    if (kSimFtmChOutSrc0 == select)
    {
        HW_SIM_SOPT8_CLR(baseAddr, FTM_CH_OUT_SRC_MASK(instance, channel));
    }
    else
    {
        HW_SIM_SOPT8_SET(baseAddr, FTM_CH_OUT_SRC_MASK(instance, channel));
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_GetFtmChOutSrcMode
 * Description   : Get FlexTimer x channel y output source select setting
 * This function will get FlexTimer x channel y output source select
 * setting.
 *
 *END**************************************************************************/
sim_ftm_ch_out_src_t SIM_HAL_GetFtmChOutSrcMode(uint32_t baseAddr,
                                                uint32_t instance,
                                                uint8_t channel)
{
    assert (0U==instance);
    assert (6U>channel);
    return (sim_ftm_ch_out_src_t)
        (HW_SIM_SOPT8_RD(baseAddr) & FTM_CH_OUT_SRC_MASK(instance, channel));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SIM_HAL_SetFtmSyncCmd
 * Description   : Set FTMxSYNCBIT
 * This function sets FlexTimer x hardware trigger 0 software synchronization.
 *
 *END**************************************************************************/
void SIM_HAL_SetFtmSyncCmd(uint32_t baseAddr, uint32_t instance, bool sync)
{
    assert (instance < HW_FTM_INSTANCE_COUNT);
    if (sync)
    {
        HW_SIM_SOPT8_SET(baseAddr, (1U<<instance));
    }
    else
    {
        HW_SIM_SOPT8_CLR(baseAddr, (1U<<instance));
    }
}

/*******************************************************************************
 * EOF
 ******************************************************************************/

