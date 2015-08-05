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

#include <assert.h>
#include <string.h>
#include "fsl_lpsci_driver.h"
#include "fsl_interrupt_manager.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Pointer to lpsci runtime state structure */
extern void * g_lpsciStatePtr[HW_UART0_INSTANCE_COUNT];

/*******************************************************************************
 * Private Functions
 ******************************************************************************/
static void LPSCI_DRV_CompleteSendData(uint32_t instance);
static lpsci_status_t LPSCI_DRV_StartSendData(uint32_t instance,
                                              const uint8_t * txBuff,
                                              uint32_t txSize);
static void LPSCI_DRV_CompleteReceiveData(uint32_t instance);
static lpsci_status_t LPSCI_DRV_StartReceiveData(uint32_t instance,
                                                 uint8_t * rxBuff,
                                                 uint32_t rxSize);

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_Init
 * Description   : This function initializes a LPSCI instance for operation.
 * This function will initialize the run-time state structure to keep track of
 * the on-going transfers, ungate the clock to the LPSCI module, initialize the
 * module to user defined settings and default settings, configure the IRQ state
 * structure and enable the module-level interrupt to the core, and enable the
 * LPSCI module transmitter and receiver.
 * The following is an example of how to set up the lpsci_state_t and the
 * lpsci_user_config_t parameters and how to call the LPSCI_DRV_Init function
 * by passing in these parameters:
 *    lpsci_user_config_t lpsciConfig;
 *    lpsciConfig.clockSource = kClockLpsciSrcPllFllSel;
 *    lpsciConfig.baudRate = 9600;
 *    lpsciConfig.bitCountPerChar = kLpsci8BitsPerChar;
 *    lpsciConfig.parityMode = kLpsciParityDisabled;
 *    lpsciConfig.stopBitCount = kLpsciOneStopBit;
 *    lpsci_state_t lpsciState;
 *    LPSCI_DRV_Init(instance, &lpsciState, &lpsciConfig);
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_DRV_Init(uint32_t instance,
                              lpsci_state_t * lpsciStatePtr,
                              const lpsci_user_config_t * lpsciUserConfig)
{
    assert(lpsciStatePtr && lpsciUserConfig);
    assert(instance < HW_UART0_INSTANCE_COUNT);

    uint32_t baseAddr = g_lpsciBaseAddr[instance];
    uint32_t lpsciSourceClock;

    /* Exit if current instance is already initialized. */
    if (g_lpsciStatePtr[instance])
    {
        return kStatus_LPSCI_Initialized;
    }

    /* Clear the state structure for this instance. */
    memset(lpsciStatePtr, 0, sizeof(lpsci_state_t));

    /* Save runtime structure pointer.*/
    g_lpsciStatePtr[instance] = lpsciStatePtr;

    /* Un-gate LPSCI module clock */
    CLOCK_SYS_EnableLpsciClock(instance);

    /* Set LPSCI clock source */
    CLOCK_SYS_SetLpsciSrc(instance, lpsciUserConfig->clockSource);

    /* Initialize LPSCI to a known state. */
    LPSCI_HAL_Init(baseAddr);

    /* Create Semaphore for txIrq and rxIrq. */
    OSA_SemaCreate(&lpsciStatePtr->txIrqSync, 0);
    OSA_SemaCreate(&lpsciStatePtr->rxIrqSync, 0);

    lpsciSourceClock = CLOCK_SYS_GetLpsciFreq(instance);

    /* Initialize LPSCI baud rate, bit count, parity and stop bit. */
    LPSCI_HAL_SetBaudRate(baseAddr, lpsciSourceClock, lpsciUserConfig->baudRate);
    LPSCI_HAL_SetBitCountPerChar(baseAddr, lpsciUserConfig->bitCountPerChar);
    LPSCI_HAL_SetParityMode(baseAddr, lpsciUserConfig->parityMode);
#if FSL_FEATURE_LPSCI_HAS_STOP_BIT_CONFIG_SUPPORT
    LPSCI_HAL_SetStopBitCount(baseAddr, lpsciUserConfig->stopBitCount);
#endif

    /* Enable LPSCI interrupt on NVIC level. */
    INT_SYS_EnableIRQ(g_lpsciRxTxIrqId[instance]);

    /* Finally, enable the LPSCI transmitter and receiver*/
    LPSCI_HAL_EnableTransmitter(baseAddr);
    LPSCI_HAL_EnableReceiver(baseAddr);

    return kStatus_LPSCI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_Deinit
 * Description   : This function shuts down the LPSCI by disabling interrupts
 * and the transmitter/receiver.
 *
 *END**************************************************************************/
void LPSCI_DRV_Deinit(uint32_t instance)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);

    uint32_t baseAddr = g_lpsciBaseAddr[instance];
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];

    /* Wait until the data is completely shifted out of shift register */
    while(!(LPSCI_HAL_IsTxComplete(baseAddr))) { }

    /* Disable the interrupt */
    INT_SYS_DisableIRQ(g_lpsciRxTxIrqId[instance]);

    /* Disable TX and RX */
    LPSCI_HAL_DisableTransmitter(baseAddr);
    LPSCI_HAL_DisableReceiver(baseAddr);

    /* Destroy TX and RX sema. */
    OSA_SemaDestroy(&lpsciState->txIrqSync);
    OSA_SemaDestroy(&lpsciState->rxIrqSync);

    /* Cleared state pointer. */
    g_lpsciStatePtr[instance] = NULL;

    /* Gate LPSCI module clock */
    CLOCK_SYS_DisableUartClock(instance);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_InstallRxCallback
 * Description   : Install receive data callback function.
 *
 *END**************************************************************************/
