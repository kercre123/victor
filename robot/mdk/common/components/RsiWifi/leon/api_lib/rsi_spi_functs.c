/**
 * @file
 * @version		2.0.0.0
 * @date 		2011-Feb-25
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief  SPI bus Layer function used to transfer spi protocol level commands to module.
 *         For more detail refer PRM for spi commands
 *
 */

/*
 * Includes
 */
#include "rsi_global.h"

#include <stdio.h>

/**
 * Global Variables
 */

/*==================================================*/
/**
 * @fn          int16 rsi_sendC1C2(uint8 c1, uint8 c2)
 * @brief       Sends the C1 & C2 commands bytes, should check response for C1 command.Incase of busy should retry.
 * @param[in]   uint8 c1 SPI c1 command.
 * @param[in]   uint8 c2 SPI c2 command
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *             	-2 = SPI Failure
 *              0  = SUCCESS
 */
int16 rsi_sendC1C2(uint8 c1, uint8 c2)
{
    int16 retval;
    uint16 timeout;
    uint8 txCmd[2];
    uint8 localBuf[4];

    timeout = 1 * RSI_TICKS_PER_SECOND;
    /* reset the timeout timer to 0; */RSI_RESET_TIMER1;
    while (1)
    {
        txCmd[0] = c1;
        txCmd[1] = c2;
        if (RSI_INC_TIMER_1>timeout)
        {
            retval = RSI_BUSY;
#ifdef RSI_DEBUG_PRINT
            RSI_DPRINT(RSI_PL8, "\r\n C1C2 command Timeout");
#endif
            break;
        }
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL7, " C1C2=");
#endif
        /* Send C1 */
        retval = rsi_spiSend(&txCmd[0], 1, localBuf, RSI_MODE_8BIT);
        /* Send C2 */
        retval = rsi_spiSend(&txCmd[1], 1, localBuf, RSI_MODE_8BIT);
        if (localBuf[0] == RSI_SPI_SUCCESS)
        {
            /* success, so return now */
            retval = RSI_SUCCESS;
            break;
        }
        else if (localBuf[0] == RSI_SPI_FAIL)
        {
#ifdef RSI_DEBUG_PRINT
            RSI_DPRINT(RSI_PL8, "\r\n C1C2 command Failed");
#endif
            retval = RSI_FAIL;
            break;
        }
        else if (localBuf[0] == RSI_SPI_BUSY)
        {
            /* Busy, retry once again */
            retval = RSI_BUSY;
        }
    }

    return retval;
}

/*==================================================*/
/**
 * @fn          int16 rsi_sendC3C4(uint8 c3, uint8 c4)
 * @brief       Sends the C3/C4 bytes
 * @param[in]   uint8 c3 SPI c3 command bytes to be sent
 * @param[in]   uint8 c4 SPI c4 command bytes to be sent
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @prerequisite rsi_sendC1C2 should successful before this function.
 */
int16 rsi_sendC3C4(uint8 c3, uint8 c4)
{
    int16 retval;
    uint8 txCmd[2];    // command buffer
    uint8 localBuf[4];

    txCmd[0] = c3;
    txCmd[1] = c4;
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL15, " C3C4=");
#endif
    /*command should send only 8 bit mode */
    retval = rsi_spiSend(txCmd, (uint16) sizeof(txCmd), localBuf, RSI_MODE_8BIT);
    return retval;
}

/*==================================================*/
/**
 * @fn		int16 rsi_spiWaitStartToken(uint32 timeout,uint8 mode)
 * @brief	Loops reading the SPI until a start token, 0x55, is received
 * @param[in]	uint32  timeout   Timeout for start token.
 * @param[in]	uint8   mode      To indicate 8bit/32bit mode.
 * @param[out]	none
 * @return	errCode
 *		0 = success
 *	       -1 = busy/timeout failure
 * @section prerequisite 
 * should issue read commands before this function.
 */
int16 rsi_spiWaitStartToken(uint32 timeout, uint8 mode)
{
    int16 retval;
#ifdef RSI_BIT_32_SUPPORT
    uint32 txChar;	      // char to send/receive data in
#else
    uint8 txChar;
#endif
    // Look for start token
    // Send a character, could be any character, and check the response for a start token
    // If we don't find it within the timeout time, error out
    // Timeout value needs to be passed since context is important
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL8, "\r\nWaitStartToken=");
#endif
    RSI_RESET_TIMER1;		     // reset the timeout timer to 0;
    while (1)
    {
        if (RSI_INC_TIMER_1>timeout)
        {
            retval = RSI_BUSY;
            /* timeout */
            break;
        }

        txChar = 0x00;
#ifdef RSI_BIT_32_SUPPORT
        if( mode == RSI_MODE_8BIT)
#endif
        retval = rsi_spiRecv(&txChar, 1, mode);
#ifdef RSI_BIT_32_SUPPORT	
        else
        retval = rsi_spiRecv(&txChar, 4, mode);
#endif    
        if (txChar == RSI_SPI_START_TOKEN)
        {
            /* found the start token */
            retval = RSI_SUCCESS;
            break;
        }
    }
    return retval;
}

