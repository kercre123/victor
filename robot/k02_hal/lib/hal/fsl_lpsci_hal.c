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
#include "fsl_lpsci_hal.h"

/*******************************************************************************
 * Code
 ******************************************************************************/
/*******************************************************************************
 * LPSCI Common Configurations
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_Init
 * Description   : This function initializes the module to a known state.
 *
 *END**************************************************************************/
void LPSCI_HAL_Init(uint32_t baseAddr)
{
    HW_UART0_BDH_WR(baseAddr, 0U);
    HW_UART0_BDL_WR(baseAddr, 4U);
    HW_UART0_C1_WR(baseAddr, 0U);
    HW_UART0_C2_WR(baseAddr, 0U);
    HW_UART0_S2_WR(baseAddr, 0U);
    HW_UART0_C3_WR(baseAddr, 0U);
#if FSL_FEATURE_LPSCI_HAS_ADDRESS_MATCHING
    HW_UART0_MA1_WR(baseAddr, 0U);
    HW_UART0_MA2_WR(baseAddr, 0U);
#endif
    HW_UART0_C4_WR(baseAddr, 0U);
#if FSL_FEATURE_LPSCI_HAS_DMA_ENABLE
    HW_UART0_C5_WR(baseAddr, 0U);
#endif
#if FSL_FEATURE_LPSCI_HAS_MODEM_SUPPORT
    HW_UART0_MODEM_WR(baseAddr, 0U);
#endif
#if FSL_FEATURE_LPSCI_HAS_IR_SUPPORT
    HW_UART0_IR_WR(baseAddr, 0U);
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_SetBaudRate
 * Description   : Configure the LPSCI baud rate.
 * This function programs the LPSCI baud rate to the desired value passed in by
 * the user. The user must also pass in the module source clock so that the
 * function can calculate the baud rate divisors to their appropriate values.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_HAL_SetBaudRate(uint32_t baseAddr,
                                     uint32_t sourceClockInHz,
                                     uint32_t baudRate)
{
    uint16_t sbrTemp;
    uint32_t osr, sbr;
    uint8_t i;
    uint32_t tempDiff, calculatedBaud, baudDiff;

    /* This uart instantiation uses a slightly different baud rate calculation
     * The idea is to use the best OSR (over-sampling rate) possible
     * Note, osr is typically hard-set to 16 in other uart instantiations
     * First calculate the baud rate using the minimum OSR possible (4). */
    osr = 4;
    sbr = (sourceClockInHz/(baudRate * osr));
    calculatedBaud = (sourceClockInHz / (osr * sbr));

    if (calculatedBaud > baudRate)
    {
        baudDiff = calculatedBaud - baudRate;
    }
    else
    {
        baudDiff = baudRate - calculatedBaud;
    }

    /* loop to find the best osr value possible, one that generates minimum baudDiff
     * iterate through the rest of the supported values of osr */
    for (i = 5; i <= 32; i++)
    {
        /* calculate the temporary sbr value   */
        sbrTemp = (sourceClockInHz/(baudRate * i));
        /* calculate the baud rate based on the temporary osr and sbr values*/
        calculatedBaud = (sourceClockInHz / (i * sbrTemp));

        if (calculatedBaud > baudRate)
        {
            tempDiff = calculatedBaud - baudRate;
        }
        else
        {
            tempDiff = baudRate - calculatedBaud;
        }

        if (tempDiff <= baudDiff)
        {
            baudDiff = tempDiff;
            osr = i;  /* update and store the best osr value calculated*/
            sbr = sbrTemp;  /* update store the best sbr value calculated*/
        }
    }

    /* next, check to see if actual baud rate is within 3% of desired baud rate
     * based on the best calculate osr value */
    if (baudDiff < ((baudRate / 100) * 3))
    {
        /* Acceptable baud rate */
        /* Check if osr is between 4x and 7x oversampling*/
        /* If so, then "BOTHEDGE" sampling must be turned on*/
        if ((osr > 3) && (osr < 8))
        {
            HW_UART0_C5_SET(baseAddr, BM_UART0_C5_BOTHEDGE);
        }

        /* program the osr value (bit value is one less than actual value)*/
        BW_UART0_C4_OSR(baseAddr, osr-1);

        /* program the sbr (divider) value obtained above*/
        BW_UART0_BDH_SBR(baseAddr, (uint8_t)(sbr >> 8));
        BW_UART0_BDL_SBR(baseAddr, (uint8_t)sbr);
    }
    else
    {
        /* Unacceptable baud rate difference of more than 3%*/
        return kStatus_LPSCI_BaudRateCalculationError ;
    }

    return kStatus_LPSCI_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_SetBaudRateDivisor
 * Description   : Set the LPSCI baud rate modulo divisor value.
 * This function allows the user to program the baud rate divisor directly in
 * situations where the divisor value is known. In this case, the user may not
 * want to call the LPSCI_HAL_SetBaudRate() function as the divisor is already
 * known to them.
 *
 *END**************************************************************************/
void LPSCI_HAL_SetBaudRateDivisor(uint32_t baseAddr, uint16_t baudRateDivisor)
{
    /* check to see if baudRateDivisor is out of range of register bits */
    assert( (baudRateDivisor < 0x1FFF) && (baudRateDivisor > 1) );

    /* program the sbr (baudRateDivisor) value to the BDH and BDL registers*/
    BW_UART0_BDH_SBR(baseAddr, (uint8_t)(baudRateDivisor >> 8));
    BW_UART0_BDL_SBR(baseAddr, (uint8_t)baudRateDivisor);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_SetTxRxInversionCmd
 * Description   : Configures the parity mode in the LPSCI controller.
 * This function allows the user to configure the parity mode of the LPSCI
 * controller to disable it or enable it for even parity or for odd parity.
 *
 *END**************************************************************************/
void LPSCI_HAL_SetParityMode(uint32_t baseAddr, lpsci_parity_mode_t parityMode)
{
    BW_UART0_C1_PE(baseAddr, parityMode >> 1U);
    BW_UART0_C1_PT(baseAddr, parityMode & 1U);
}

/*******************************************************************************
 * LPSCI Transfer Functions
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_Putchar9
 * Description   : This function allows the user to send a 9-bit character from
 * the LPSCI data register.
 *
 *END**************************************************************************/
void LPSCI_HAL_Putchar9(uint32_t baseAddr, uint16_t data)
{
    uint8_t ninthDataBit;

    ninthDataBit = (data >> 8U) & 0x1U;

    /* Write to the ninth data bit (bit position T8) */
    BW_UART0_C3_R9T8(baseAddr, ninthDataBit);

    HW_UART0_D_WR(baseAddr, (uint8_t)data);
}
/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_Putchar10
 * Description   : This function allows the user to send a 10-bit character from
 * the LPSCI data register.
 *
 *END**************************************************************************/
void LPSCI_HAL_Putchar10(uint32_t baseAddr, uint16_t data)
{
    uint8_t ninthDataBit, tenthDataBit;

    ninthDataBit = (data >> 8U) & 0x1U;
    tenthDataBit = (data >> 9U) & 0x1U;

    /* Write to the tenth data bit (bit position T9) */
    BW_UART0_C3_R8T9(baseAddr, tenthDataBit);

    /* Write to the ninth data bit (bit position T8) */
    BW_UART0_C3_R9T8(baseAddr, ninthDataBit);

    HW_UART0_D_WR(baseAddr, (uint8_t)data);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_Getchar9
 * Description   : This function gets a received 9-bit character from the LPSCI
 * data register.
 *
 *END**************************************************************************/
void LPSCI_HAL_Getchar9(uint32_t baseAddr, uint16_t *readData)
{
    *readData = (uint16_t)BR_UART0_C3_R8T9(baseAddr) << 8;
    *readData |= HW_UART0_D_RD(baseAddr);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_Getchar10
 * Description   : This function gets a received 10-bit character from the LPSCI
 * data register.
 *
 *END**************************************************************************/
void LPSCI_HAL_Getchar10(uint32_t baseAddr, uint16_t *readData)
{
    *readData = (uint16_t)((uint32_t)(BR_UART0_C3_R9T8(baseAddr)) << 9U);
    *readData |= (uint16_t)((uint32_t)(BR_UART0_C3_R8T9(baseAddr)) << 8U);
    *readData |= HW_UART0_D_RD(baseAddr);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_SendDataPolling
 * Description   : Send out multiple bytes of data using polling method.
 * This function only supports 8-bit transaction.
 *
 *END**************************************************************************/
void LPSCI_HAL_SendDataPolling(uint32_t baseAddr,
                               const uint8_t *txBuff,
                               uint32_t txSize)
{
    while (txSize--)
    {
        while (!LPSCI_HAL_IsTxDataRegEmpty(baseAddr))
        {}

        LPSCI_HAL_Putchar(baseAddr, *txBuff++);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_ReceiveDataPolling
 * Description   : Receive multiple bytes of data using polling method.
 * This function only supports 8-bit transaction.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_HAL_ReceiveDataPolling(uint32_t baseAddr,
                                            uint8_t *rxBuff,
                                            uint32_t rxSize)
{
    lpsci_status_t retVal = kStatus_LPSCI_Success;

    while (rxSize--)
    {
        while (!LPSCI_HAL_IsRxDataRegFull(baseAddr))
        {}

        LPSCI_HAL_Getchar(baseAddr, rxBuff++);

        /* Clear the Overrun flag since it will block receiving */
        if(BR_UART0_S1_OR(baseAddr))
        {
            BW_UART0_S1_OR(baseAddr, 1U);
            retVal = kStatus_LPSCI_RxOverRun;
        }
    }

    return retVal;
}

/*******************************************************************************
 * LPSCI Interrupts and DMA
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_ConfigureInterrupts
 * Description   : Configure the LPSCI module interrupts to enable/disable
 * various interrupt sources.
 *
 *END**************************************************************************/
void LPSCI_HAL_SetIntMode(uint32_t baseAddr, lpsci_interrupt_t interrupt, bool enable)
{
    uint8_t reg = (uint32_t)interrupt >> LPSCI_SHIFT;
    uint32_t temp = 1U << (uint8_t)interrupt;

    switch ( reg )
    {
        case 0 :
            enable ? HW_UART0_BDH_SET(baseAddr, temp) : HW_UART0_BDH_CLR(baseAddr, temp);
            break;
        case 1 :
            enable ? HW_UART0_C2_SET(baseAddr, temp) : HW_UART0_C2_CLR(baseAddr, temp);
            break;
        case 2 :
            enable ? HW_UART0_C3_SET(baseAddr, temp) : HW_UART0_C3_CLR(baseAddr, temp);
            break;
        default :
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_GetIntMode
 * Description   : Return whether the LPSCI module interrupts is enabled/disabled.
 *
 *END**************************************************************************/
bool LPSCI_HAL_GetIntMode(uint32_t baseAddr, lpsci_interrupt_t interrupt)
{
    uint8_t reg = (uint32_t)interrupt >> LPSCI_SHIFT;
    uint8_t temp = 0;

    switch ( reg )
    {
        case 0 :
            temp = HW_UART0_BDH_RD(baseAddr) >> (uint8_t)(interrupt) & 1U;
            break;
        case 1 :
            temp = HW_UART0_C2_RD(baseAddr) >> (uint8_t)(interrupt) & 1U;
            break;
        case 2 :
            temp = HW_UART0_C3_RD(baseAddr) >> (uint8_t)(interrupt) & 1U;
            break;
        default :
            break;
    }
    return (bool)temp;
}

#if FSL_FEATURE_LPSCI_HAS_DMA_ENABLE
/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_SetTxDmaCmd
 * Description   : Enable or disable LPSCI DMA request for Transmitter.
 * This function allows the user to configure the receive data register full
 * flag to generate a DMA request.
 *
 *END**************************************************************************/
void LPSCI_HAL_SetTxDmaCmd(uint32_t baseAddr, bool enable)
{
    BW_UART0_C5_TDMAE(baseAddr, enable);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_SetRxDmaCmd
 * Description   : Enable or disable LPSCI DMA request for Receiver.
 * This function allows the user to configure the receive data register full
 * flag to generate a DMA request.
 *
 *END**************************************************************************/
void LPSCI_HAL_SetRxDmaCmd(uint32_t baseAddr, bool enable)
{
    BW_UART0_C5_RDMAE(baseAddr, enable);
}
#endif

/*******************************************************************************
 * LPSCI LPSCI Status Flags
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_GetStatusFlag
 * Description   : Get LPSCI status flag states.
 *
 *END**************************************************************************/
bool LPSCI_HAL_GetStatusFlag(uint32_t baseAddr, lpsci_status_flag_t statusFlag)
{
    uint8_t reg = (uint32_t)statusFlag >> LPSCI_SHIFT;
    uint8_t temp = 0;

    switch ( reg )
    {
        case 0 :
            temp = HW_UART0_S1_RD(baseAddr) >> (uint8_t)(statusFlag) & 1U;
            break;
        case 1 :
            temp = HW_UART0_S2_RD(baseAddr) >> (uint8_t)(statusFlag) & 1U;
            break;
#if FSL_FEATURE_LPSCI_HAS_EXTENDED_DATA_REGISTER_FLAGS
        case 2 :
            temp = HW_UART0_ED_RD(baseAddr) >> (uint8_t)(statusFlag) & 1U;
            break;
#endif
        default :
            break;
    }
    return (bool)temp;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_ClearStatusFlag
 * Description   : Clear an individual and specific LPSCI status flag.
 * This function allows the user to clear an individual and specific LPSCI
 * status flag. Refer to structure definition lpsci_status_flag_t for list of
 * status bits.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_HAL_ClearStatusFlag(uint32_t baseAddr,
                                         lpsci_status_flag_t statusFlag)
{
    lpsci_status_t returnCode = kStatus_LPSCI_Success;

    switch(statusFlag)
    {
        /* These flags are cleared automatically by other lpuart operations
         * and cannot be manually cleared, return error code */
        case kLpsciTxDataRegEmpty:
        case kLpsciTxComplete:
        case kLpsciRxDataRegFull:
        case kLpsciRxActive:
 #if FSL_FEATURE_LPSCI_HAS_EXTENDED_DATA_REGISTER_FLAGS
        case kLpsciNoiseInCurrentWord:
        case kLpsciParityErrInCurrentWord:
#endif
            returnCode = kStatus_LPSCI_ClearStatusFlagError;
            break;

        case kLpsciIdleLineDetect:
        case kLpsciRxOverrun:
        case kLpsciNoiseDetect:
        case kLpsciFrameErr:
        case kLpsciParityErr:
            HW_UART0_S1_RD(baseAddr);
            HW_UART0_D_RD(baseAddr);
            break;

        case kLpsciLineBreakDetect:
            BW_UART0_S2_LBKDIF(baseAddr, 0x1U);
            break;

        case kLpsciRxActiveEdgeDetect:
            BW_UART0_S2_RXEDGIF(baseAddr, 0x1U);
            break;

        default:
            returnCode = kStatus_LPSCI_ClearStatusFlagError;
            break;
    }

    return (returnCode);
}

/*******************************************************************************
 * LPSCI Special Feature Configurations
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_PutReceiverInStandbyMode
 * Description   : Place the LPSCI receiver in standby mode.
 * This function, when called, will place the LPSCI receiver into standby mode.
 * In some LPSCI, there is a condition that must be met before placing rx in
 * standby mode. Before placing LPSCI in standby, you need to first determine
 * if receiver is set to wake on idle and if receiver is already in idle state.
 *
 *END**************************************************************************/
lpsci_status_t LPSCI_HAL_PutReceiverInStandbyMode(uint32_t baseAddr)
{
    lpsci_wakeup_method_t rxWakeMethod;
    bool lpsci_current_rx_state;

    /* see if wake is set for idle or */
    rxWakeMethod = LPSCI_HAL_GetReceiverWakeupMethod(baseAddr);
    lpsci_current_rx_state = LPSCI_HAL_GetStatusFlag(baseAddr, kLpsciRxActive);

    /* if both rxWakeMethod is set for idle and current rx state is idle,
     * don't put in standy*/
    if ((rxWakeMethod == kLpsciIdleLineWake) && (lpsci_current_rx_state == 0))
    {
        return kStatus_LPSCI_RxStandbyModeError;
    }
    else
    {
        /* set the RWU bit to place receiver into standby mode*/
        HW_UART0_C2_SET(baseAddr, BM_UART0_C2_RWU);
        return kStatus_LPSCI_Success;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_ConfigIdleLineDetect
 * Description   : Configure the operation options of the LPSCI idle line detect.
 * This function allows the user to configure the LPSCI idle-line detect
 * operation. There are two separate operations for the user to configure, the
 * idle line bit-count start and the receive wake up affect on IDLE status bit.
 * The user will pass in a stucture of type lpsci_idle_line_config_t.
 *
 *END**************************************************************************/
void LPSCI_HAL_ConfigIdleLineDetect(uint32_t baseAddr,
                                    uint8_t idleLine,
                                    uint8_t rxWakeIdleDetect)
{
    /* Configure the idle line detection configuration as follows:
     * configure the ILT to bit count after start bit or stop bit
     * configure RWUID to set or not set IDLE status bit upon detection of
     * an idle character when recevier in standby */
    BW_UART0_C1_ILT(baseAddr, idleLine);
    BW_UART0_S2_RWUID(baseAddr, rxWakeIdleDetect);
}

#if FSL_FEATURE_LPSCI_HAS_ADDRESS_MATCHING
/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_SetMatchAddress
 * Description   : Configure the LPSCI match address mode control operation. The
 * function allows the user to configure the LPSCI match address control
 * operation. The user has the option to enable the match address mode and to
 * program the match address value. There are two match address modes, each with
 * it's own enable and programmable match address value.
 *
 *END**************************************************************************/
void LPSCI_HAL_SetMatchAddress(uint32_t baseAddr,
                               bool matchAddrMode1,
                               bool matchAddrMode2,
                               uint8_t matchAddrValue1,
                               uint8_t matchAddrValue2)
{
    /* Match Address Mode Enable 1 */
    BW_UART0_C4_MAEN1(baseAddr, matchAddrMode1);
    /* Match Address Mode Enable 2 */
    BW_UART0_C4_MAEN2(baseAddr, matchAddrMode2);
    /* match address register 1 */
    HW_UART0_MA1_WR(baseAddr, matchAddrValue1);
    /* match address register 2 */
    HW_UART0_MA2_WR(baseAddr, matchAddrValue2);
}
#endif

#if FSL_FEATURE_LPSCI_HAS_IR_SUPPORT
/*FUNCTION**********************************************************************
 *
 * Function Name : LPSCI_HAL_SetInfraredOperation
 * Description   : Configure the LPSCI infrared operation.
 * The function allows the user to enable or disable the LPSCI infrared (IR)
 * operation and to configure the IR pulse width.
 *
 *END**************************************************************************/
void LPSCI_HAL_SetInfraredOperation(uint32_t baseAddr,
                                    bool enable,
                                    lpsci_ir_tx_pulsewidth_t pulseWidth)
{
    /* enable or disable infrared */
    BW_UART0_IR_IREN(baseAddr, enable);
    /* configure the narrow pulse width of the IR pulse */
    BW_UART0_IR_TNP(baseAddr, pulseWidth);
}
#endif  /* FSL_FEATURE_LPSCI_HAS_IR_SUPPORT */

/*******************************************************************************
 * EOF
 ******************************************************************************/

