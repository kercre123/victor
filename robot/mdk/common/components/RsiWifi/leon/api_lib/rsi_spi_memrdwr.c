/**
 * @file
 * @version		2.0.0.0
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
 * @brief SPI, MEMRDWR, SPI Memory Read/Write functions, r/w memory on the module via the SPI interface
 *
 * @section Description
 * This file contains the SPI Memory Read/Write functions to the module
 *
 *
 */

/**
 * Includes
 */
#include "rsi_global.h"
#include <stdio.h>

/**
 * Global Variables
 */

/*===========================================================================*/
/** 
 * @fn          int16 rsi_memWr(uint32 addr, uint16 len, uint8 *dBuf)
 * @brief       Performs a memory write to the Wi-Fi module
 * @param[in]   uint32 addr, address to write to
 * @param[in]   uint16, len, number of bytes to write
 * @param[in]   uint8 *dBuf, pointer to the buffer of data to write
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 *
 * ABH Master Write (Internal Legacy Name)
 */
int16 rsi_memWr(uint32 addr, uint16 len, uint8 *dBuf)
{
    uint8 txCmd[4];
    int16 retval;
    uint8 c1;
    uint8 c2;
    uint8 c3;
    uint8 c4;
    uint8 localBuf[4];

    c1 = RSI_C1MEMWR16BIT4BYTE;
#ifdef RSI_BIT_32_SUPPORT
    c2 = RSI_C2SPIADDR4BYTE;                                               //0x40
#else
    c2 = RSI_C2SPIADDR1BYTE;                                               //0x00
#endif
    c3 = (int8) (len & 0x00ff);						// C3, LSB of length
    c4 = (int8) ((len >> 8) & 0x00ff);					// C4, MSB of length

#ifdef RSI_LITTLE_ENDIAN
    *(uint32 *)txCmd=addr;
#else
    rsi_uint32To4Bytes(txCmd, addr);
#endif
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5,"\r\nMemWr");
#endif
    // Send C1/C2
    retval = rsi_sendC1C2(c1, c2);

    // Check for SPI Busy/Error
    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL6,"\r\nMemWr C1C2 Fail, SPI Busy ");
#endif
        return retval;
    }

    // Send C3/C4
    retval = rsi_sendC3C4(c3, c4);
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5,"\r\nMemWrAddr=");
#endif
    // Send the 4 address bytes
    retval = rsi_spiSendNoIrq(txCmd, sizeof(txCmd), localBuf, RSI_MODE_8BIT);
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL5,"\r\n MemWrData=");
#endif
    // Send the Data
#ifdef RSI_BIT_32_SUPPORT
    retval = rsi_spiSendNoIrq(dBuf, len, localBuf,RSI_MODE_32BIT);
#else
    retval = rsi_spiSendNoIrq(dBuf, len, localBuf, RSI_MODE_8BIT);
#endif  
    return retval;
}

/*===========================================================================*/
/**
 * @fn          int16 rsi_memRd(uint32 addr, uint16 len, uint8 *dBuf)
 * @brief       Performs a memory read from the Wi-Fi module
 * @param[in]   uint32, address to read from
 * @param[in]   uint16, len, number of bytes to read
 * @param[in]   uint8 *dBuf, pointer to the buffer to receive the data into
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 *
 * ABH Master Read (Internal Legacy Name)
 */
int16 rsi_memRd(uint32 addr, uint16 len, uint8 *dBuf)
{
    uint8 txCmd[4];
    int16 retval;
    uint8 c1;
    uint8 c2;
    uint8 c3;
    uint8 c4;
    uint8 localBuf[4];

    c1 = RSI_C1MEMRD16BIT4BYTE;
#ifdef RSI_BIT_32_SUPPORT
    c2 = RSI_C2SPIADDR4BYTE;
#else
    c2 = RSI_C2MEMRDWRNOCARE;
#endif  
    c3 = len & 0x00ff;						// C3, LSB of length
    c4 = (len >> 8) & 0x00ff;					// C4, MSB of length
    // put the address bytes into the buffer to send
    txCmd[0] = addr & 0x000000ff;						// A0, Byte 0 of address (LSB)
    txCmd[1] = (addr >> 8) & 0x000000ff;					// A1, Byte 1 of address
    txCmd[2] = (addr >> 16) & 0x000000ff;					// A2, Byte 2 of address
    txCmd[3] = (addr >> 24) & 0x000000ff;					// A3, Byte 3 of address (MSB)
#ifdef RSI_DEBUG_PRINT
            RSI_DPRINT(RSI_PL15,"\r\nMemoryRd");
#endif
    // Send C1/C2
    retval = rsi_sendC1C2(c1, c2);

    // Check for SPI busy
    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL16,"MemRd, C1C2 Fail, SPI Busy ");
#endif
        return retval;
    }

    // Send C3/C4
    retval = rsi_sendC3C4(c3, c4);
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL15," MemRdAddr=");
#endif
    // Send the 4 address bytes
    retval = rsi_spiSendNoIrq(txCmd, sizeof(txCmd), localBuf, RSI_MODE_8BIT);

    // Wait for the start token
    retval = rsi_spiWaitStartToken(RSI_START_TOKEN_TIMEOUT, RSI_MODE_8BIT);
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL15," MRdDataRd=");
#endif
    // Read in the memory data
    retval = rsi_spiRecv(dBuf, len, RSI_MODE_8BIT);

    return retval;
}