/*==================================================*/
/**
 * @fn          int16 rsi_set_intr_mask(uint8 interruptMask)
 * @brief       Sets the INTERRUPT MASK REGISTER of the Wi-Fi module
 * @param[in]   uint8 interruptMask, the value to set the mask register to
 * @param[out]  none
 * @return      errorcode 
 *              -1 = busy/timeout failure
 *              -2 = spi failure  
 */
int16 rsi_set_intr_mask(uint8 interruptMask)
{
    int16 retval;
    int16 i;
#ifdef RSI_BIT_32_SUPPORT
    uint8 dBuf[4]=
    {   0,0,0,0};
#else
    uint8 dBuf[2] = { 0, 0 };
#endif
    dBuf[0] = interruptMask;

    // memory write the mask value
    retval = rsi_memWr(RSI_INT_MASK_REG_ADDR, sizeof(dBuf), dBuf);    // 0x74, 0x00, 0x22000008, 0x04, interruptMask

    return retval;
}

/*==================================================*/
/**
 * @fn          int16 rsi_clear_interrupt(uint8 interruptClear)
 * @brief       Clears the interrupt register
 * @param[in]   uint8 interrutClear, the value to set the interrupt clear register to
 * @param[out]	none
 * @return      errorcode 
 *              -1 = busy/timeout failure
 *              -2 = spi failure  
 */
int16 rsi_clear_interrupt(uint8 interruptClear)
{
    int16 retval;
#ifdef RSI_BIT_32_SUPPORT
    uint8 dBuf[4]=
    {   0,0,0,0};
#else
    uint8 dBuf[2] = { 0, 0 };
#endif

    // read in the register so we can set bit 5 and write back out
    retval = rsi_memRd(RSI_INT_CLR_REG_ADDR, sizeof(dBuf), dBuf);
    // 0x74, 0x00, 0x22000010
    // set bit 5, interrupt clear
    dBuf[0] |= interruptClear;
    // memory write the mask value
    retval = rsi_memWr(RSI_INT_CLR_REG_ADDR, sizeof(dBuf), dBuf);
    // 0x74, 0x00, 0x22000010

    return retval;
}

/*==================================================*/
/**
 * @fn          int16 rsi_irqstatus(void)
 * @brief       Returns the value of the Interrupt register
 * @param[in]   none
 * @param[out]  none
 * @return      value of the interrupt status register in dBuf[0] 
 */
int16 rsi_irqstatus(void)
{
    int16 retval;
    uint16 timeout;
    uint8 dBuf[2] = { 0 };

    timeout = 1 * RSI_TICKS_PER_SECOND;

    RSI_RESET_TIMER2;
    while (1)
    {
        if (RSI_INC_TIMER_2 > timeout)
        {
            retval = RSI_BUSY;    // timeout
#ifdef RSI_DEBUG_PRINT
            RSI_DPRINT(RSI_PL3, "\r\nError Readng Interrupt Status Register = Timeout");
#endif
            break;			 // retry some more
        }

        // read the interrupt register, dBuf[0], results are returned in dBuf, addr=0x00
        retval = rsi_regRd(RSI_SPI_INT_REG_ADDR, dBuf);

        if (retval >= 0)
        {
            retval = (int16) dBuf[0];    // return the value of the interrupt register
            break;
        }
    }

    return retval;
}

/*==================================================*/
/**
 * @fn          void rsi_buildFrameDescriptor(rsi_uFrameDsc *uFrameDscFrame, uint8 *cmd)
 * @brief       Creates a Frame Descriptor
 * @param[in]   rsi_uFrameDsc *uFrameDscFrame,Frame Descriptor
 * @param[in]   uint8 *cmd,Indicates type of the packet(data or management)
 * @param[out]  none
 * @return      none
 */
void rsi_buildFrameDescriptor(rsi_uFrameDsc *uFrameDscFrame, uint8 *cmd)
{
    uint8 i;
    for (i = 0; i < RSI_FRAME_DESC_LEN; i++)
    {
        uFrameDscFrame->uFrmDscBuf[i] = 0;
    }    // zero the frame descriptor buffer
    // data or management frame type
    uFrameDscFrame->uFrmDscBuf[14] = cmd[2];
    // The first two bytes have different functions for management frames and
    // control frames, but these are set in the pre-defined
    uFrameDscFrame->uFrmDscBuf[0] = cmd[0];
    // arrays which are the argument passed to this function, so we just set the two values
    uFrameDscFrame->uFrmDscBuf[1] = cmd[1];

    return;
}