lpsci_rx_callback_t LPSCI_DRV_InstallRxCallback(uint32_t instance, 
                                                lpsci_rx_callback_t function, 
                                                uint8_t * rxBuff, 
                                                void * callbackParam,
                                                bool alwaysEnableRxIrq)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);
    uint32_t baseAddr = g_lpsciBaseAddr[instance];
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];
    
    lpsci_rx_callback_t currentCallback = lpsciState->rxCallback;
    lpsciState->rxCallback = function;
    lpsciState->rxCallbackParam = callbackParam;
    lpsciState->rxBuff = rxBuff;

    /* Enable/Disable the receive data full interrupt */
    lpsciState->isRxBusy = true;
    LPSCI_HAL_SetRxDataRegFullIntCmd(baseAddr, alwaysEnableRxIrq);

    return currentCallback;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_SendDataBlocking
 * Description   : This function sends data out through the LPSCI module using a
 * blocking method. It does not return until the transmit is complete. 
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_DRV_SendDataBlocking(uint32_t instance,
                                          const uint8_t * txBuff,
                                          uint32_t txSize,
                                          uint32_t timeout)
{
    assert(txBuff);
    assert(instance < HW_UART0_INSTANCE_COUNT);

    uint32_t baseAddr = g_lpsciBaseAddr[instance];
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];
    lpsci_status_t retVal = kStatus_LPSCI_Success;
    osa_status_t syncStatus;

    lpsciState->isTxBlocking = true;

    /* Start the transmission process */
    retVal = LPSCI_DRV_StartSendData(instance, txBuff, txSize);

    if (retVal == kStatus_LPSCI_Success)
    {
        /* Wait until the transmit is complete. */
        do
        {
            syncStatus = OSA_SemaWait(&lpsciState->txIrqSync, timeout);
        }while(syncStatus == kStatus_OSA_Idle);

        if (syncStatus != kStatus_OSA_Success)
        {
            /* Disable the transmitter data register empty interrupt */
            LPSCI_HAL_SetTxDataRegEmptyIntCmd(baseAddr, false);

            /* Update the information of the module driver state */
            lpsciState->isTxBusy = false; 

            retVal = kStatus_LPSCI_Timeout;
        }
    }

    return retVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_SendData
 * Description   : This function sends data through the LPSCI module using a
 * non-blocking method. It returns immediately after initiating the transmit
 * function. The application has to get the transmit status to see when the
 * transmit is complete. In other words, after calling non-blocking send
 * function, the application must get the transmit status to check if transmit
 * is completed or not.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_DRV_SendData(uint32_t instance,
                                  const uint8_t * txBuff,
                                  uint32_t txSize)
{
    assert(txBuff);
    assert(instance < HW_UART0_INSTANCE_COUNT);

    lpsci_status_t retVal = kStatus_LPSCI_Success;
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];

    lpsciState->isTxBlocking = false;

    /* Start the transmission process*/
    retVal = LPSCI_DRV_StartSendData(instance, txBuff, txSize);
    
    return retVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_GetTransmitStatus
 * Description   : This function returns whether the previous LPSCI transmit has
 * finished. When performing a non-blocking transmit, the user can call this
 * function to ascertain the state of current transmission: in progress (busy)
 * or complete (success). In addition, if the transmission is still in progress,
 * the user can obtain the number of words that left for transferring.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_DRV_GetTransmitStatus(uint32_t instance,
                                           uint32_t * bytesRemaining)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);

    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];
    lpsci_status_t retVal = kStatus_LPSCI_Success;
    uint32_t txSize = lpsciState->txSize;

    if (bytesRemaining)
    {
        *bytesRemaining = txSize;
    }

    if (txSize)
    {
        retVal = kStatus_LPSCI_TxBusy;
    }

    return retVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_AbortSendingData
 * Description   : This function terminates a non-blocking transmission early.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_DRV_AbortSendingData(uint32_t instance)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);

    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];

    /* Check if a transfer is running. */
    if (!lpsciState->isTxBusy)
    {
        return kStatus_LPSCI_NoTransmitInProgress;
    }

    /* Stop the running transfer. */
    LPSCI_DRV_CompleteSendData(instance);

    return kStatus_LPSCI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_ReceiveDataBlocking
 * Description   : This function receives data from LPSCI using a blocking
 * method. It does not return until the receive is complete.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_DRV_ReceiveDataBlocking(uint32_t instance,
                                             uint8_t * rxBuff,
                                             uint32_t rxSize,
                                             uint32_t timeout)
{
    assert(rxBuff);
    assert(instance < HW_UART0_INSTANCE_COUNT);

    uint32_t baseAddr = g_lpsciBaseAddr[instance];
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];
    lpsci_status_t retVal = kStatus_LPSCI_Success;
    osa_status_t syncStatus;

    lpsciState->isRxBlocking = true;

    retVal = LPSCI_DRV_StartReceiveData(instance, rxBuff, rxSize);

    if (retVal == kStatus_LPSCI_Success)
    {
        /* Wait until all the data is received or for timeout.*/
        do
        {
            syncStatus = OSA_SemaWait(&lpsciState->rxIrqSync, timeout);
        }while(syncStatus == kStatus_OSA_Idle);

        if (syncStatus != kStatus_OSA_Success)
        {
            /* Disable the receive data full and overrun interrupt */
            LPSCI_HAL_SetTxDataRegEmptyIntCmd(baseAddr, false);
            LPSCI_HAL_SetIntMode(baseAddr, kLpsciIntRxOverrun, false);

            /* Update the information of the module driver state */
            lpsciState->isTxBusy = false; 

            retVal = kStatus_LPSCI_Timeout;
        }
    }

    return retVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_ReceiveData
 * Description   : This function receives data from LPSCI using a non-blocking
 * method. A non-blocking function means that the function returns immediately
 * after initiating the receive function. The application has to get the
 * receive status to see when the receive is complete. In other words, after
 * calling non-blocking get function, the application must get the receive
 * status to check if receive is completed or not.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_DRV_ReceiveData(uint32_t instance,
                                     uint8_t * rxBuff,
                                     uint32_t rxSize)
{
    assert(rxBuff);
    assert(instance < HW_UART0_INSTANCE_COUNT);

    lpsci_status_t retVal = kStatus_LPSCI_Success;
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];

    lpsciState->isRxBlocking = false;

    retVal = LPSCI_DRV_StartReceiveData(instance, rxBuff, rxSize);
    
    return retVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_GetReceiveStatus
 * Description   : This function returns whether the previous LPSCI receive is
 * complete. When performing a non-blocking receive, the user can call this
 * function to ascertain the state of current progress: in progress (busy)
 * or complete (success). In addition, if the receive is still in progress,
 * the user can obtain the number of words that need to be received.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_DRV_GetReceiveStatus(uint32_t instance,
                                          uint32_t * bytesRemaining)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];
    lpsci_status_t retVal = kStatus_LPSCI_Success;
    uint32_t rxSize = lpsciState->rxSize;

    if (bytesRemaining)
    {
        *bytesRemaining = rxSize;
    }

    if (rxSize)
    {
        retVal = kStatus_LPSCI_RxBusy;
    }

    return retVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_AbortReceivingData
 * Description   : This function terminates a non-blocking receive early.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_DRV_AbortReceivingData(uint32_t instance)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];

    /* Check if a transfer is running. */
    if (!lpsciState->isRxBusy)
    {
        return kStatus_LPSCI_NoReceiveInProgress;
    }

    /* Stop the running transfer. */
    LPSCI_DRV_CompleteReceiveData(instance);

    return kStatus_LPSCI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_IRQHandler
 * Description   : Interrupt handler for LPSCI.
 * This is not a public API as it is called whenever an interrupt occurs.
 *
 *END**************************************************************************/
