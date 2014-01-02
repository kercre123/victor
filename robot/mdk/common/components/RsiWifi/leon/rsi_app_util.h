/**
 * @file
 * @version		2.1.0.0
 * @date 		2011-May-30
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @defgroup RsiWifi RsiWifi component
 * @{
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief HEADER, UTIL, Util Header file, the things that are useful for application 
 *
 * @section Description
 * This is the rsi_app_util.h file for the utility functions used by applications that
 * are using this library.
 *
 *
 */


#ifndef _RSIAPPUTIL_H_
#define _RSIAPPUTIL_H_

///Convert a 2 byte array to uint16, first byte in array is MSB
///@param[in]		dBuf pointer to a buffer to get the data from
///@return		uint16, converted data
uint16 rsi_bytes2ToUint16(uint8 *dBuf);

///Convert a 4 byte array to uint32, first byte in array is MSB
///@param[in]		dBuf pointer to buffer to get the data from
///@return		uint32, converted data
uint32 rsi_bytes4ToUint32(uint8 *dBuf);

///swap the first 2 bytes of an array
///@param[in]		buffer pointer to buffer of data to convert
///@return		value returned in the array passed
void rsi_swap2Bytes(uint8 *buffer);

///@brief		swap the byte order of a 4 Byte array
///@param[in]		buffer pointer to buffer of data to convert
///@return		value returned in the array passed
void rsi_swap4Bytes(uint8 *buffer);

///Print n characters from the string passed
///@param[in]		st string to print from
///@param[in]		n number of characters to print
///@return			none
void rsi_printNChars (int8* st, int16 n);

 ///Print an uint8 number in binary, as in: 0x31 --> 00110001
 ///@param[in]		number number to print
 ///@return		value returned in the array passed
void rsi_printUint8AsBinary(uint8 number);

///Check if a uint8 number is in a list
///@param[in]		arg number to be checked in the list
///@param[in]		list list of numbers to check against
///@param[in]		nargs number of arguments
///@return		value returned in the array passed
int16 rsi_inUint8List(uint8 arg, uint8 *list, uint8 nargs);

///Convert an uint8 6-Byte array to  : notation MAC address
///@param[in]		hexAddr Hex address to convert
///@param[in]		strBuf  pointer to a dummy String to hold the MAC address
///@return		int8 * destString, pointer to the string with the data
int8 * rsi_bytes6ToAsciiMacAddr(uint8 *hexAddr,uint8 *strBuf);

///Convert an uint8 4-Byte array to  . notation network address
///@param[in]		hexAddr Hex address to convert
///@param[in]		strBuf String Pointer to hold the Concatenated String
///@return		char * destString, pointer to the string with the data
int8 * rsi_bytes4ToAsciiDotAddr(uint8 *hexAddr,uint8 *strBuf);

///Convert an ASCII . notation network address to 4-byte hex address
///@param[in]		asciiDotAddress  source address to convert, must be a null terminated string
///@param[out]		hexAddr Output value is passed back in the 4-byte Hex Address
///@return		none
void rsi_asciiDotAddressTo4Bytes(uint8 *hexAddr, int8 *asciiDotAddress);

///Convert an ASCII : notation MAC address to a 6-byte hex address
///@param[in]		asciiMacAddress source address to convert, must be a null terminated string
///@param[out]		hexAddr converted address is returned here 
///@return		none

void rsi_asciiMacAddressTo6Bytes(uint8 *hexAddr, int8 *asciiMacAddress);

///Convert a 4 byte array to uint32, first byte in array is LSB
///@param[in]		dBuf pointer to buffer to get the data from
///@return		uint32, converted data
uint32 rsi_bytes4RToUint32(uint8 *dBuf);

 ///Convert given ASCII hex notation to descimal notation (used for mac address)
 ///@param[in]	cBuf ASCII hex notation string
 ///@return	value in integer
int8 rsi_charHex2Dec ( int8 *cBuf);
/// @}
#endif
