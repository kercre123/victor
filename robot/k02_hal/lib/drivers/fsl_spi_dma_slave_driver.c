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

#include <string.h>
#include "fsl_spi_dma_slave_driver.h"
#include "fsl_spi_dma_shared_function.h"
#include "fsl_clock_manager.h"
#include "fsl_interrupt_manager.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*!
 * @brief Flags of SPI slave event.
 *
 * SPI event used to notify user that it finishes the task.
 */
typedef enum _spi_dma_event_flags {
    kSpiDmaTransferDone = 0x01,        /*!< Transferring done flag */
} spi_dma_event_flag_t;
/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Pointer to runtime state structure.*/
extern void * g_spiDmaStatePtr[HW_SPI_INSTANCE_COUNT];

static uint8_t s_byteToSend;  /* Word to send, if no send buffer, this variable is used
                                 as the word to send, which should be initialized to 0. Needs
                                 to be static and stored in data section as this variable
                                 address is the DMA source address if no source buffer. */

static uint8_t s_rxBuffIfNull; /* If no receive buffer provided, direct rx DMA channel to this
                                  destination */

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Configure SPI slave module and start the transfer using DMA
 *
 * This function updates SPI slave state structure, configures SPI slave module
 * using DMA driven and then start the transfer.
 *
 * @param instance The instance number of the SPI peripheral.
 */
static spi_status_t SPI_DRV_DmaSlaveStartTransfer(uint32_t instance);

/*!
 * @brief Stop the SPI slave transfer using DMA
 *
 * This function makes SPI slave transferring stop.
 *
 * @param instance The instance number of the SPI peripheral.
 */
static void SPI_DRV_DmaSlaveCompleteTransfer(uint32_t instance);

/*!
 * @brief The callback function for DMA complete interrupt.
 *
 * @param param Instance number of the SPI module.
 * @param chanStatus Current status of DMA channel.
 */
void SPI_DRV_DmaSlaveCallback(void *param, dma_channel_status_t chanStatus);

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaSlaveCallback
 * Description   : This function is called when the DMA generates an interrupt.
 * The DMA generates an interrupt when the channel is "done", meaning that the
 * expected number of bytes have been transferred.  When the interrupt occurs,
 * the DMA will jump to this callback as it was registered in the DMA register
 * callback service function.  The user will defined their own callback function
 * to take whatever action they deem necessary for handling the end of a transfer.
 * For example, the user may simply want their callback function to set a global
 * flag to indicate that the transfer is complete.  The user defined callback
 * is passed in through the "param" parameter.
 * The parameter chanStatus is currently not used.
 *
 *END**************************************************************************/
void SPI_DRV_DmaSlaveCallback(void *param, dma_channel_status_t chanStatus)
{
    uint32_t instance = (uint32_t)(param);

    /* instantiate local variable of type spi_master_state_t and point to global state */
    spi_dma_slave_state_t * spiDmaState = (spi_dma_slave_state_t *)g_spiDmaStatePtr[instance];

    uint32_t baseAddr = g_spiBaseAddr[instance];

    /* If hasExtraByte flag was set, need to enable the TX empty interrupt to get the last byte */
    if ((spiDmaState->hasExtraByte) && (spiDmaState->remainingReceiveByteCount))
    {
        SPI_HAL_SetTxDmaCmd(baseAddr, false);
        SPI_HAL_SetTransmitIntCmd(baseAddr, true);
    }
    else
    {
        /* Transfer completed, finish the transfer */
        SPI_DRV_DmaSlaveCompleteTransfer(instance);
    }

}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaSlaveInit
 * Description   : Initializes the SPI module for slave mode.
 * Saves the application callback info, turns on the clock to the module,
 * enables the device, and enables interrupts. Sets the SPI to a slave mode.
 *
 *END**************************************************************************/