void LPSCI_DRV_IRQHandler(uint32_t instance)
{
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];
    uint32_t baseAddr = g_lpsciBaseAddr[instance];

    /* Exit the ISR if no transfer is happening for this instance. */
    if ((!lpsciState->isTxBusy) && (!lpsciState->isRxBusy))
    {
        return;
    }

    /* Handle Rx Data Register Full interrupt */
    if((LPSCI_HAL_GetRxDataRegFullIntCmd(baseAddr))
      && (LPSCI_HAL_IsRxDataRegFull(baseAddr)))
    {
        /* Get data and put in receive buffer */
        LPSCI_HAL_Getchar(baseAddr, lpsciState->rxBuff);

        /* Invoke callback if there is one */
        if (lpsciState->rxCallback != NULL)
        {
            lpsciState->rxCallback(instance, lpsciState);
        }
        else
        {
            ++lpsciState->rxBuff;
            --lpsciState->rxSize;

            /* Check and see if this was the last byte received */
            if (lpsciState->rxSize == 0)
            {
                LPSCI_DRV_CompleteReceiveData(instance);
            }
        }
    }

    /* Handle Tx Data Register Empty interrupt */
    if((LPSCI_HAL_GetTxDataRegEmptyIntCmd(baseAddr))
      && (LPSCI_HAL_IsTxDataRegEmpty(baseAddr)))
    {
        /* Check to see if there are any more bytes to send */
        if (lpsciState->txSize)
        {
            /* Transmit data and update tx size/buff. */
            LPSCI_HAL_Putchar(baseAddr, *(lpsciState->txBuff));
            ++lpsciState->txBuff; 
            --lpsciState->txSize;

            if (lpsciState->txSize == 0)
            {
                /* Complete the transfer and disable the interrupt */
                LPSCI_DRV_CompleteSendData(instance);
            }
        }
    }

    /* Handle receive overrun interrupt */
    if (LPSCI_HAL_GetStatusFlag(baseAddr, kLpsciRxOverrun))
    {
        /* Clear the flag, OR the rxDataRegFull will not be set any more */
        LPSCI_HAL_ClearStatusFlag(baseAddr, kLpsciRxOverrun);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_CompleteSendData
 * Description   : Finish up a transmit by completing the process of sending 
 * data and disabling the interrupt. 
 * This is not a public API as it is called from other driver functions.
 *
 *END**************************************************************************/
static void LPSCI_DRV_CompleteSendData(uint32_t instance)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);

    uint32_t baseAddr = g_lpsciBaseAddr[instance];
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];

    /* Disable the transmitter data register empty interrupt */
    LPSCI_HAL_SetTxDataRegEmptyIntCmd(baseAddr, false);

    /* Signal the synchronous completion object. */
    if (lpsciState->isTxBlocking)
    {
        OSA_SemaPost(&lpsciState->txIrqSync);
    }

    /* Update the information of the module driver state */
    lpsciState->isTxBusy = false; 
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_StartSendData
 * Description   : Initiate a transmit by beginning the process of sending data
 * and enabling the interrupt.
 * This is not a public API as it is called from other driver functions.
 *
 *END**************************************************************************/
