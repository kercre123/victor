/**
 * @file
 * @version			2.1.0.0
 * @date			2011-May-30
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief UTIL, Generic utils such as bytes2Touint16 which are not tied to anything.
 *
 * @section Description
 * This file implements misc utilities/functions.
 *
 */


/**
 * Include files
 */
#include "rsi_global.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/**
 * Global defines
 */


/*=============================================================================*/
/**
 * @fn              void rsi_uint16To2Bytes(uint8 *dBuf, uint16 val)
 * @brief           Convert uint16 to two byte array
 * @param[in]       uint8 *dBuf,pointer to buffer to put the data in
 * @param[in]       uint16 data to convert
 * @param[out]      none
 * @return          none
 */
void rsi_uint16To2Bytes(uint8 *dBuf, uint16 val)
{
	dBuf[0] = val & 0x00ff;
	dBuf[1] = (val >> 8) & 0x00ff;
}

/*=============================================================================*/
/**
 * @fn              void rsi_uint32To4Bytes(uint8 *dBuf, uint32 val)
 * @brief           Convert uint32 to four byte array
 * @param[in]       uint8 *dBuf,pointer to the buffer to put the data in
 * @param[in]       uint32 data to convert
 * @param[out]      none
 * @return          none
 */
void rsi_uint32To4Bytes(uint8 *dBuf, uint32 val)
{
	dBuf[0] = val & 0x000000ff;
	dBuf[1] = (val >> 8) & 0x000000ff;
	dBuf[2] = (val >> 16) & 0x000000ff;
	dBuf[3] = (val >> 24) & 0x000000ff;
}

/*=============================================================================*/
/**
 * @fn              uint16 rsi_bytes2RToUint16(uint8 *dBuf)
 * @brief           Convert a 2 byte array to uint16, first byte in array is LSB
 * @param[in]       uint8 *dBuf,pointer to a buffer to get the data from
 * @param[out]      none
 * @return          uint16, converted data
 */
uint16 rsi_bytes2RToUint16(uint8 *dBuf)
{
	uint16				val;		// the 16-bit value to return
	val = dBuf[1];
	val <<= 8;
	val |= dBuf[0] & 0x000000ff;
	return val;
}