spi_status_t SPI_DRV_DmaSlaveInit(uint32_t instance, spi_dma_slave_state_t * spiState,
                       const spi_dma_slave_user_config_t * slaveConfig)
{
    uint32_t baseAddr = g_spiBaseAddr[instance];
    assert(slaveConfig);
    assert(instance < HW_SPI_INSTANCE_COUNT);
    assert(spiState);

#if FSL_FEATURE_SPI_16BIT_TRANSFERS
    if (slaveConfig->bitCount > kSpi16BitMode)
    {
        /* bits/frame larger than hardware support */
        return kStatus_SPI_InvalidParameter;
    }
#endif

    /* Check if the slave already initialized */
    if (g_spiDmaStatePtr[instance])
    {
        return kStatus_SPI_AlreadyInitialized;
    }

    /* Clear the state for this instance. */
    memset(spiState, 0, sizeof(* spiState));

    spiState->hasExtraByte = false;

    /* Update dummy pattern value */
    spiState->dummyPattern = slaveConfig->dummyPattern;

    /* Enable clock for SPI */
    CLOCK_SYS_EnableSpiClock(instance);

    /* Reset the SPI module to its default settings including disabling SPI */
    SPI_HAL_Init(baseAddr);

    /* Initialize the event structure */
    OSA_EventCreate(&spiState->event, kEventAutoClear);

    /* Set SPI to slave mode */
    SPI_HAL_SetMasterSlave(baseAddr, kSpiSlave);

    /* Configure the slave clock polarity, phase and data direction */
    SPI_HAL_SetDataFormat(baseAddr, slaveConfig->polarity, slaveConfig->phase,
            slaveConfig->direction);

    /* Set the SPI pin mode to normal mode */
    SPI_HAL_SetPinMode(baseAddr, kSpiPinMode_Normal);

#if FSL_FEATURE_SPI_16BIT_TRANSFERS
    SPI_HAL_Set8or16BitMode(baseAddr, slaveConfig->bitCount);
#endif

#if FSL_FEATURE_SPI_FIFO_SIZE
    if (FSL_FEATURE_SPI_FIFO_SIZEn(instance) != 0)
    {
        /* If SPI module contains a FIFO, enable it and set watermarks to half full/empty */
        SPI_HAL_SetFifoMode(baseAddr, true, kSpiTxFifoOneHalfEmpty, kSpiRxFifoOneHalfFull);

        /* Set the interrupt clearing mechansim select for later use in driver to clear
         * status flags
         */
        SPI_HAL_SetIntClearCmd(baseAddr, true);
    }
#endif

    /*****************************************
     * Request DMA channel for RX and TX FIFO
     *****************************************/
    /* This channel transfers data from RX FIFO to receiveBuffer */
    if (instance == 0)
    {
        /* Request DMA channel for RX FIFO */
        if (kDmaInvalidChannel == DMA_DRV_RequestChannel(kDmaAnyChannel,
                                                         kDmaRequestMux0SPI0Rx,
                                                         &spiState->dmaReceive))
        {
            return kStatus_SPI_DMAChannelInvalid;
        }
        /* Request DMA channel for TX FIFO */
        if (kDmaInvalidChannel == DMA_DRV_RequestChannel(kDmaAnyChannel,
                                                         kDmaRequestMux0SPI0Tx,
                                                         &spiState->dmaTransmit))
        {
            return kStatus_SPI_DMAChannelInvalid;
        }
    }
#if (HW_SPI_INSTANCE_COUNT > 1)
    else if (instance == 1)
    {
        /* Request DMA channel for RX FIFO */
        if (kDmaInvalidChannel == DMA_DRV_RequestChannel(kDmaAnyChannel,
                                                         kDmaRequestMux0SPI1Rx,
                                                         &spiState->dmaReceive))
        {
            return kStatus_SPI_DMAChannelInvalid;
        }
        /* Request DMA channel for TX FIFO */
        if (kDmaInvalidChannel == DMA_DRV_RequestChannel(kDmaAnyChannel,
                                                         kDmaRequestMux0SPI1Tx,
                                                         &spiState->dmaTransmit))
        {
            return kStatus_SPI_DMAChannelInvalid;
        }
    }
    else
    {
        return kStatus_SPI_OutOfRange;
    }
#endif

    /* Save runtime structure pointers to irq handler can point to the correct state structure */
    g_spiDmaStatePtr[instance] = spiState;

    /* Enable SPI interrupt. The transmit interrupt should immediately cause an interrupt
     * which will fill in the transmit buffer and will be ready to send once the slave initiates
     * transmission.
     */
    INT_SYS_EnableIRQ(g_spiIrqId[instance]);

    /* SPI module enable */
    SPI_HAL_Enable(baseAddr);

    return kStatus_SPI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaSlaveDeinit
 * Description   : De-initializes the device.
 * Clears the control register and turns off the clock to the module.
 *
 *END**************************************************************************/
void SPI_DRV_DmaSlaveDeinit(uint32_t instance)
{
    spi_dma_slave_state_t * spiState = (spi_dma_slave_state_t *)g_spiDmaStatePtr[instance];
    uint32_t baseAddr = g_spiBaseAddr[instance];

    assert(instance < HW_SPI_INSTANCE_COUNT);

    /* Disable SPI interrupt */
    INT_SYS_DisableIRQ(g_spiIrqId[instance]);

    /* Reset the SPI module to its default settings including disabling SPI and its interrupts */
    SPI_HAL_Init(baseAddr);

#if FSL_FEATURE_SPI_16BIT_TRANSFERS
    if (FSL_FEATURE_SPI_FIFO_SIZEn(instance) != 0)
    {
        SPI_HAL_SetFifoIntCmd(baseAddr, kSpiRxFifoNearFullInt, false);
    }

    /* disable transmit interrupt */
    SPI_HAL_SetTransmitIntCmd(baseAddr, false);
#endif

    /* Free DMA channels */
    DMA_DRV_FreeChannel(&spiState->dmaReceive);
    DMA_DRV_FreeChannel(&spiState->dmaTransmit);

    /* Disable clock for SPI */
    CLOCK_SYS_DisableSpiClock(instance);

    /* Destroy the event */
    OSA_EventDestroy(&spiState->event);

    /* Clear state pointer */
    g_spiDmaStatePtr[instance] = NULL;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaSlaveTransfer
 * Description   : Transfer data with the master using DMA driven. Starts the
 * transfer with transmit buffer, receive buffer and transfer byte count passed.
 *
 *END**************************************************************************/
spi_status_t SPI_DRV_DmaSlaveTransfer(uint32_t instance,
                                   const uint8_t *sendBuffer,
                                   uint8_t *receiveBuffer,
                                   uint32_t transferByteCount)
{
    spi_dma_slave_state_t * spiState = (spi_dma_slave_state_t *)g_spiDmaStatePtr[instance];
    spi_status_t errorStatus = kStatus_SPI_Success;

    /* Validate the parameter */
    assert(instance < HW_SPI_INSTANCE_COUNT);

    if ((!sendBuffer) && (!receiveBuffer))
    {
        /* sendBuffer and receiveBuffer are not available, this is invalid */
        return kStatus_SPI_InvalidParameter;
    }

    if (!transferByteCount)
    {
        /* number of transfer bytes is 0 */
        return kStatus_SPI_InvalidParameter;
    }

    if (spiState->isTransferInProgress)
    {
        /* The another transfer is in progress */
        return kStatus_SPI_Busy;
    }

    /* fill in members of the run-time state struct */
    spiState->isSync = false;
    spiState->sendBuffer = sendBuffer;
    spiState->receiveBuffer = (uint8_t *)receiveBuffer;
    spiState->remainingSendByteCount = transferByteCount;
    spiState->remainingReceiveByteCount = transferByteCount;

    /* Setup hardware to start the transfer */
    errorStatus = SPI_DRV_DmaSlaveStartTransfer(instance);
    if (errorStatus != kStatus_SPI_Success)
    {
        return errorStatus;
    }

    return kStatus_SPI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaSlaveTransferBlocking
 * Description   : Transfer data with the master, using blocking call and DMA
 * driven.
 *
 *END**************************************************************************/
spi_status_t SPI_DRV_DmaSlaveTransferBlocking(uint32_t instance,
                                           const uint8_t *sendBuffer,
                                           uint8_t *receiveBuffer,
                                           uint32_t transferByteCount,
                                           uint32_t timeout)
{
    spi_dma_slave_state_t * spiState = (spi_dma_slave_state_t *)g_spiDmaStatePtr[instance];
    spi_status_t errorStatus = kStatus_SPI_Success;
    event_flags_t setFlags = 0;

    /* fill in members of the run-time state struct */
    spiState->isSync = true;
    spiState->sendBuffer = (const uint8_t *)sendBuffer;
    spiState->receiveBuffer = (uint8_t *)receiveBuffer;
    spiState->remainingSendByteCount = transferByteCount;
    spiState->remainingReceiveByteCount = transferByteCount;

    /* Clear the event flags */
    OSA_EventClear(&spiState->event, kSpiDmaTransferDone);

    errorStatus = SPI_DRV_DmaSlaveStartTransfer(instance);
    if (errorStatus != kStatus_SPI_Success)
    {
        return errorStatus;
    }

    /* As this is a synchronous transfer, wait until the transfer is complete. */
    osa_status_t syncStatus;

    do
    {
        syncStatus = OSA_EventWait(&spiState->event, kSpiDmaTransferDone, true, timeout, &setFlags);
    }while(syncStatus == kStatus_OSA_Idle);

    if (syncStatus != kStatus_OSA_Success)
    {
        /* Abort the transfer so it doesn't continue unexpectedly. */
        SPI_DRV_DmaSlaveAbortTransfer(instance);

        errorStatus = kStatus_SPI_Timeout;
    }

    return errorStatus;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaSlaveStartTransfer
 * Description   : Starts the transfer with information passed.
 *
 *END**************************************************************************/
static spi_status_t SPI_DRV_DmaSlaveStartTransfer(uint32_t instance)
{
    spi_dma_slave_state_t * spiState = (spi_dma_slave_state_t *)g_spiDmaStatePtr[instance];

    /* For temporarily storing DMA register channel */
    uint8_t txChannel, rxChannel;
    void * param;
    uint32_t spiBaseAddr = g_spiBaseAddr[instance];
    uint32_t dmaBaseAddr;
    uint32_t dmamuxBaseAddr;
    uint32_t transferSizeInBytes;  /* DMA transfer size in bytes */

    /* Initialize s_byteToSend */
    s_byteToSend = spiState->dummyPattern;

    /* If the transfer count is zero, then return immediately. */
    if (spiState->remainingSendByteCount == 0)
    {
        /* Signal the synchronous completion object if the transfer wasn't async.
         * Otherwise, when we return the the sync function we'll get stuck in the sync wait loop.
         */
        if (spiState->isSync)
        {
            /* Signal the synchronous completion object */
            OSA_EventSet(&spiState->event, kSpiDmaTransferDone);
        }
        return kStatus_SPI_Success;
    }

    /* In order to flush any remaining data in the shift register, disable then enable the SPI */
    SPI_HAL_Disable(spiBaseAddr);
    SPI_HAL_Enable(spiBaseAddr);

    /* First, set the DMA transfer size in bytes */
#if FSL_FEATURE_SPI_16BIT_TRANSFERS
    if (SPI_HAL_Get8or16BitMode(spiBaseAddr) == kSpi16BitMode)
    {
        transferSizeInBytes = 2;
        /* If bits/frame > 8, meaning 2 bytes, then the transfer byte count must not be an odd
         * count. If so, drop the last odd byte. This odd byte will be transferred in when dma
         * completed
         */
        if (spiState->remainingSendByteCount % 2 != 0)
        {
            spiState->remainingSendByteCount ++;
            spiState->remainingReceiveByteCount --;
            spiState->hasExtraByte = true;
        }
        else
        {
            spiState->hasExtraByte = false;
        }
    }
    else
    {
        transferSizeInBytes = 1;
    }
#else
    transferSizeInBytes = 1;
#endif

    param = (void *)(instance);     /* For DMA callback, set "param" as the SPI instance number */
    rxChannel = spiState->dmaReceive.channel;
    txChannel = spiState->dmaTransmit.channel;
    /* Only need to set the DMA reg base addr once since it should be the same for all code */
    dmaBaseAddr = g_dmaRegBaseAddr[rxChannel/FSL_FEATURE_DMA_DMAMUX_CHANNELS];
    dmamuxBaseAddr = g_dmamuxRegBaseAddr[txChannel/FSL_FEATURE_DMAMUX_MODULE_CHANNEL];

    /* Save information about the transfer for use by the ISR. */
    spiState->isTransferInProgress = true;

    /* The DONE needs to be cleared before programming the channel's TCDs for the next transfer. */
    DMA_HAL_ClearStatus(dmaBaseAddr, rxChannel);
    DMA_HAL_ClearStatus(dmaBaseAddr, txChannel);

    /* Disable and enable the TX DMA channel at the DMA mux. Doing so will prevent an
     * inadvertent DMA transfer when the TX DMA channel ERQ bit is set after having been
     * cleared from a previous DMA transfer (clearing of the ERQ bit is automatically performed
     * at the end of a transfer when D_REQ is set).
     */
    DMAMUX_HAL_SetChannelCmd(dmamuxBaseAddr, txChannel, false);
    DMAMUX_HAL_SetChannelCmd(dmamuxBaseAddr, txChannel, true);
    DMAMUX_HAL_SetChannelCmd(dmamuxBaseAddr, rxChannel, false);
    DMAMUX_HAL_SetChannelCmd(dmamuxBaseAddr, rxChannel, true);

    /************************************************************************************
     * Set up the RX DMA channel Transfer Control Descriptor (TCD)
     * Note, if there is no receive buffer (if user passes in NULL), then bypass RX DMA
     * set up.
     ************************************************************************************/
    /* If no receive buffer then disable incrementing the destination and set the destination
     * to a temporary location
     */
    if ((spiState->remainingReceiveByteCount > 0) || (spiState->hasExtraByte))
    {
        uint32_t receiveSize = spiState->remainingReceiveByteCount;
        if ((!spiState->receiveBuffer) || (!spiState->remainingReceiveByteCount))
        {
            if (!spiState->remainingReceiveByteCount)
            {
                /* If receive count is 0, always receive 1 frame (2 bytes) */
                receiveSize = 2;
            }
            /* Set up this channel's control which includes enabling the DMA interrupt */
            DMA_DRV_ConfigTransfer(&spiState->dmaReceive,
                                   kDmaPeripheralToMemory,
                                   transferSizeInBytes,
                                   SPI_HAL_GetDataRegAddr(spiBaseAddr), /* src is data register */
                                   (uint32_t)(&s_rxBuffIfNull), /* dest is temporary location */
                                   (uint32_t)(receiveSize));

            /* Do not increment the destination address */
            DMA_HAL_SetDestIncrementCmd(dmaBaseAddr, rxChannel, false);
        }
        else
        {
            /* Set up this channel's control which includes enabling the DMA interrupt */
            DMA_DRV_ConfigTransfer(&spiState->dmaReceive,
                                   kDmaPeripheralToMemory,
                                   transferSizeInBytes,
                                   SPI_HAL_GetDataRegAddr(spiBaseAddr), /* src is data register */
                                   (uint32_t)(spiState->receiveBuffer), /* dest is rx buffer */
                                   (uint32_t)(receiveSize));
        }

        /* For SPI16 modules, if the bits/frame is 16, then we need to adjust the destination
         * size to still be 8-bits since the DMA_DRV_ConfigTransfer sets this to 16-bits, but
         * we always provide a 8-bit buffer.
         */
        if (transferSizeInBytes == 2)
        {
            DMA_HAL_SetDestTransferSize(dmaBaseAddr, rxChannel, kDmaTransfersize8bits);
        }

        /* Enable the cycle steal mode which forces a single read/write transfer per request */
        DMA_HAL_SetCycleStealCmd(dmaBaseAddr, rxChannel, true);

        /* Enable the DMA peripheral request */
        DMA_DRV_StartChannel(&spiState->dmaReceive);

        /* Register callback for DMA interrupt */
        DMA_DRV_RegisterCallback(&spiState->dmaReceive, SPI_DRV_DmaSlaveCallback, param);
    }

    /************************************************************************************
     * Set up the TX DMA channel Transfer Control Descriptor (TCD)
     * Note, if there is no source buffer (if user passes in NULL), then send zeros
     ************************************************************************************/
    /* Per the reference manual, before enabling the SPI transmit DMA request, we first need
     * to read the status register and then write to the SPI data register.  Afterwards, we need
     * to decrement the sendByteCount and perform other driver maintenance functions.
     */
    /* Read the SPI Status register */
    SPI_HAL_IsTxBuffEmptyPending(spiBaseAddr);

    /* Start the transfer by writing the first byte/word to the SPI data register.
     * If a send buffer was provided, the byte/word comes from there. Otherwise we just send zeros.
     */
#if FSL_FEATURE_SPI_16BIT_TRANSFERS
    if (transferSizeInBytes == 2) /* 16-bit transfers for SPI16 module */
    {
        if (spiState->sendBuffer)
        {
            s_byteToSend = *(spiState->sendBuffer);
            SPI_HAL_WriteDataLow(spiBaseAddr, s_byteToSend);
            ++spiState->sendBuffer;

            s_byteToSend = *(spiState->sendBuffer);
            SPI_HAL_WriteDataHigh(spiBaseAddr, s_byteToSend);
            ++spiState->sendBuffer;
        }
        else  /* Else, if no send buffer, write zeros */
        {
            SPI_HAL_WriteDataLow(spiBaseAddr, s_byteToSend);
            SPI_HAL_WriteDataHigh(spiBaseAddr, s_byteToSend);
        }
        spiState->remainingSendByteCount -= 2;  /* Decrement the send byte count by 2 */
    }
    else /* 8-bit transfers for SPI16 module */
    {
        if (spiState->sendBuffer)
        {
            s_byteToSend = *(spiState->sendBuffer);
            ++spiState->sendBuffer;
        }
        SPI_HAL_WriteDataLow(spiBaseAddr, s_byteToSend);  /* If no send buffer, s_byteToSend=0 */
        --spiState->remainingSendByteCount; /* Decrement the send byte count for use in DMA setup */
    }
#else
    /* For SPI modules that do not support 16-bit transfers */
    if (spiState->sendBuffer)
    {
        s_byteToSend = *(spiState->sendBuffer);
        ++spiState->sendBuffer;
    }
    SPI_HAL_WriteData(spiBaseAddr, s_byteToSend); /* If no send buffer, s_byteToSend=0 */
    --spiState->remainingSendByteCount; /* Decrement the send byte count for use in DMA setup */
#endif

    /* If there are no more bytes to send then return without setting up the TX DMA channel.
     * Else, set up the TX DMA channel and enable the TX DMA request.
     */
    if (!spiState->remainingSendByteCount) /* No more bytes to send */
    {
        if ((spiState->remainingReceiveByteCount) || (spiState->hasExtraByte))
        {
            /* Enable the RX DMA channel request now */
            SPI_HAL_SetRxDmaCmd(spiBaseAddr, true);
            return kStatus_SPI_Success;
        }
        else    /* If RX DMA chan not setup then enable the interrupt to get the received byte */
        {
            SPI_HAL_SetTransmitIntCmd(spiBaseAddr, true);
            return kStatus_SPI_Success;
        }
    }
    else  /* Since there are more bytes to send, set up the TX DMA channel */
    {
        /* If there is a send buffer, data comes from there, else send 0 */
        if (spiState->sendBuffer)
        {
            /* Set up this channel's control which includes enabling the DMA interrupt */
            DMA_DRV_ConfigTransfer(&spiState->dmaTransmit, kDmaMemoryToPeripheral,
                                   transferSizeInBytes,
                                   (uint32_t)(spiState->sendBuffer),
                                   SPI_HAL_GetDataRegAddr(spiBaseAddr),
                                   (uint32_t)(spiState->remainingSendByteCount));
        }
        else /* Configure TX DMA channel to send zeros */
        {
            /* Set up this channel's control which includes enabling the DMA interrupt */
            DMA_DRV_ConfigTransfer(&spiState->dmaTransmit, kDmaMemoryToPeripheral,
                                   transferSizeInBytes,
                                   (uint32_t)(&s_byteToSend),
                                   SPI_HAL_GetDataRegAddr(spiBaseAddr),
                                   (uint32_t)(spiState->remainingSendByteCount));

            /* Now clear SINC since we are only sending zeroes, don't increment source */
            DMA_HAL_SetSourceIncrementCmd(dmaBaseAddr, txChannel, false);
        }

        /* For SPI16 modules, if the bits/frame is 16, then we need to adjust the source
         * size to still be 8-bits since the DMA_DRV_ConfigTransfer sets this to 16-bits, but
         * we always provide a 8-bit buffer.
         */
        if (transferSizeInBytes == 2)
        {
            DMA_HAL_SetSourceTransferSize(dmaBaseAddr, txChannel, kDmaTransfersize8bits);
        }

        /* Enable the cycle steal mode which forces a single read/write transfer per request */
        DMA_HAL_SetCycleStealCmd(dmaBaseAddr, txChannel, true);

        /* Now, disable the TX chan interrupt since we'll use the RX chan interrupt */
        DMA_HAL_SetIntCmd(dmaBaseAddr, txChannel, false);

        /* Enable the DMA peripheral request */
        DMA_DRV_StartChannel(&spiState->dmaTransmit);

        /* Enable the SPI TX DMA Request */
        SPI_HAL_SetTxDmaCmd(spiBaseAddr, true);

        /* Enable the SPI RX DMA request also. */
        SPI_HAL_SetRxDmaCmd(spiBaseAddr, true);
    }

    return kStatus_SPI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaSlaveCompleteTransfer
 * Description   : Finish up a transfer.
 * Cleans up after a transfer is complete. Interrupts are disabled, and the SPI module
 * is disabled. This is not a public API as it is called from other driver functions.
 *
 *END**************************************************************************/
static void SPI_DRV_DmaSlaveCompleteTransfer(uint32_t instance)
{
    spi_dma_slave_state_t * spiState = (spi_dma_slave_state_t *)g_spiDmaStatePtr[instance];

    uint32_t spiBaseAddr = g_spiBaseAddr[instance];

    /* Disable DMA requests and interrupts. */
    SPI_HAL_SetRxDmaCmd(spiBaseAddr, false);
    SPI_HAL_SetTxDmaCmd(spiBaseAddr, false);
    SPI_HAL_SetTransmitIntCmd(spiBaseAddr, false);

    /* Stop DMA channels */
    DMA_DRV_StopChannel(&spiState->dmaTransmit);
    DMA_DRV_StopChannel(&spiState->dmaReceive);

    /* Disable interrupts */
    SPI_HAL_SetReceiveAndFaultIntCmd(spiBaseAddr, false);

#if FSL_FEATURE_SPI_16BIT_TRANSFERS
    if (FSL_FEATURE_SPI_FIFO_SIZEn(instance) != 0)
    {
        /* Now disable the SPI FIFO interrupts */
        SPI_HAL_SetFifoIntCmd(spiBaseAddr, kSpiTxFifoNearEmptyInt, false);
        SPI_HAL_SetFifoIntCmd(spiBaseAddr, kSpiRxFifoNearFullInt, false);
    }

    /* Receive extra byte if remaining receive byte is 0 */
    if ((spiState->hasExtraByte) && (!spiState->remainingReceiveByteCount) &&
        (spiState->receiveBuffer))
    {
        spiState->receiveBuffer[spiState->remainingReceiveByteCount] =
                                                                 SPI_HAL_ReadDataLow(spiBaseAddr);
    }
#endif

    if (spiState->isSync)
    {
        /* Signal the synchronous completion object */
        OSA_EventSet(&spiState->event, kSpiDmaTransferDone);
    }

    /* The transfer is complete, update the state structure */
    spiState->isTransferInProgress = false;
    spiState->status = kStatus_SPI_Success;
    spiState->errorCount = 0;
    spiState->sendBuffer = NULL;
    spiState->receiveBuffer = NULL;
    spiState->remainingSendByteCount = 0;
    spiState->remainingReceiveByteCount = 0;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaSlaveAbortTransfer
 * Description   : Stop the transfer if it is in progress
 *
 *END**************************************************************************/
spi_status_t SPI_DRV_DmaSlaveAbortTransfer(uint32_t instance)
{
    spi_dma_slave_state_t * spiState = (spi_dma_slave_state_t *)g_spiDmaStatePtr[instance];

    /* Check instance is valid or not */
    assert(instance < HW_SPI_INSTANCE_COUNT);

    /* Check driver is initialized */
    if (!spiState)
    {
        return kStatus_SPI_NonInit;
    }

    /* Check transfer is in progress */
    if (!spiState->isTransferInProgress)
    {
        return kStatus_SPI_NoTransferInProgress;
    }

    /* Stop transfer */
    SPI_DRV_DmaSlaveCompleteTransfer(instance);

    return kStatus_SPI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaSlaveGetTransferStatus
 * Description   : Returns whether the previous transfer finished.
 * When performing an a-sync transfer, the user can call this function to ascertain the state of the
 * current transfer: in progress (or busy) or complete (success). In addition, if the transfer
 * is still in progress, the user can get the number of words that have been
 * transferred up to now.
 *
 *END**************************************************************************/
spi_status_t SPI_DRV_DmaSlaveGetTransferStatus(uint32_t instance,
                                            uint32_t * framesTransferred)
{
    spi_dma_slave_state_t * spiState = (spi_dma_slave_state_t *)g_spiDmaStatePtr[instance];

    /* Fill in the bytes transferred. */
    if (framesTransferred)
    {
        *framesTransferred = spiState->remainingSendByteCount -
                            DMA_DRV_GetUnfinishedBytes(&spiState->dmaTransmit);
    }

    return (spiState->isTransferInProgress ? kStatus_SPI_Busy : kStatus_SPI_Success);
}


/*!
 * @brief Interrupt handler for SPI master mode.
 * This handler is used when the hasExtraByte flag is set to retrieve the received last byte.
 * This is not a public API as it is called whenever an interrupt occurs.
 */
void SPI_DRV_DmaSlaveIRQHandler(uint32_t instance)
{
    /* instantiate local variable of type spi_master_state_t and point to global state */
    spi_dma_slave_state_t * spiDmaState = (spi_dma_slave_state_t *)g_spiDmaStatePtr[instance];

    uint32_t baseAddr = g_spiBaseAddr[instance];

#if FSL_FEATURE_SPI_16BIT_TRANSFERS
        /* If the SPI module contains a FIFO, check the FIFO empty flag */
        if ((FSL_FEATURE_SPI_FIFO_SIZEn(instance) != 0) && (SPI_HAL_GetFifoCmd(baseAddr)))
        {
            if (SPI_HAL_GetFifoStatusFlag(baseAddr, kSpiRxFifoEmpty) == 0)
            {
                /* If there is a receive buffer, copy the final byte from the SPI data register
                 *  to the receive buffer
                 */
                if (spiDmaState->receiveBuffer)
                {
                    spiDmaState->receiveBuffer[spiDmaState->remainingReceiveByteCount] =
                                                                  SPI_HAL_ReadDataLow(baseAddr);
                }
                /* Else, read out the data register and throw away the bytes read */
                else
                {
                    /* Read and throw away the lower data buffer to clear it out */
                    s_rxBuffIfNull = SPI_HAL_ReadDataLow(baseAddr);
                }
                /* Read and throw away the upper data buffer to clear it out */
                s_rxBuffIfNull = SPI_HAL_ReadDataHigh(baseAddr);

                SPI_DRV_DmaSlaveCompleteTransfer(instance);
            }
        }
        else /* Check the read pending flag */
        {
            if (SPI_HAL_IsReadBuffFullPending(baseAddr) == 1)
            {
                /* If there is a receive buffer, copy the final byte from the SPI data register
                 *  to the receive buffer
                 */
                if (spiDmaState->receiveBuffer)
                {
                    spiDmaState->receiveBuffer[spiDmaState->remainingReceiveByteCount] =
                                                                  SPI_HAL_ReadDataLow(baseAddr);
                }
                /* Else, read out the data register and throw away the bytes read */
                else
                {
                    /* Read and throw away the lower data buffer to clear it out */
                    s_rxBuffIfNull = SPI_HAL_ReadDataLow(baseAddr);
                }
                /* Read and throw away the upper data buffer to clear it out */
                s_rxBuffIfNull = SPI_HAL_ReadDataHigh(baseAddr);
                SPI_DRV_DmaSlaveCompleteTransfer(instance);
            }
        }
#endif
}
/*******************************************************************************
 * EOF
 ******************************************************************************/

