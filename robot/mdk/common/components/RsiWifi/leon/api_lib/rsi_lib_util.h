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
 * @brief HEADER, UTIL, Util Header file, the things that are used in the library 
 *
 * @section Description
 * This is the util.h file for the utility functions used by library.
 *
 *
 */


#ifndef _RSILIBUTIL_H_
#define _RSILIBUTIL_H_


void rsi_uint32To4Bytes(uint8 *dBuf, uint32 val);
void rsi_uint16To2Bytes(uint8 *dBuf, uint16 val);
uint16 rsi_bytes2RToUint16(uint8 *dBuf);

#endif