static lpsci_status_t LPSCI_DRV_StartSendData(uint32_t instance,
                                              const uint8_t * txBuff,
                                              uint32_t txSize)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);

    uint32_t baseAddr = g_lpsciBaseAddr[instance];
    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];

    /* Check that we're not busy sending data from a previous function call. */
    if (lpsciState->isTxBusy)
    {
        return kStatus_LPSCI_TxBusy;
    }

    if (txSize == 0U)
    {
        return kStatus_LPSCI_NoDataToDeal;
    }

    /* Initialize the module driver state structure. */
    lpsciState->txBuff = txBuff;
    lpsciState->txSize = txSize;
    lpsciState->isTxBusy = true;

    /* Make sure the transmit data register is empty and ready for data */
    while(!LPSCI_HAL_IsTxDataRegEmpty(baseAddr)) { }

    /* Enable the transmitter data register empty interrupt.*/
    LPSCI_HAL_SetTxDataRegEmptyIntCmd(baseAddr, true);

    return kStatus_LPSCI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_CompleteReceiveData
 * Description   : Finish up a receive by completing the process of receiving data
 * and disabling the interrupt. 
 * This is not a public API as it is called from other driver functions.
 *
 *END**************************************************************************/
static void LPSCI_DRV_CompleteReceiveData(uint32_t instance)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);

    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];
    uint32_t baseAddr = g_lpsciBaseAddr[instance];

    /* Disable receive data full and overrun interrupt */
    LPSCI_HAL_SetRxDataRegFullIntCmd(baseAddr, false);
    LPSCI_HAL_SetIntMode(baseAddr, kLpsciIntRxOverrun, false);

    /* Signal the synchronous completion object. */
    if (lpsciState->isRxBlocking)
    {
        OSA_SemaPost(&lpsciState->rxIrqSync);
    }

    /* Update the information of the module driver state */
    lpsciState->isRxBusy = false;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_DRV_StartReceiveData
 * Description   : Initiate (start) a receive by beginning the process of
 * receiving data and enabling the interrupt. 
 * This is not a public API as it is called from other driver functions.
 *
 *END**************************************************************************/
static lpsci_status_t LPSCI_DRV_StartReceiveData(uint32_t instance,
                                                 uint8_t * rxBuff,
                                                 uint32_t rxSize)
{
    assert(instance < HW_UART0_INSTANCE_COUNT);

    lpsci_state_t * lpsciState = (lpsci_state_t *)g_lpsciStatePtr[instance];
    uint32_t baseAddr = g_lpsciBaseAddr[instance];

    /* Check that we're not busy receiving data from a previous function call. */
    if ((lpsciState->isRxBusy) && (!lpsciState->rxCallback))
    {
        return kStatus_LPSCI_RxBusy;
    }

    if (rxSize == 0U)
    {
        return kStatus_LPSCI_NoDataToDeal;
    }

    /* Initialize the module driver state structure. */
    lpsciState->rxBuff = rxBuff;
    lpsciState->rxSize = rxSize;
    lpsciState->isRxBusy = true;

    /* Enable the receive data overrun interrupt */
    LPSCI_HAL_SetIntMode(baseAddr, kLpsciIntRxOverrun, true);

    /* Enable the receive data full interrupt */
    LPSCI_HAL_SetRxDataRegFullIntCmd(baseAddr, true);

    return kStatus_LPSCI_Success;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/

