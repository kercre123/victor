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
#include "fsl_lpuart_hal.h"

/*******************************************************************************
 * Code
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_Init
 * Description   : Initializes the LPUART controller to known state.
 *
 *END**************************************************************************/
void LPUART_HAL_Init(uint32_t baseAddr)
{
    HW_LPUART_BAUD_WR(baseAddr, 0x0F000004);
    HW_LPUART_STAT_WR(baseAddr, 0xC01FC000);
    HW_LPUART_CTRL_WR(baseAddr, 0x00000000);
    HW_LPUART_MATCH_WR(baseAddr, 0x00000000);
#if FSL_FEATURE_LPUART_HAS_MODEM_SUPPORT
    HW_LPUART_MODIR_WR(baseAddr, 0x00000000);
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetBaudRate
 * Description   : Configures the LPUART baud rate.
 * In some LPUART instances the user must disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 *END**************************************************************************/
lpuart_status_t LPUART_HAL_SetBaudRate(uint32_t baseAddr,
                                       uint32_t sourceClockInHz,
                                       uint32_t desiredBaudRate)
{
    uint16_t sbr, sbrTemp, i;
    uint32_t osr, tempDiff, calculatedBaud, baudDiff;

    /* This lpuart instantiation uses a slightly different baud rate calculation
     * The idea is to use the best OSR (over-sampling rate) possible
     * Note, osr is typically hard-set to 16 in other lpuart instantiations
     * First calculate the baud rate using the minimum OSR possible (4) */
    osr = 4;
    sbr = (sourceClockInHz/(desiredBaudRate * osr));
    calculatedBaud = (sourceClockInHz / (osr * sbr));

    if (calculatedBaud > desiredBaudRate)
    {
        baudDiff = calculatedBaud - desiredBaudRate;
    }
    else
    {
        baudDiff = desiredBaudRate - calculatedBaud;
    }

    /* loop to find the best osr value possible, one that generates minimum baudDiff
     * iterate through the rest of the supported values of osr */
    for (i = 5; i <= 32; i++)
    {
        /* calculate the temporary sbr value   */
        sbrTemp = (sourceClockInHz/(desiredBaudRate * i));
        /* calculate the baud rate based on the temporary osr and sbr values */
        calculatedBaud = (sourceClockInHz / (i * sbrTemp));

        if (calculatedBaud > desiredBaudRate)
        {
            tempDiff = calculatedBaud - desiredBaudRate;
        }
        else
        {
            tempDiff = desiredBaudRate - calculatedBaud;
        }

        if (tempDiff <= baudDiff)
        {
            baudDiff = tempDiff;
            osr = i;  /* update and store the best osr value calculated */
            sbr = sbrTemp;  /* update store the best sbr value calculated */
        }
    }

    /* Check to see if actual baud rate is within 3% of desired baud rate
     * based on the best calculate osr value */
    if (baudDiff < ((desiredBaudRate / 100) * 3))
    {
        /* Acceptable baud rate, check if osr is between 4x and 7x oversampling.
         * If so, then "BOTHEDGE" sampling must be turned on */
        if ((osr > 3) && (osr < 8))
        {
            BW_LPUART_BAUD_BOTHEDGE(baseAddr, 1);
        }

        /* program the osr value (bit value is one less than actual value) */
        BW_LPUART_BAUD_OSR(baseAddr, (osr-1));

        /* write the sbr value to the BAUD registers */
        BW_LPUART_BAUD_SBR(baseAddr, sbr);
    }
    else
    {
        /* Unacceptable baud rate difference of more than 3% */
        return kStatus_LPUART_BaudRateCalculationError;
    }

    return kStatus_LPUART_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetBitCountPerChar
 * Description   : Configures the number of bits per char in LPUART controller.
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 *END**************************************************************************/
void LPUART_HAL_SetBitCountPerChar(uint32_t baseAddr,
                                   lpuart_bit_count_per_char_t bitCountPerChar)
{
    if (bitCountPerChar == kLpuart10BitsPerChar)
    {
        BW_LPUART_BAUD_M10(baseAddr, 0x1U);
    }
    else
    {
        /* config 8-bit (M=0) or 9-bits (M=1) */
        BW_LPUART_CTRL_M(baseAddr, bitCountPerChar);
        /* clear M10 to make sure not 10-bit mode */
        BW_LPUART_BAUD_M10(baseAddr, 0x0U);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetParityMode
 * Description   : Configures parity mode in the LPUART controller.
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 *END**************************************************************************/
void LPUART_HAL_SetParityMode(uint32_t baseAddr, lpuart_parity_mode_t parityModeType)
{
    BW_LPUART_CTRL_PE(baseAddr, parityModeType >> 1U);
    BW_LPUART_CTRL_PT(baseAddr, parityModeType & 1U);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_Putchar9
 * Description   : Sends the LPUART 9-bit character.
 *
 *END**************************************************************************/
void LPUART_HAL_Putchar9(uint32_t baseAddr, uint16_t data)
{
    uint8_t ninthDataBit;

    ninthDataBit = (data >> 8U) & 0x1U;

    /* write to ninth data bit T8(where T[0:7]=8-bits, T8=9th bit) */
    BW_LPUART_CTRL_R9T8(baseAddr, ninthDataBit);

    /* write 8-bits to the data register*/
    HW_LPUART_DATA_WR(baseAddr, (uint8_t)data);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_Putchar10
 * Description   : Sends the LPUART 10-bit character.
 *
 *END**************************************************************************/
lpuart_status_t LPUART_HAL_Putchar10(uint32_t baseAddr, uint16_t data)
{
    uint8_t ninthDataBit, tenthDataBit;

    ninthDataBit = (data >> 8U) & 0x1U;
    tenthDataBit = (data >> 9U) & 0x1U;

    /* write to ninth/tenth data bit (T[0:7]=8-bits, T8=9th bit, T9=10th bit) */
    BW_LPUART_CTRL_R9T8(baseAddr, ninthDataBit);
    BW_LPUART_CTRL_R8T9(baseAddr, tenthDataBit);

    /* write to 8-bits to the data register */
    HW_LPUART_DATA_WR(baseAddr, (uint8_t)data);

    return kStatus_LPUART_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_Getchar9
 * Description   : Gets the LPUART 9-bit character.
 *
 *END**************************************************************************/
void LPUART_HAL_Getchar9(uint32_t baseAddr, uint16_t *readData)
{
    /* get ninth bit from lpuart data register */
    *readData = (uint16_t)BR_LPUART_CTRL_R8T9(baseAddr) << 8;

    /* get 8-bit data from the lpuart data register */
    *readData |= (uint8_t)HW_LPUART_DATA_RD(baseAddr);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_Getchar10
 * Description   : Gets the LPUART 10-bit character, available only on
 *                 supported lpuarts
 *
 *END**************************************************************************/
void LPUART_HAL_Getchar10(uint32_t baseAddr, uint16_t *readData)
{
    /* read tenth data bit */
    *readData = (uint16_t)((uint32_t)(BR_LPUART_CTRL_R9T8(baseAddr)) << 9U);

    /* read ninth data bit */
    *readData |= (uint16_t)((uint32_t)(BR_LPUART_CTRL_R8T9(baseAddr)) << 8U);

    /* get 8-bit data */
    *readData |= HW_LPUART_DATA_RD(baseAddr);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SendDataPolling
 * Description   : Send out multiple bytes of data using polling method.
 * This function only supports 8-bit transaction.
 *
 *END**************************************************************************/
void LPUART_HAL_SendDataPolling(uint32_t baseAddr,
                                const uint8_t *txBuff,
                                uint32_t txSize)
{
    while (txSize--)
    {
        while (!LPUART_HAL_IsTxDataRegEmpty(baseAddr))
        {}

        LPUART_HAL_Putchar(baseAddr, *txBuff++);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_ReceiveDataPolling
 * Description   : Receive multiple bytes of data using polling method.
 * This function only supports 8-bit transaction.
 *
 *END**************************************************************************/
lpuart_status_t LPUART_HAL_ReceiveDataPolling(uint32_t baseAddr,
                                              uint8_t *rxBuff,
                                              uint32_t rxSize)
{
    lpuart_status_t retVal = kStatus_LPUART_Success;

    while (rxSize--)
    {
        while (!LPUART_HAL_IsRxDataRegFull(baseAddr))
        {}

        LPUART_HAL_Getchar(baseAddr, rxBuff++);

        /* Clear the Overrun flag since it will block receiving */
        if (BR_LPUART_STAT_OR(baseAddr))
        {
            BW_LPUART_STAT_OR(baseAddr, 1U);
            retVal = kStatus_LPUART_RxOverRun;
        }
    }

    return retVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetIntMode
 * Description   : Configures the LPUART module interrupts to enable/disable
 * various interrupt sources.
 *
 *END**************************************************************************/
void LPUART_HAL_SetIntMode(uint32_t baseAddr, lpuart_interrupt_t interrupt, bool enable)
{
    uint32_t reg = (uint32_t)(interrupt) >> LPUART_SHIFT;
    uint32_t temp = 1U << (uint32_t)interrupt;

    switch ( reg )
    {
        case LPUART_BAUD_REG_ID:
            enable ? HW_LPUART_BAUD_SET(baseAddr, temp) : HW_LPUART_BAUD_CLR(baseAddr, temp);
            break;
        case LPUART_STAT_REG_ID:
            enable ? HW_LPUART_STAT_SET(baseAddr, temp) : HW_LPUART_STAT_CLR(baseAddr, temp);
            break;
        case LPUART_CTRL_REG_ID:
            enable ? HW_LPUART_CTRL_SET(baseAddr, temp) : HW_LPUART_CTRL_CLR(baseAddr, temp);
            break;
        case LPUART_DATA_REG_ID:
            enable ? HW_LPUART_DATA_SET(baseAddr, temp) : HW_LPUART_DATA_CLR(baseAddr, temp);
            break;
        case LPUART_MATCH_REG_ID:
            enable ? HW_LPUART_MATCH_SET(baseAddr, temp) : HW_LPUART_MATCH_CLR(baseAddr, temp);
            break;
#if FSL_FEATURE_LPUART_HAS_MODEM_SUPPORT
        case LPUART_MODIR_REG_ID:
            enable ? HW_LPUART_MODIR_SET(baseAddr, temp) : HW_LPUART_MODIR_CLR(baseAddr, temp);
            break;
#endif
        default :
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_GetIntMode
 * Description   : Returns whether LPUART module interrupts is enabled/disabled.
 *
 *END**************************************************************************/
bool LPUART_HAL_GetIntMode(uint32_t baseAddr, lpuart_interrupt_t interrupt)
{
    uint32_t reg = (uint32_t)(interrupt) >> LPUART_SHIFT;
	  bool retVal = false;

    switch ( reg )
    {
        case LPUART_BAUD_REG_ID:
            retVal = HW_LPUART_BAUD_RD(baseAddr) >> (uint32_t)(interrupt) & 1U;
            break;
        case LPUART_STAT_REG_ID:
            retVal = HW_LPUART_STAT_RD(baseAddr) >> (uint32_t)(interrupt) & 1U;
            break;
        case LPUART_CTRL_REG_ID:
            retVal = HW_LPUART_CTRL_RD(baseAddr) >> (uint32_t)(interrupt) & 1U;
            break;
        case LPUART_DATA_REG_ID:
            retVal = HW_LPUART_DATA_RD(baseAddr) >> (uint32_t)(interrupt) & 1U;
            break;
        case LPUART_MATCH_REG_ID:
            retVal = HW_LPUART_MATCH_RD(baseAddr) >> (uint32_t)(interrupt) & 1U;
            break;
#if FSL_FEATURE_LPUART_HAS_MODEM_SUPPORT
        case LPUART_MODIR_REG_ID:
            retVal = HW_LPUART_MODIR_RD(baseAddr) >> (uint32_t)(interrupt) & 1U;
            break;
#endif
        default :
            break;
    }

    return retVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetLoopbackCmd
 * Description   : Configures the LPUART loopback operation (enable/disable
 *                 loopback operation)
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 *END**************************************************************************/
void LPUART_HAL_SetLoopbackCmd(uint32_t baseAddr, bool enable)
{
    /* configure LOOPS bit to enable(1)/disable(0) loopback mode, but also need
     * to clear RSRC */
    BW_LPUART_CTRL_LOOPS(baseAddr, enable);

    /* clear RSRC for loopback mode, and if loopback disabled, */
    /* this bit has no meaning but clear anyway */
    /* to set it back to default value */
    BW_LPUART_CTRL_RSRC(baseAddr, 0);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetSingleWireCmd
 * Description   : Configures the LPUART single-wire operation (enable/disable
 *                 single-wire mode)
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 *END**************************************************************************/
void LPUART_HAL_SetSingleWireCmd(uint32_t baseAddr, bool enable)
{
    /* to enable single-wire mode, need both LOOPS and RSRC set,
     * to enable or clear both */
    BW_LPUART_CTRL_LOOPS(baseAddr, enable);
    BW_LPUART_CTRL_RSRC(baseAddr, enable);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetReceiverInStandbyMode
 * Description   : Places the LPUART receiver in standby mode.
 * In some LPUART instances, before placing LPUART in standby mode, first
 * determine whether the receiver is set to wake on idle or whether it is
 * already in idle state.
 *
 *END**************************************************************************/
lpuart_status_t LPUART_HAL_SetReceiverInStandbyMode(uint32_t baseAddr)
{
    lpuart_wakeup_method_t rxWakeMethod;
    bool lpuart_current_rx_state;

    rxWakeMethod = LPUART_HAL_GetReceiverWakeupMode(baseAddr);
    lpuart_current_rx_state = LPUART_HAL_GetStatusFlag(baseAddr, kLpuartRxActive);

    /* if both rxWakeMethod is set for idle and current rx state is idle,
     * don't put in standby */
    if ((rxWakeMethod == kLpuartIdleLineWake) && (lpuart_current_rx_state == 0))
    {
        return kStatus_LPUART_RxStandbyModeError;
    }
    else
    {
        /* set the RWU bit to place receiver into standby mode */
        BW_LPUART_CTRL_RWU(baseAddr, 1);
        return kStatus_LPUART_Success;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetIdleLineDetect
 * Description   : LPUART idle-line detect operation configuration (idle line
 * bit-count start and wake up affect on IDLE status bit).
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 *END**************************************************************************/
void LPUART_HAL_SetIdleLineDetect(uint32_t baseAddr,
                                  const lpuart_idle_line_config_t *config)
{
    /* Configure the idle line detection configuration as follows:
     * configure the ILT to bit count after start bit or stop bit
     * configure RWUID to set or not set IDLE status bit upon detection of
     * an idle character when receiver in standby */
    BW_LPUART_CTRL_ILT(baseAddr, config->idleLineType);
    BW_LPUART_STAT_RWUID(baseAddr, config->rxWakeIdleDetect);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetMatchAddressReg1
 * Description   : Configures match address register 1
 *
 *END**************************************************************************/
void LPUART_HAL_SetMatchAddressReg1(uint32_t baseAddr, bool enable, uint8_t value)
{
    /* The MAEN bit must be cleared before configuring MA value */
    BW_LPUART_BAUD_MAEN1(baseAddr, 0x0U);
    if (enable)
    {
        BW_LPUART_MATCH_MA1(baseAddr, value);
        BW_LPUART_BAUD_MAEN1(baseAddr, 0x1U);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetMatchAddressReg2
 * Description   : Configures match address register 2
 *
 *END**************************************************************************/
void LPUART_HAL_SetMatchAddressReg2(uint32_t baseAddr, bool enable, uint8_t value)
{
    /* The MAEN bit must be cleared before configuring MA value */
    BW_LPUART_BAUD_MAEN2(baseAddr, 0x0U);
    if (enable)
    {
        BW_LPUART_MATCH_MA2(baseAddr, value);
        BW_LPUART_BAUD_MAEN2(baseAddr, 0x1U);
    }
}

#if FSL_FEATURE_LPUART_HAS_IR_SUPPORT
/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_SetInfrared
 * Description   : Configures the LPUART infrared operation.
 *
 *END**************************************************************************/
void LPUART_HAL_SetInfrared(uint32_t baseAddr, bool enable,
                            lpuart_ir_tx_pulsewidth_t pulseWidth)
{
    /* enable or disable infrared */
    BW_LPUART_MODIR_IREN(baseAddr, enable);

    /* configure the narrow pulse width of the IR pulse */
    BW_LPUART_MODIR_TNP(baseAddr, pulseWidth);
}
#endif  /* FSL_FEATURE_LPUART_HAS_IR_SUPPORT */

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_GetStatusFlag
 * Description   : LPUART get status flag by passing flag enum.
 *
 *END**************************************************************************/
bool LPUART_HAL_GetStatusFlag(uint32_t baseAddr, lpuart_status_flag_t statusFlag)
{
    uint32_t reg = (uint32_t)(statusFlag) >> LPUART_SHIFT;
	bool retVal = false;

    switch ( reg )
    {
        case LPUART_BAUD_REG_ID:
            retVal = HW_LPUART_BAUD_RD(baseAddr) >> (uint32_t)(statusFlag) & 1U;
            break;
        case LPUART_STAT_REG_ID:
            retVal = HW_LPUART_STAT_RD(baseAddr) >> (uint32_t)(statusFlag) & 1U;
            break;
        case LPUART_CTRL_REG_ID:
            retVal = HW_LPUART_CTRL_RD(baseAddr) >> (uint32_t)(statusFlag) & 1U;
            break;
        case LPUART_DATA_REG_ID:
            retVal = HW_LPUART_DATA_RD(baseAddr) >> (uint32_t)(statusFlag) & 1U;
            break;
        case LPUART_MATCH_REG_ID:
            retVal = HW_LPUART_MATCH_RD(baseAddr) >> (uint32_t)(statusFlag) & 1U;
            break;
#if FSL_FEATURE_LPUART_HAS_MODEM_SUPPORT
        case LPUART_MODIR_REG_ID:
            retVal = HW_LPUART_MODIR_RD(baseAddr) >> (uint32_t)(statusFlag) & 1U;
            break;
#endif
        default:
            break;
    }

    return retVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LPUART_HAL_ClearStatusFlag
 * Description   : LPUART clears an individual status flag
 * (see lpuart_status_flag_t for list of status bits).
 *
 *END**************************************************************************/
lpuart_status_t LPUART_HAL_ClearStatusFlag(uint32_t baseAddr,
                                           lpuart_status_flag_t statusFlag)
{
    lpuart_status_t returnCode = kStatus_LPUART_Success;

    switch(statusFlag)
    {
        /* These flags are cleared automatically by other lpuart operations
         * and cannot be manually cleared, return error code */
        case kLpuartTxDataRegEmpty:
        case kLpuartTxComplete:
        case kLpuartRxDataRegFull:
        case kLpuartRxActive:
#if FSL_FEATURE_LPUART_HAS_EXTENDED_DATA_REGISTER_FLAGS
        case kLpuartNoiseInCurrentWord:
        case kLpuartParityErrInCurrentWord:
#endif
            returnCode = kStatus_LPUART_ClearStatusFlagError;
            break;

        case kLpuartIdleLineDetect:
            BW_LPUART_STAT_IDLE(baseAddr, 0x1U);
            break;

        case kLpuartRxOverrun:
            BW_LPUART_STAT_OR(baseAddr, 0x1U);
            break;

        case kLpuartNoiseDetect:
            BW_LPUART_STAT_NF(baseAddr, 0x1U);
            break;

        case kLpuartFrameErr:
            BW_LPUART_STAT_FE(baseAddr, 0x1U);
            break;

        case kLpuartParityErr:
            BW_LPUART_STAT_PF(baseAddr, 0x1U);
            break;

        case kLpuartLineBreakDetect:
            BW_LPUART_STAT_LBKDIF(baseAddr, 0x1U);
            break;

        case kLpuartRxActiveEdgeDetect:
            BW_LPUART_STAT_RXEDGIF(baseAddr, 0x1U);
            break;

#if FSL_FEATURE_LPUART_HAS_ADDRESS_MATCHING
        case kLpuartMatchAddrOne:
            BW_LPUART_STAT_MA1F(baseAddr, 0x1U);
            break;
        case kLpuartMatchAddrTwo:
            BW_LPUART_STAT_MA2F(baseAddr, 0x1U);
            break;
#endif
        default:
            returnCode = kStatus_LPUART_ClearStatusFlagError;
            break;
    }

    return (returnCode);
}

/*******************************************************************************
 * EOF
 ******************************************************************************/

