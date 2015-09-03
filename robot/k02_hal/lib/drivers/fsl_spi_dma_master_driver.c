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
#include "fsl_spi_dma_master_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_interrupt_manager.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
uint32_t g_interruptCnt = 0;
/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Pointer to runtime state structure.*/
extern void * g_spiDmaStatePtr[HW_SPI_INSTANCE_COUNT];

static uint8_t s_byteToSend;  /* Word to send, if no send buffer, this variable is used
                                 as the word to send, which should be initialized to 0. Needs
                                 to be static and stored in data section as this variable
                                 address is the DMA source address if no source buffer.  */

static uint8_t s_rxBuffIfNull; /* If no receive buffer provided, direct rx DMA channel to this
                                  destination */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
spi_status_t SPI_DRV_DmaMasterStartTransfer(uint32_t instance,
                                            const spi_dma_master_user_config_t * restrict device);

static void SPI_DRV_DmaMasterCompleteTransfer(uint32_t instance);

void SPI_DRV_DmaMasterCallback(void *param, dma_channel_status_t chanStatus);

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaMasterCallback
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
void SPI_DRV_DmaMasterCallback(void *param, dma_channel_status_t chanStatus)
{
    uint32_t instance = (uint32_t)(param);

    /* instantiate local variable of type spi_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];

    uint32_t baseAddr = g_spiBaseAddr[instance];

    /* If the extraByte flag was set, need to enable the TX empty interrupt to get the last byte */
    if (spiDmaState->extraByte)
    {
        SPI_HAL_SetTxDmaCmd(baseAddr, false);
        SPI_HAL_SetTransmitIntCmd(baseAddr, true);
    }
    else
    {
        SPI_DRV_DmaMasterCompleteTransfer(instance);
    }
}


/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaMasterInit
 * Description   : Initializes a SPI instance for master mode operation to work with DMA.
 * This function uses a dma driven method for transferring data.
 * this function initializes the run-time state structure to track the ongoing
 * transfers, ungates the clock to the SPI module, resets the SPI module, initializes the module
 * to user defined settings and default settings, configures the IRQ state structure, enables
 * the module-level interrupt to the core, and enables the SPI module.
 *
 * This initialization function also configures the DMA module by requesting channels for DMA
 * operation.
 *
 *END**************************************************************************/
