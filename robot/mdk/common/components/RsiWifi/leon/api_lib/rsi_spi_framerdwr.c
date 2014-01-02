/**
 * @file
 * @version		2.2.0.0
 * @date 		2011-May-30
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief SPI, control frame read/write functions
 *
 * @section Description
 * SPI function to read/write management descriptors frames to/from the Wi-Fi module
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
extern void (*gCallBack)();

/*===========================================================================*/
/**
 * @fn          int16 rsi_SpiFrameDscRd(uFrameDsc *uFrameDscFrame)
 * @brief       Reads frame descriptor from module.
 * @param[in]   rsi_uFrameDsc *uFrameDscFrame, Pointer to frame descriptor
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This function is used to read the descriptor of packet received from the module. 
 */
int16 rsi_SpiFrameDscRd(rsi_uFrameDsc *uFrameDscFrame)
{
    int16 retval;
    uint8 c1;
    uint8 c2;
    uint8 c3;
    uint8 c4;

    c1 = RSI_C1FRMRD16BIT4BYTE;					// 0x5c
#ifdef RSI_BIT_32_SUPPORT
            c2 = RSI_C2SPIADDR4BYTE;                                        // 0x40
#else
    c2 = RSI_C2MEMRDWRNOCARE;					// 0x00
#endif
    c3 = RSI_FRAME_DESC_LEN;				        // command frame response descriptor
    c4 = 0x00;						        // upper bye of transfer length
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5, " CFrameDscRd=");
#endif
    // Send C1/C2
    retval = rsi_sendC1C2(c1, c2);
    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL6, " Desc Read C1C2 Failed");
#endif
        return retval;			// exit with error if we timed out waiting for the SPI to get ready
    }

    // Send C3/C4
    retval = rsi_sendC3C4(c3, c4);

    // Wait for start token
    retval = rsi_spiWaitStartToken(RSI_START_TOKEN_TIMEOUT, RSI_MODE_32BIT);
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL6, " CFrameDscRd=");
#endif
    retval = rsi_spiRecv((uint8 *) &uFrameDscFrame->uFrmDscBuf, RSI_FRAME_DESC_LEN, RSI_MODE_32BIT);
    // read the command frame descriptor
    return retval;							// return the control frame length
}

/*===========================================================================*/
/**
 * @fn          int16 rsi_SpiFrameDscWr(rsi_uFrameDsc *uFrameDscFramee,uint8 type)
 * @brief       Writes a Frame descriptor
 * @param[in]   rsi_uFrameDsc *uFrameDscFrame, pointer to frame descriptor
 * @param[in]   uint8 type, To indicate the type whether it is DATA or MGMT
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This function performs Frame Decriptor Write. 
 */
int16 rsi_SpiFrameDscWr(rsi_uFrameDsc *uFrameDscFrame, uint8 type)
{
    int16 retval;
    uint8 c1;
    uint8 c2;
    uint8 c3;
    uint8 c4;
    uint8 localBuf[4];

    c1 = RSI_C1FRMWR16BIT4BYTE;				// 0x7c
#ifdef RSI_BIT_32_SUPPORT
            c2 = RSI_C2RDWR4BYTE | type;                            // 0x42
#else
    c2 = RSI_C2RDWR1BYTE | type;				// 0x02
#endif  
    c3 = RSI_FRAME_DESC_LEN;				// frame descriptor is 16 bytes long
    c4 = 0x00;					        // upper bye of transfer length
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5, "\r\nCFrmDscWr");
#endif
    retval = rsi_sendC1C2(c1, c2);			        // send C1/C2

    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL6, " Desc Write C1C2 Failed");
#endif    
        return retval;			// exit with error if we timed out waiting for the SPI to get ready
    }

    retval = rsi_sendC3C4(c3, c4);			        // send C3/C4
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5, " CFrameDscWr=");
#endif
    retval = rsi_spiSend(&uFrameDscFrame->uFrmDscBuf[0], RSI_FRAME_DESC_LEN, localBuf,
            RSI_MODE_32BIT);
    // the Management Type byte is in byte 1 (byte 0 is the length)
    return retval;
    // here we can only return ok
}

/*===========================================================================*/
/**
 * @fn          int16 rsi_SpiFrameDataWr(uint16 bufLen, uint8 *dBuf,uint16 tbufLen,uint8 *tBuf,uint8 type)
 * @brief       Writes a Control data frame decriptor.
 * @param[in]   uint16 buflen length of the data buffer to write
 * @param[in]   uint8 *dBuf, pointer to the buffer of data to write
 * @param[in]   uint16 tbuflen length of the data fragment to write
 * @param[in]   uint8 *tBuf, pointer to the buffer of data fragment to write
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS  
 * @section description 
 * This function performs Frame Data Write. 
 */
