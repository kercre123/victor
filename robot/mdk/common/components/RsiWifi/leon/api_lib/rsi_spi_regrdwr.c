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
 * @brief SPI, REGRDWR, Register Read/Write functions
 *
 * @section Description
 * This file contains the SPI based Register R/W functionss
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




/**===========================================================================
 * @fn          int16 rsi_regRd(uint8 regAddr, uint8 *dBuf)
 * @brief       Reads a register in the Wi-Fi module
 * @param[in]   uint8 regAddr, address of spi register to read, addr is 6-bits, upper 2 bits must be cleared
 * @param[in]   uint8 *dBuf, pointer to the buffer of data to write, assumed to be at least 2 bytes long
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * 
 * @section description 
 * Reads a spi register with an address specified.
 *
 */
int16 rsi_regRd(uint8 regAddr, uint8 *dBuf)
{
	int16                   retval;
	uint8                   c1;
	uint8                   c2;

	// set the C1/C2 values
	c1 = RSI_C1INTREAD1BYTES;
	c2 = 0x00;																		// and/or the addrss into C2, addr is 6-bits long, upper two bits must be cleared for byte mode
	c2 |= regAddr;
#ifdef RSI_DEBUG_PRINT
	// send C2/C2
	RSI_DPRINT(RSI_PL5,"\r\nRegRd");
#endif
	retval = rsi_sendC1C2(c1, c2);


	// Check for SPI Busy/Error
	if (retval != 0) {
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL6,"\r\nRegRd C1C2 Fail, SPI Busy ");
#endif
		return retval;																	// exit with error if we timed out waiting for the SPI to get ready
	}

	retval = rsi_spiWaitStartToken(RSI_START_TOKEN_TIMEOUT, RSI_MODE_8BIT);

	if (retval != 0) { 																	// exit with error if we timed out waiting for start token
		return retval;
	}

	// Start token found now read the two bytes of data
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL5," RegData=");
#endif
	retval = rsi_spiRecv(dBuf, 1, RSI_MODE_8BIT);
	return retval;																		// we return the error status here
}

/**===========================================================================
 * @fn          int16 rsi_regWr(uint8 regAddr, uint8 *dBuf)
 * @brief       writes to a register in the Wi-Fi module
 * @param[in]   uint8 regAddr, address of spi register to be written
 * @param[in]   uint8 *dBuf, pointer to the buffer of data to write, assumed to be at least 2 bytes long
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * 
 * @section description 
 * Writes to a spi register with an address specified.
 *
 */

int16 rsi_regWr(uint8 regAddr, uint8 *dBuf)
{
	int16                   retval;
	uint8                   c1;
	uint8                   c2;
    uint8                   localBuf[4];

	// set the C1/C2 values
	c1 = RSI_C1INTWRITE2BYTES;
	c2 = 0x00;																		
	   // and/or the addrss into C2, addr is 6-bits long, upper two bits must be cleared for byte mode
	c2 |= regAddr;
#ifdef RSI_DEBUG_PRINT
	// send C2/C2
	RSI_DPRINT(RSI_PL5,"\r\nRegWr");
#endif
	retval = rsi_sendC1C2(c1, c2);


	// Check for SPI Busy/Error
	if (retval != 0) {
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL6,"\r\nRegWr C1C2 Fail, SPI Busy ");
#endif
		return retval;		
		// exit with error if we timed out waiting for the SPI to get ready
	}
        
  retval = rsi_spiSend(dBuf, 2, localBuf, RSI_MODE_8BIT);

	if (retval != 0) 
    {   
		// exit with error if we timed out waiting for start token
    return retval;
	}
	return retval;
}