void SPI_DRV_DmaMasterInit(uint32_t instance, spi_dma_master_state_t * spiDmaState)
{
    uint32_t baseAddr = g_spiBaseAddr[instance];

    /* Clear the state for this instance.*/
    memset(spiDmaState, 0, sizeof(* spiDmaState));

    /* Enable clock for SPI*/
    CLOCK_SYS_EnableSpiClock(instance);

    /* configure the run-time state struct with the source clock value */
    spiDmaState->spiSourceClock = CLOCK_SYS_GetSpiFreq(instance);

    /* Reset the SPI module to it's default state, which includes SPI disabled */
    SPI_HAL_Init(baseAddr);

    /* Init the interrupt sync object.*/
    OSA_SemaCreate(&spiDmaState->irqSync, 0);

    /* Set SPI to master mode */
    SPI_HAL_SetMasterSlave(baseAddr, kSpiMaster);

    /* Set slave select to automatic output mode */
    SPI_HAL_SetSlaveSelectOutputMode(baseAddr, kSpiSlaveSelect_AutomaticOutput);

    /* Set the SPI pin mode to normal mode */
    SPI_HAL_SetPinMode(baseAddr, kSpiPinMode_Normal);

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
    /* Save runtime structure pointers to irq handler can point to the correct state structure*/
    g_spiDmaStatePtr[instance] = spiDmaState;

    /*****************************************
     * Request DMA channel for RX and TX FIFO
     *****************************************/
    /* This channel transfers data from RX FIFO to receiveBuffer */
    if (instance == 0)
    {
        /* Request DMA channel for RX FIFO */
        DMA_DRV_RequestChannel(kDmaAnyChannel, kDmaRequestMux0SPI0Rx, &spiDmaState->dmaReceive);
        /* Request DMA channel for TX FIFO */
        DMA_DRV_RequestChannel(kDmaAnyChannel, kDmaRequestMux0SPI0Tx, &spiDmaState->dmaTransmit);
    }
#if (HW_SPI_INSTANCE_COUNT > 1)
    else
    {
        /* Request DMA channel for RX FIFO */
        DMA_DRV_RequestChannel(kDmaAnyChannel, kDmaRequestMux0SPI1Rx, &spiDmaState->dmaReceive);
        /* Request DMA channel for TX FIFO */
        DMA_DRV_RequestChannel(kDmaAnyChannel, kDmaRequestMux0SPI1Tx, &spiDmaState->dmaTransmit);
    }
#endif

    /* Enable SPI interrupt.*/
    INT_SYS_EnableIRQ(g_spiIrqId[instance]);

    /* SPI system Enable */
    SPI_HAL_Enable(baseAddr);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaMasterDeinit
 * Description   : Shuts down a SPI instance with DMA support.
 *
 * This function resets the SPI peripheral, gates its clock, disables any used interrupts to
 * the core, and releases any used DMA channels.
 *
 *END**************************************************************************/
void SPI_DRV_DmaMasterDeinit(uint32_t instance)
{
    /* instantiate local variable of type spi_dma_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];
    uint32_t baseAddr = g_spiBaseAddr[instance];

    /* Restore the module to defaults which includes disabling the SPI then power it down.*/
    SPI_HAL_Init(baseAddr);

    /* destroy the interrupt sync object.*/
    OSA_SemaDestroy(&spiDmaState->irqSync);

    /* Disable SPI interrupt.*/
    INT_SYS_DisableIRQ(g_spiIrqId[instance]);

    /* Gate the clock for SPI.*/
    CLOCK_SYS_DisableSpiClock(instance);

    /* Free DMA channels */
    DMA_DRV_FreeChannel(&spiDmaState->dmaReceive);
    DMA_DRV_FreeChannel(&spiDmaState->dmaTransmit);

    /* Clear state pointer. */
    g_spiDmaStatePtr[instance] = NULL;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaMasterConfigureBus
 * Description   : Configures the SPI port to access a device on the bus with DMA support.
 *
 * The term "device" is used to indicate the SPI device for which the SPI master is communicating.
 * The user has two options to configure the device parameters: either pass in the
 * pointer to the device configuration structure to the desired transfer function or pass it in to
 * the SPI_DRV_DmaMasterConfigureBus function.  The user can pass in a device structure to the
 * transfer function which contains the parameters for the bus (the transfer function then calls
 * this function). However, the user has the option to call this function directly especially
 * to get the calculated baud rate, at which point they may pass in NULL for the device
 * structure in the transfer function (assuming they have called this configure bus function
 * first).
 *
 *END**************************************************************************/
void SPI_DRV_DmaMasterConfigureBus(uint32_t instance,
                                   const spi_dma_master_user_config_t * device,
                                   uint32_t * calculatedBaudRate)
{
    assert(device);

    /* instantiate local variable of type spi_dma_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];

    uint32_t baseAddr = g_spiBaseAddr[instance];

    /* Configure the bus to access the provided device.*/
    *calculatedBaudRate = SPI_HAL_SetBaud(baseAddr, device->bitsPerSec,
                                          spiDmaState->spiSourceClock);
    SPI_HAL_SetDataFormat(baseAddr, device->polarity, device->phase, device->direction);

#if FSL_FEATURE_SPI_16BIT_TRANSFERS
    SPI_HAL_Set8or16BitMode(baseAddr, device->bitCount);
#endif

}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaMasterTransferBlocking
 * Description   : Performs a blocking SPI master mode transfer with DMA support.
 *
 * This function simultaneously sends and receives data on the SPI bus, as SPI is naturally
 * a full-duplex bus. The function does return until the transfer is complete.
 *
 *END**************************************************************************/
spi_status_t SPI_DRV_DmaMasterTransferBlocking(uint32_t instance,
                                               const spi_dma_master_user_config_t * restrict device,
                                               const uint8_t * restrict sendBuffer,
                                               uint8_t * restrict receiveBuffer,
                                               size_t transferByteCount,
                                               uint32_t timeout)
{
    /* instantiate local variable of type spi_dma_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];
    spi_status_t errorStatus = kStatus_SPI_Success;
    uint32_t spiBaseAddr = g_spiBaseAddr[instance];

    /* fill in members of the run-time state struct*/
    spiDmaState->isTransferBlocking = true; /* Indicates this is a blocking transfer */
    spiDmaState->sendBuffer = (const uint8_t *)sendBuffer;
    spiDmaState->receiveBuffer = (uint8_t *)receiveBuffer;
    spiDmaState->remainingSendByteCount = transferByteCount;
    spiDmaState->remainingReceiveByteCount = transferByteCount;

    /* start the transfer process*/
    errorStatus = SPI_DRV_DmaMasterStartTransfer(instance, device);
    if (errorStatus != kStatus_SPI_Success)
    {
        return errorStatus;
    }

    /* As this is a synchronous transfer, wait until the transfer is complete.*/
    osa_status_t syncStatus;

    do
    {
        syncStatus = OSA_SemaWait(&spiDmaState->irqSync, timeout);
    }while(syncStatus == kStatus_OSA_Idle);

    /* If a timeout occurs, stop the transfer by setting the isTransferInProgress to false and
     * disabling DMA requests and interrupts, then return the timeout error status.
     */
    if (syncStatus != kStatus_OSA_Success)
    {
        /* The transfer is complete.*/
        spiDmaState->isTransferInProgress = false;

        /* Disable DMA requests and interrupts. */
        SPI_HAL_SetRxDmaCmd(spiBaseAddr, false);
        SPI_HAL_SetTxDmaCmd(spiBaseAddr, false);
        SPI_HAL_SetTransmitIntCmd(spiBaseAddr, false);

        errorStatus = kStatus_SPI_Timeout;
    }

    return errorStatus;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaMasterTransfer
 * Description   : Performs a non-blocking SPI master mode transfer with DMA support.
 *
 * This function returns immediately. It is the user's responsibility to check back to
 * ascertain if the transfer is complete (using the SPI_DRV_DmaMasterGetTransferStatus function).
 * This function simultaneously sends and receives data on the SPI bus, as SPI is naturally
 * a full-duplex bus. The function does return until the transfer is complete.
 *
 *END**************************************************************************/
spi_status_t SPI_DRV_DmaMasterTransfer(uint32_t instance,
                                       const spi_dma_master_user_config_t * restrict device,
                                       const uint8_t * restrict sendBuffer,
                                       uint8_t * restrict receiveBuffer,
                                       size_t transferByteCount)
{
    /* instantiate local variable of type spi_dma_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];
    spi_status_t errorStatus = kStatus_SPI_Success;

    /* fill in members of the run-time state struct*/
    spiDmaState->isTransferBlocking = false; /* Indicates this is a non-blocking transfer */
    spiDmaState->sendBuffer = sendBuffer;
    spiDmaState->receiveBuffer = (uint8_t *)receiveBuffer;
    spiDmaState->remainingSendByteCount = transferByteCount;
    spiDmaState->remainingReceiveByteCount = transferByteCount;

    errorStatus = SPI_DRV_DmaMasterStartTransfer(instance, device);
    if (errorStatus != kStatus_SPI_Success)
    {
        return errorStatus;
    }

    /* Else, return immediately as this is an async transfer */
    return kStatus_SPI_Success;
}

/*!
 * @brief Initiate (start) a transfer using DMA. This is not a public API as it is called from
 *  other driver functions
 */
spi_status_t SPI_DRV_DmaMasterStartTransfer(uint32_t instance,
                                            const spi_dma_master_user_config_t * restrict device)
{
    /* instantiate local variable of type spi_dma_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];

    /* For temporarily storing DMA register channel */
    uint8_t txChannel, rxChannel;
    void * param;
    uint32_t calculatedBaudRate;
    uint32_t spiBaseAddr = g_spiBaseAddr[instance];
    uint32_t dmaBaseAddr;
    uint32_t dmamuxBaseAddr;
    uint32_t transferSizeInBytes;  /* DMA transfer size in bytes */

    /* Initialize s_byteToSend */
    s_byteToSend = 0;

    /* If the transfer count is zero, then return immediately.*/
    if (spiDmaState->remainingSendByteCount == 0)
    {
        /* Signal the synchronous completion object if the transfer wasn't async.
         * Otherwise, when we return the the sync function we'll get stuck in the sync wait loop.
         */
        if (spiDmaState->isTransferBlocking)
        {
            OSA_SemaPost(&spiDmaState->irqSync);
        }

        return kStatus_SPI_Success;
    }

    /* Configure bus for this device. If NULL is passed, we assume the caller has
     * preconfigured the bus using SPI_DRV_DmaMasterConfigureBus().
     * Do nothing for calculatedBaudRate. If the user wants to know the calculatedBaudRate
     * then they can call this function separately.
     */
    if (device)
    {
        SPI_DRV_DmaMasterConfigureBus(instance, device, &calculatedBaudRate);
    }

    /* In order to flush any remaining data in the shift register, disable then enable the SPI */
    SPI_HAL_Disable(spiBaseAddr);
    SPI_HAL_Enable(spiBaseAddr);

#if FSL_FEATURE_SPI_16BIT_TRANSFERS
    /* Check the transfer byte count. If bits/frame > 8, meaning 2 bytes, and if
     * the transfer byte count is an odd count we'll have to round down the RX transfer byte count
     * to the next lowest even number by one and assert a flag to indicate in the interrupt handler
     * that we take care of sending and receiving this last byte.  We'll round up TX byte count.
     */
    if (SPI_HAL_Get8or16BitMode(spiBaseAddr) == kSpi16BitMode) /* Applies to 16-bit transfers */
    {
        /* Odd byte count for 16-bit transfers, set the extraByte flag */
        if (spiDmaState->remainingSendByteCount & 1UL) /* If odd byte count */
        {
            transferSizeInBytes = 2; /* Set transfer size to two bytes for the DMA operation */
            spiDmaState->extraByte = true; /* Set the extraByte flag */

            /* Round up TX byte count so when DMA completes, all data would've been sent */
            spiDmaState->remainingSendByteCount += 1U;

            /* Round down RX byte count which means at the end of the RX DMA transfer, we'll need
             * to set up an interrupt to get the last byte.
             */
            spiDmaState->remainingReceiveByteCount &= ~1U;

            /* Store the transfer byte count to the run-time state struct
             * for later use in the interrupt handler.
             */
            spiDmaState->transferByteCnt = spiDmaState->remainingSendByteCount;
        }
        /* Even byte count for 16-bit transfers, clear the extraByte flag */
        else
        {
            transferSizeInBytes = 2; /* Set transfer size to two bytes for the DMA operation */
            spiDmaState->extraByte = false; /* Clear the extraByte flag */
        }
    }
    else /* For 8-bit transfers */
    {
        transferSizeInBytes = 1;
        spiDmaState->extraByte = false;
    }
#else
    transferSizeInBytes = 1;
#endif

    param = (void *)(instance); /* For DMA callback, set "param" as the SPI instance number */
    rxChannel = spiDmaState->dmaReceive.channel;
    txChannel = spiDmaState->dmaTransmit.channel;
    /* Only need to set the DMA reg base addr once since it should be the same for all code */
    dmaBaseAddr = g_dmaRegBaseAddr[rxChannel/FSL_FEATURE_DMA_DMAMUX_CHANNELS];
    dmamuxBaseAddr = g_dmamuxRegBaseAddr[txChannel/FSL_FEATURE_DMAMUX_MODULE_CHANNEL];

    /* Check that we're not busy.*/
    if (spiDmaState->isTransferInProgress)
    {
        return kStatus_SPI_Busy;
    }

    /* Save information about the transfer for use by the ISR.*/
    spiDmaState->isTransferInProgress = true;

    /* The DONE needs to be cleared before programming the channel's TCDs for the next
     * transfer.
     */
    DMA_HAL_ClearStatus(dmaBaseAddr, rxChannel);
    DMA_HAL_ClearStatus(dmaBaseAddr, txChannel);

    /* Disable and enable the TX and RX DMA channel at the DMA mux. Doing so will prevent an
     * inadvertent DMA transfer when the TX and RX DMA channel ERQ bit is set after having been
     * cleared from a previous DMA transfer (clearing of the ERQ bit is automatically performed
     * at the end of a transfer when D_REQ is set).
     */
    DMAMUX_HAL_SetChannelCmd(dmamuxBaseAddr, txChannel, false);
    DMAMUX_HAL_SetChannelCmd(dmamuxBaseAddr, txChannel, true);
    DMAMUX_HAL_SetChannelCmd(dmamuxBaseAddr, rxChannel, false);
    DMAMUX_HAL_SetChannelCmd(dmamuxBaseAddr, rxChannel, true);

    /************************************************************************************
     * Set up the RX DMA channel Transfer Control Descriptor (TCD)
     * Note, if there is no receive byte count, then bypass RX DMA set up.
     ***********************************************************************************/
    if (spiDmaState->remainingReceiveByteCount)
    {
        /* If no receive buffer then disable incrementing the destination and set the destination
         * to a temporary location
         */
        if (!spiDmaState->receiveBuffer)
        {
            /* Set up this channel's control which includes enabling the DMA interrupt */
            DMA_DRV_ConfigTransfer(&spiDmaState->dmaReceive,
                                   kDmaPeripheralToMemory,
                                   transferSizeInBytes,
                                   SPI_HAL_GetDataRegAddr(spiBaseAddr), /* src is data register */
                                   (uint32_t)(&s_rxBuffIfNull), /* dest is temporary location */
                                   (uint32_t)(spiDmaState->remainingReceiveByteCount));

            /* Do not increment the destination address */
            DMA_HAL_SetDestIncrementCmd(dmaBaseAddr, rxChannel, false);
        }
        else
        {
            /* Set up this channel's control which includes enabling the DMA interrupt */
            DMA_DRV_ConfigTransfer(&spiDmaState->dmaReceive,
                                   kDmaPeripheralToMemory,
                                   transferSizeInBytes,
                                   SPI_HAL_GetDataRegAddr(spiBaseAddr), /* src is data register */
                                   (uint32_t)(spiDmaState->receiveBuffer),/* dest is rx buffer */
                                   (uint32_t)(spiDmaState->remainingReceiveByteCount));
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
        DMA_DRV_StartChannel(&spiDmaState->dmaReceive);

        /* Register callback for DMA interrupt */
        DMA_DRV_RegisterCallback(&spiDmaState->dmaReceive, SPI_DRV_DmaMasterCallback, param);

        /* Enable the SPI RX DMA Request after the TX DMA request is enabled.  This is done to
         * make sure that the RX DMA channel does not end prematurely before we've completely set
         * up the TX DMA channel since part of the TX DMA set up involves placing 1 or 2 bytes of
         * data into the send data register which causes an immediate transfer.
         */
    }

    /************************************************************************************
     * Set up the TX DMA channel Transfer Control Descriptor (TCD)
     * Note, if there is no source buffer (if user passes in NULL), then send zeros
     ***********************************************************************************/
    /* Per the reference manual, before enabling the SPI transmit DMA request, we first need
     * to read the status register and then write to the SPI data register.  Afterwards, we need
     * to decrement the sendByteCount and perform other driver maintenance functions.
     */

    /* Read the SPI Status register */
    SPI_HAL_IsTxBuffEmptyPending(spiBaseAddr);

    /* Start the transfer by writing the first byte/word to the SPI data register.
     * If a send buffer was provided, the byte/word comes from there. Otherwise we just send zeros.
     * This will cause an immeidate transfer which in some cases may cause the RX DMA channel to
     * complete before having the chance to completely set up the TX DMA channel. As such, we'll
     * enable the RX DMA channel last.
     */
#if FSL_FEATURE_SPI_16BIT_TRANSFERS
    if (transferSizeInBytes == 2) /* 16-bit transfers for SPI16 module */
    {
        if (spiDmaState->sendBuffer)
        {
            s_byteToSend = *(spiDmaState->sendBuffer);
            SPI_HAL_WriteDataLow(spiBaseAddr, s_byteToSend);
            ++spiDmaState->sendBuffer;

            s_byteToSend = *(spiDmaState->sendBuffer);
            SPI_HAL_WriteDataHigh(spiBaseAddr, s_byteToSend);
            ++spiDmaState->sendBuffer;
        }
        else  /* Else, if no send buffer, write zeros */
        {
            SPI_HAL_WriteDataLow(spiBaseAddr, s_byteToSend);
            SPI_HAL_WriteDataHigh(spiBaseAddr, s_byteToSend);
        }
        spiDmaState->remainingSendByteCount -= 2;  /* Decrement the send byte count by 2 */
    }
    else /* 8-bit transfers for SPI16 module */
    {
        if (spiDmaState->sendBuffer)
        {
            s_byteToSend = *(spiDmaState->sendBuffer);
            ++spiDmaState->sendBuffer;
        }
        SPI_HAL_WriteDataLow(spiBaseAddr, s_byteToSend); /* If no send buffer, s_byteToSend=0 */
        --spiDmaState->remainingSendByteCount; /* Decrement the send byte count */
    }
#else
    /* For SPI modules that do not support 16-bit transfers */
    if (spiDmaState->sendBuffer)
    {
        s_byteToSend = *(spiDmaState->sendBuffer);
        ++spiDmaState->sendBuffer;
    }
    SPI_HAL_WriteData(spiBaseAddr, s_byteToSend); /* If no send buffer, s_byteToSend=0 */
    --spiDmaState->remainingSendByteCount; /* Decrement the send byte count */
#endif

    /* If there are no more bytes to send then return without setting up the TX DMA channel
     * and let the receive DMA channel complete the transfer if the RX DMA channel was setup.
     * If the RX DMA channel was not set up (due to odd byte count of 1 in 16-bit mode), enable
     * the interrupt to get the received byte.
     */
    if (!spiDmaState->remainingSendByteCount) /* No more bytes to send */
    {
        if (spiDmaState->remainingReceiveByteCount)
        {
            /* Enable the RX DMA channel request now  */
            SPI_HAL_SetRxDmaCmd(spiBaseAddr, true);
            return kStatus_SPI_Success;
        }
        else /* If RX DMA chan not setup then enable the interrupt to get the received byte */
        {
            SPI_HAL_SetTransmitIntCmd(spiBaseAddr, true);
            return kStatus_SPI_Success;
        }
    }
    /* Else, since there are more bytes to send, go ahead and set up the TX DMA channel */
    else
    {
        /* If there is a send buffer, data comes from there, else send 0 */
        if (spiDmaState->sendBuffer)
        {
            /* Set up this channel's control which includes enabling the DMA interrupt */
            DMA_DRV_ConfigTransfer(&spiDmaState->dmaTransmit, kDmaMemoryToPeripheral,
                                   transferSizeInBytes,
                                   (uint32_t)(spiDmaState->sendBuffer),
                                   SPI_HAL_GetDataRegAddr(spiBaseAddr),
                                   (uint32_t)(spiDmaState->remainingSendByteCount));
        }
        else /* Configure TX DMA channel to send zeros */
        {
            /* Set up this channel's control which includes enabling the DMA interrupt */
            DMA_DRV_ConfigTransfer(&spiDmaState->dmaTransmit, kDmaMemoryToPeripheral,
                                   transferSizeInBytes,
                                   (uint32_t)(&s_byteToSend),
                                   SPI_HAL_GetDataRegAddr(spiBaseAddr),
                                   (uint32_t)(spiDmaState->remainingSendByteCount));

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
        DMA_DRV_StartChannel(&spiDmaState->dmaTransmit);

        /* Enable the SPI TX DMA Request */
        SPI_HAL_SetTxDmaCmd(spiBaseAddr, true);

        /* Enable the SPI RX DMA Request after the TX DMA request is enabled.  This is done to
         * make sure that the RX DMA channel does not end prematurely before we've completely set
         * up the TX DMA channel since part of the TX DMA set up involves placing 1 or 2 bytes of
         * data into the send data register which causes an immediate transfer.
         */
        SPI_HAL_SetRxDmaCmd(spiBaseAddr, true);
    }

    return kStatus_SPI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaMasterGetTransferStatus
 * Description   : Returns whether the previous transfer finished with DMA support.
 *
 * When performing an a-sync transfer, the user can call this function to ascertain the state of the
 * current transfer: in progress (or busy) or complete (success). In addition, if the transfer
 * is still in progress, the user can get the number of words that have been
 * transferred up to now.
 *
 *END**************************************************************************/
spi_status_t SPI_DRV_DmaMasterGetTransferStatus(uint32_t instance,
                                                uint32_t * bytesTransferred)
{
    /* instantiate local variable of type spi_dma_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];

    /* Fill in the bytes transferred.*/
    if (bytesTransferred)
    {
        *bytesTransferred = spiDmaState->remainingSendByteCount -
                            DMA_DRV_GetUnfinishedBytes(&spiDmaState->dmaTransmit);
    }

    return (spiDmaState->isTransferInProgress ? kStatus_SPI_Busy : kStatus_SPI_Success);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SPI_DRV_DmaMasterAbortTransfer
 * Description   : Terminates an asynchronous transfer early with DMA support.
 *
 * During an async transfer, the user has the option to terminate the transfer early if the transfer
 * is still in progress.
 *
 *END**************************************************************************/
spi_status_t SPI_DRV_DmaMasterAbortTransfer(uint32_t instance)
{
    /* instantiate local variable of type spi_dma_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];

    /* Check if a transfer is running.*/
    if (!spiDmaState->isTransferInProgress)
    {
        return kStatus_SPI_NoTransferInProgress;
    }

    /* Stop the running transfer.*/
    SPI_DRV_DmaMasterCompleteTransfer(instance);

    return kStatus_SPI_Success;
}

/*!
 * @brief Finish up a transfer.
 * Cleans up after a transfer is complete. Interrupts are disabled, and the SPI module
 * is disabled. This is not a public API as it is called from other driver functions.
 */
static void SPI_DRV_DmaMasterCompleteTransfer(uint32_t instance)
{
    /* instantiate local variable of type spi_dma_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];

    uint32_t spiBaseAddr = g_spiBaseAddr[instance];

    /* Disable DMA requests and interrupts. */
    SPI_HAL_SetRxDmaCmd(spiBaseAddr, false);
    SPI_HAL_SetTxDmaCmd(spiBaseAddr, false);
    SPI_HAL_SetTransmitIntCmd(spiBaseAddr, false);

    /* The transfer is complete.*/
    spiDmaState->isTransferInProgress = false;

    if (spiDmaState->isTransferBlocking)
    {
        /* Signal the synchronous completion object */
        OSA_SemaPost(&spiDmaState->irqSync);
    }
}

/*!
 * @brief Interrupt handler for SPI master mode.
 * This handler is used when the extraByte flag is set to retrieve the received last byte.
 * This is not a public API as it is called whenever an interrupt occurs.
 */
void SPI_DRV_DmaMasterIRQHandler(uint32_t instance)
{
    /* instantiate local variable of type spi_master_state_t and point to global state */
    spi_dma_master_state_t * spiDmaState = (spi_dma_master_state_t *)g_spiDmaStatePtr[instance];

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
                    spiDmaState->receiveBuffer[spiDmaState->transferByteCnt-2] =
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

                SPI_DRV_DmaMasterCompleteTransfer(instance);
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
                    spiDmaState->receiveBuffer[spiDmaState->transferByteCnt-2] =
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
                SPI_DRV_DmaMasterCompleteTransfer(instance);
            }
        }
#endif
}

/*******************************************************************************
 * EOF
 ******************************************************************************/