int16 rsi_SpiFrameDataWr(uint16 bufLen, uint8 *dBuf, uint16 tbufLen, uint8 *tBuf, uint8 type)
{
    int16 retval;
    uint8 c1;
    uint8 c2;
    uint8 c3;
    uint8 c4;
    uint8 localBuf[4];
    uint16 tempbufLen;
    tempbufLen = bufLen + tbufLen;

    c1 = RSI_C1FRMWR16BIT4BYTE;						   // 0x7c
#ifdef RSI_BIT_32_SUPPORT
            c2 = RSI_C2RDWR4BYTE | type;                                     // 0x42-DATA, 0x44-MGMT
#else
    c2 = RSI_C2RDWR1BYTE | type;						   // 0x02-DATA, 0x44-MGMT
#endif  
    c3 = (uint8) (tempbufLen & 0x00ff);					   // lower byte of transfer length
    c4 = (uint8) ((tempbufLen >> 8) & 0x00FF);				   // upper byte of transfer length
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5, "\r\nFrmDataWr");
#endif
    retval = rsi_sendC1C2(c1, c2);						   // send C1/C2
    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL6, "\r\n Data Write C1C2 Failed");
#endif 
        return retval;
    }						   // exit with error if we timed out waiting for the SPI to get ready

    retval = rsi_sendC3C4(c3, c4);					   // send C3/C4
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5, " FrameData=");
#endif
#if 1
    retval = rsi_spiSend(dBuf, bufLen, localBuf, RSI_MODE_32BIT);
    if (tbufLen)
    {
        gCallBack = NULL;
        retval = rsi_spiSend(tBuf, tbufLen, localBuf, RSI_MODE_32BIT);
    }
#endif

    return retval;							   // return success
}

int16 rsi_SpiFrameDataWrNonBlk(uint16 bufLen, uint8 *dBuf, uint16 tbufLen, uint8 *tBuf, uint8 type,
        void *callback)
{
    int16 retval;
    uint8 c1;
    uint8 c2;
    uint8 c3;
    uint8 c4;
    uint8 localBuf[4];
    uint16 tempbufLen;
    tempbufLen = bufLen + tbufLen;

    c1 = RSI_C1FRMWR16BIT4BYTE;						   // 0x7c
#ifdef RSI_BIT_32_SUPPORT
            c2 = RSI_C2RDWR4BYTE | type;                                     // 0x42-DATA, 0x44-MGMT
#else
    c2 = RSI_C2RDWR1BYTE | type;						   // 0x02-DATA, 0x44-MGMT
#endif  
    c3 = (uint8) (tempbufLen & 0x00ff);					   // lower byte of transfer length
    c4 = (uint8) ((tempbufLen >> 8) & 0x00FF);				   // upper byte of transfer length
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5, "\r\nFrmDataWr");
#endif
    retval = rsi_sendC1C2(c1, c2);						   // send C1/C2
    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL6, "\r\n Data Write C1C2 Failed");
#endif 
        return retval;
    }						   // exit with error if we timed out waiting for the SPI to get ready

    retval = rsi_sendC3C4(c3, c4);					   // send C3/C4
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5, " FrameData=");
#endif

    retval = rsi_spiSend(dBuf, bufLen, localBuf, RSI_MODE_32BIT);
    if (tbufLen)
    {
        gCallBack = (volatile void *) callback;
        retval = rsi_spiSend(tBuf, tbufLen, localBuf, RSI_MODE_32BIT);
    }

    return retval;							   // return success
}

/*===========================================================================*/
/**
 * @fn          int16 rsi_SpiFrameDataRd(uint16 bufLen, uint8 *dBuf)
 * @brief       Reads a data frame descriptor read
 * @param[in]   uint16 bufLen, length of data to write
 * @param[in]   uint8 *dBuf, pointer to the buffer of data to write
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This function performs Frame Data Read. 
 * 
 */
int16 rsi_SpiFrameDataRd(uint16 bufLen, uint8 *dBuf)
{
    int16 retval;
    uint8 c1;
    uint8 c2;
    uint8 c3;
    uint8 c4;

    bufLen = (bufLen + 3) & ~3;

    c1 = RSI_C1FRMRD16BIT4BYTE;				  // 0x5c
#ifdef RSI_BIT_32_SUPPORT
            c2 = RSI_C2MEMRDWRNOCARE | (0x40);
#else
    c2 = RSI_C2MEMRDWRNOCARE;				  // 0x00
#endif  
    c3 = (uint8) (bufLen & 0x00ff);				  // lower byte of transfer length
    c4 = (uint8) ((bufLen >> 8) & 0x00FF);			  // upper byte of transfer length
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5, " CFrmDataRd");
#endif
    retval = rsi_sendC1C2(c1, c2);				  // send C1/C2

    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL6, "\r\n Data read C1C2 Failed");
#endif
        return retval;
        // exit with error if we timed out waiting for the SPI to get ready
    }

    retval = rsi_sendC3C4(c3, c4);				  // send C3/C4

    retval = rsi_spiWaitStartToken(RSI_START_TOKEN_TIMEOUT, RSI_MODE_32BIT);
    // wait for start token
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5, " CFrameData=");
#endif
    retval = rsi_spiRecv(dBuf, bufLen, RSI_MODE_32BIT);

    return retval;						  // return success
}

