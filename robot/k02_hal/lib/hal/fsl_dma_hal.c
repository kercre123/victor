/*
* Copyright (c) 2013 - 2014, Freescale Semiconductor, Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*	of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*	list of conditions and the following disclaimer in the documentation and/or
*	other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*	contributors may be used to endorse or promote products derived from this
*	software without specific prior written permission.
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
#include "fsl_dma_hal.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : DMA_HAL_Init
 * Description   : Set all registers for the channel to 0.
 *
 *END**************************************************************************/
void DMA_HAL_Init(uint32_t baseAddr,uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    HW_DMA_SARn_WR(baseAddr,channel,0);
    HW_DMA_DARn_WR(baseAddr,channel,0);
    HW_DMA_DCRn_WR(baseAddr,channel,0);
    HW_DMA_DSRn_WR(baseAddr,channel, 0);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : DMA_HAL_ConfigTransfer
 * Description   : Configure the basic paramenters for dma transfer.
 *
 *END**************************************************************************/
void DMA_HAL_ConfigTransfer(
    uint32_t baseAddr,uint32_t channel,dma_transfer_size_t size,dma_transfer_type_t type,
    uint32_t sourceAddr, uint32_t destAddr, uint32_t length)
{

    dma_channel_link_config_t config;

    config.channel1 = 0;
    config.channel2 = 0;
    config.linkType = kDmaChannelLinkDisable;

    /* Common configuration. */
    DMA_HAL_SetAutoAlignCmd(baseAddr, channel, false);
    DMA_HAL_SetCycleStealCmd(baseAddr, channel, false);
    DMA_HAL_SetAsyncDmaRequestCmd(baseAddr, channel, false);
    DMA_HAL_SetDisableRequestAfterDoneCmd(baseAddr, channel, true);
    DMA_HAL_SetChanLink(baseAddr, channel, &config);

    DMA_HAL_SetIntCmd(baseAddr, channel, true);
    DMA_HAL_SetSourceAddr(baseAddr, channel, sourceAddr);
    DMA_HAL_SetDestAddr(baseAddr, channel, destAddr);
    DMA_HAL_SetSourceModulo(baseAddr, channel, kDmaModuloDisable);
    DMA_HAL_SetDestModulo(baseAddr, channel, kDmaModuloDisable);
    DMA_HAL_SetSourceTransferSize(baseAddr, channel, size);
    DMA_HAL_SetDestTransferSize(baseAddr, channel, size);
    DMA_HAL_SetTransferCount(baseAddr, channel, length);

    switch (type)
    {
      case kDmaMemoryToPeripheral:
          DMA_HAL_SetSourceIncrementCmd(baseAddr, channel, true);
          DMA_HAL_SetDestIncrementCmd(baseAddr, channel, false);
          break;
      case kDmaPeripheralToMemory:
          DMA_HAL_SetSourceIncrementCmd(baseAddr, channel, false);
          DMA_HAL_SetDestIncrementCmd(baseAddr, channel, true);
          break;
      case kDmaMemoryToMemory:
          DMA_HAL_SetSourceIncrementCmd(baseAddr, channel, true);
          DMA_HAL_SetDestIncrementCmd(baseAddr, channel, true);
          break;
      default:
          break;
    }

}

/*FUNCTION**********************************************************************
 *
 * Function Name : DMA_HAL_SetChanLink
 * Description   : Configure the channel link feature.
 *
 *END**************************************************************************/
void DMA_HAL_SetChanLink(
        uint32_t baseAddr, uint8_t channel, dma_channel_link_config_t *mode)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);

    BW_DMA_DCRn_LINKCC(baseAddr, channel, mode->linkType);

    switch(mode->linkType)
    {
        case kDmaChannelLinkDisable:
            break;
        case kDmaChannelLinkChan1AndChan2:
            BW_DMA_DCRn_LCH1(baseAddr, channel, mode->channel1);
            BW_DMA_DCRn_LCH2(baseAddr, channel, mode->channel2);
            DMA_HAL_SetCycleStealCmd(baseAddr,channel,true);
            break;
        case kDmaChannelLinkChan1:
            BW_DMA_DCRn_LCH1(baseAddr, channel, mode->channel1);
            DMA_HAL_SetCycleStealCmd(baseAddr,channel,true);
            break;
        case kDmaChannelLinkChan1AfterBCR0:
            BW_DMA_DCRn_LCH1(baseAddr, channel, mode->channel1);
            break;
        default:
            break;
    }
}

/*******************************************************************************
 * EOF
 ******************************************************************************/

