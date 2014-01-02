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
 * @brief UTIL, Generic utils such as swapping which are not tied to anything.
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

/**
 * Global variables
 */
volatile rsi_intStatus		rsi_strIntStatus;

 void mv_spiInit( u32 spiBaseAddr );
 
/*================================================*/
/**
 * @fn			void rsi_printNChars (char* st, int n)
 * @brief		Print n characters from the string passed
 * @param[in]		char* st,string to print from
 * @param[in]		int n,number of characters to print
 * @param[out]		none
 * @return			none
 */
void rsi_printNChars (int8* st, int16 n)
{
	int16			i;

	for (i = 0; i < n; i++) {
		putchar(st[i]);
	}
}


/*=============================================================================*/
/**
 * @fn			uint32 rsi_bytes4ToUint32(uint8 *dBuf)
 * @brief		Convert a 4 byte array to uint32, first byte in array is MSB
 * @param[in]		uint8 *dBuf,pointer to buffer to get the data from
 * @param[out]		none
 * @return		uint32, converted data
 */
uint32 rsi_bytes4ToUint32(uint8 *dBuf)
{
	uint32				val;				// the 32-bit value to return

	val = dBuf[0];
	val <<= 8;
	val |= dBuf[1] & 0x000000ff;
	val <<= 8;
	val |= dBuf[2] & 0x000000ff;
	val <<= 8;
	val |= dBuf[3] & 0x000000ff;

	return val;
}



/*=============================================================================*/
/**
 * @fn			uint32 rsi_bytes4RToUint32(uint8 *dBuf)
 * @brief		Convert a 4 byte array to uint32, first byte in array is LSB
 * @param[in]		uint8 *dBuf,pointer to buffer to get the data from
 * @param[out]		none
 * @return		uint32, converted data
 */
uint32 rsi_bytes4RToUint32(uint8 *dBuf)
{
	uint32				val;				// the 32-bit value to return

	val = dBuf[3];
	val <<= 8;
	val |= dBuf[2] & 0x000000ff;
	val <<= 8;
	val |= dBuf[1] & 0x000000ff;
	val <<= 8;
	val |= dBuf[0] & 0x000000ff;

	return val;
}



/*=============================================================================*/
/**
 * @fn			uint16 rsi_bytes2ToUint16(uint8 *dBuf)
 * @brief		Convert a 2 byte array to uint16, first byte in array is MSB
 * @param[in]		uint8 *dBuf,pointer to a buffer to get the data from
 * @param[out]		none
 * @return		uint16, converted data
 */
uint16 rsi_bytes2ToUint16(uint8 *dBuf)
{
	uint16				val;				// the 16-bit value to return

	val = dBuf[0];
	val <<= 8;
	val |= dBuf[1] & 0x000000ff;

	return val;
}

/*=============================================================================*/
/**
 * @fn			int8 * rsi_bytes6ToAsciiMacAddr(uint8 *hexAddr)
 * @brief		Convert an uint8 6-Byte array to  : notation MAC address
 * @param[in]		uint8 *hexAddress, Address to convert
 * @param[in]		uint8 *strBuf, pointer to a dummy String to hold the MAC address
 * @param[out]		none
 * @return		int8 * destString, pointer to the string with the data
 */
int8 * rsi_bytes6ToAsciiMacAddr(uint8 *hexAddr,uint8 *strBuf)
{
	uint8			i;					// loop counter
	uint8			cBuf[3];				// character buffer
	uint8			*destString;

	strBuf[0] = 0;							// make  strcat think the array is empty
	for (i = 0; i < 5; i++) {					// MAC Address is 6 bytes long
		// This will take care of the first5 bytes
		sprintf ((char *)cBuf, "%02x", (unsigned int)(((uint8*)hexAddr)[i]));
		cBuf[2] = 0;							// terminate the string
		destString =(uint8 *) strcat((char *)strBuf,(char *) cBuf);
		destString = (uint8 *)strcat((char *)strBuf, (char *)":");
	}
	// take care of the last entry outside the loop, there is no . after the last octet
	sprintf ((char *)cBuf, "%02x", (unsigned int)(((uint8*)hexAddr)[i]));
	cBuf[2] = 0;															
	// terminate the string
	destString = (uint8 *)strcat((char *)strBuf,(char *) cBuf);
	return (int8 *) destString;
}


/*=============================================================================*/
/**
 * @fn			void rsi_swap2Bytes(uint8 *buffer)
 * @brief		swap the first 2 bytes of an array
 * @param[in]		uint8 *buffer,pointer to buffer of data to convert
 * @param[out]		none
 * @return		value returned in the array passed
 */
void rsi_swap2Bytes(uint8 *buffer)
{
	uint8			tmp;

	tmp = buffer[0];
	buffer[0] = buffer[1];
	buffer[1] = tmp;
}


/*=============================================================================*/
/**
 * @fn			void rsi_swap4Bytes(uint8 *buffer)
 * @brief		swap the byte order of a 4 Byte array
 * @param[in]		uint8 *buffer,pointer to buffer of data to convert
 * @param[out]		none
 * @return		value returned in the array passed
 */
void rsi_swap4Bytes(uint8 *buffer)
{
	uint8			tmp;

	tmp = buffer[0];
	buffer[0] = buffer[3];
	buffer[3] = tmp;
	tmp = buffer[1];
	buffer[1] = buffer[2];
	buffer[2] = tmp;
}


/*=============================================================================*/
/**
 * @fn			void rsi_printUint8AsBinary(uint8 number)
 * @brief		print an uint8 number in binary, as in: 0x31 --> 00110001
 * @param[in]		uint8 number to print
 * @param[out]		none
 * @return		value returned in the array passed
 */
void rsi_printUint8AsBinary(uint8 number)
{
	uint16			i;						// loop counter

	i = 1<<(sizeof(number) * 8 - 1);


	while (i > 0) {
		if (number & i) {
			printf("1");
		}
		else {
			printf("0");
		}
		i >>= 1;
	}
}



/*=============================================================================*/
/**
 * @fn			int16 rsi_isUint8InList(uint8 arg, uint8 *list, uint8 nargs)
 * @brief		check if a uint8 number is in a list
 * @param[in]		uint8 arg,number to be checked in the list
 * @param[in]		uint8 *list,list of numbers to check against
 * @param[in]		uint8 nargs,number of arguments
 * @param[out]		none
 * @return		value returned in the array passed
 */
int16 rsi_inUint8List(uint8 arg, uint8 *list, uint8 nargs)
{
	int16				retval;	           // return value, TRUE or FALSE
	uint8				i;			   // loop counter

	retval = RSI_FALSE;																
	// start off with a false, if we find a match, then change to a true to return
	for (i = 0; i < nargs; i++) {
		if (arg == list[i]) {
			retval = RSI_TRUE;
			break;
		}
	}

	return retval;
}


/*=============================================================================*/
/**
 * @fn			int8 * rsi_bytes4ToAsciiDotAddr(uint8 *hexAddr, uint8 *strBuf)
 * @brief		Convert an uint8 4-Byte array to  . notation network address
 * @param[in]		uint8 *hexAddress, Address to convert
 * @param[in]		uint8 *strBuf, String Pointer to hold the Concatenated String
 * @param[out]		none
 * @return		char * destString, pointer to the string with the data
 */
int8 * rsi_bytes4ToAsciiDotAddr(uint8 *hexAddr,uint8 *strBuf)
{
  uint8			i;							// loop counter
  uint8			ii;							// loop counter
  int8			cBuf[4];						// character buffer
  int8			*destString;

  strBuf[0] = 0;								// make  strcat think the array is empty
  for (i = 0; i < 3; i++) {							// we are assuming IPV4, so 4 bytes
    // This will take care of the first 3 bytes
    // zero out the character buffer since we don't know how long the string will be
    for(ii = 0; ii < sizeof(cBuf); ii++) { cBuf[ii] = 0; }	
    sprintf ((char *)cBuf, "%d", (unsigned int)(((uint8*)hexAddr)[i]));
    destString =(int8 *) strcat((char *)strBuf,(char *) cBuf);
    destString = (int8 *)strcat((char *)strBuf,(char *) ".");
  }
  // take care of the last entry outside the loop, there is no . after the last octet
  // zero out the character buffer since we don't know how long the string will be
  for(ii = 0; ii < sizeof(cBuf); ii++) { cBuf[ii] = 0; }		
  sprintf ((char *)cBuf, "%d", (unsigned int)(((uint8*)hexAddr)[i]));
  destString = (int8 *) strcat((char *)strBuf,(char *) cBuf);
  return destString;
}

/*=============================================================================*/
/**
 * @fn		  int8 rsi_charHex2Dec ( uint8 *cBuf)
 * @brief	  Convert given ASCII hex notation to descimal notation (used for mac address)
 * @param[in]	  int8 *cBuf, ASCII hex notation string
 * @return	  value in integer
 */
int8 rsi_charHex2Dec ( int8 *cBuf)
{
	int8 k=0;
	int8 i;
	for(i=0;i<strlen((char*)cBuf);i++)
	 {
	    k = ((k*16) +(cBuf[i] - '0')); 
     }
	 return k;
}

/*=============================================================================*/
/**
 * @fn			void rsi_asciiMacAddressTo6Bytes(uint8 *hexAddr, int8 *asciiMacAddress)
 * @brief		Convert an ASCII : notation MAC address to a 6-byte hex address
 * @param[in		int8 *asciiMacFormatAddress, source address to convert, must be a null terminated string
 * @param[out]		uint8 *hexAddr, converted address is returned here 
 * @return		none
 */
void rsi_asciiMacAddressTo6Bytes(uint8 *hexAddr, int8 *asciiMacAddress)
{
  uint8			i;						// loop counter
  uint8			cBufPos;					// which char in the ASCII representation
  uint8			byteNum;					// which byte in the 32BitHexAddress
  int8			cBuf[6];					// temporary buffer

  byteNum = 0;
  cBufPos = 0;
  for (i = 0; i < strlen((char *)asciiMacAddress); i++) {
    // this will take care of the first 5 octets
    if (asciiMacAddress[i] == ':') {					// we are at the end of the address octet
      cBuf[cBufPos] = 0;						// terminate the string
      cBufPos = 0;							// reset for the next char
      hexAddr[byteNum++] = (uint8)rsi_charHex2Dec((int8 *)cBuf);	// convert the strint to an integer
    }
    else {
      cBuf[cBufPos++] = asciiMacAddress[i];
    }
  }
  // handle the last octet						// we are at the end of the string with no .
  cBuf[cBufPos] = 0x00;							// terminate the string
  hexAddr[byteNum] = (uint8)rsi_charHex2Dec((int8 *)cBuf);		// convert the strint to an integer
}


/*=============================================================================*/
/**
 * @fn			void rsi_asciiDotAddressTo4Bytes(uint8 *hexAddr, int8 *asciiDotAddress)
 * @brief		Convert an ASCII . notation network address to 4-byte hex address
 * @param[in]		int8 *asciiDotFormatAddress, source address to convert, must be a null terminated string
 * @param[out]		uint8 *hexAddr,	Output value is passed back in the 4-byte Hex Address
 * @return		none
 */
void rsi_asciiDotAddressTo4Bytes(uint8 *hexAddr, int8 *asciiDotAddress)
{
  uint8			i;															
  // loop counter
  uint8			cBufPos;													
  // which char in the ASCII representation
  uint8			byteNum;													
  // which byte in the 32BitHexAddress
  int8			cBuf[4];													
  // character buffer

  byteNum = 0;
  cBufPos = 0;
  for (i = 0; i < strlen((char *)asciiDotAddress); i++) {
    // this will take care of the first 3 octets
    if (asciiDotAddress[i] == '.') {										
	    // we are at the end of the address octet
      cBuf[cBufPos] = 0;													
      // terminate the string
      cBufPos = 0;														
      // reset for the next char
      hexAddr[byteNum++] = (uint8)atoi((char *)cBuf);								
      // convert the strint to an integer
    }
    else {
      cBuf[cBufPos++] = asciiDotAddress[i];
    }
  }
  // handle the last octet													
  // // we are at the end of the string with no .
  cBuf[cBufPos] = 0x00;														
  // terminate the string
  hexAddr[byteNum] = (uint8)atoi((char *)cBuf);										
  // convert the strint to an integer
}


/*=============================================================================*/
/**
 * @fn			int rsiSetup(rsi_api *ptrStrApi, rsi_uSocket *ptrSocketFrame, rsi_uCmdRsp	*ptrCmdRspFrame);
 * @brief		Setup the Redpine Signals Wifi
 * @param[in]		rsi_api *ptrStrApi most settings to connect to a router
 * @param[in]		rsi_uSocket *ptrSocketFrame settings to connect to a TCP/UDP server
 * @param[out]	rsi_uCmdRsp	*ptrCmdRspFrame last response from the wifi module
 * @return      status
 *	        0  = SUCCESS ( ptrCmdRspFrame for detailed error codes )
 */
int rsiSetup(u32 rsiSpiBaseAddr, rsi_api *ptrStrApi, rsi_uSocket *ptrSocketFrame, rsi_uCmdRsp	*ptrCmdRspFrame) {
  int16 retval;
  /*========================================================*/
  /**
   * System Initialization
   *========================================================*/
  /*
   * Initialize the platform specific HAL init - here 
   */
  mv_spiInit( rsiSpiBaseAddr );

  /*
   * Module specfic init
   */
  retval = rsi_sys_init();
  if(retval != 0)
  {
    return -1;  // if there is any interface level failure, we will return from main
  }
 
/*
 * The inclusion of Image upgrade, Soft boot load and Functional firmware files
 * are done by considering a folder with name "Firmware" in API_Lib directory,
 * which has these files. User is recommended to place his FF,SB,IU files in a 
 * folder with the name "Firmware" and the folder should be with in API_Lib, 
 * instead of changing the path in the APIs.
 */
  
/*
 * To upgrade the firmware the MACRO, RSI_FIRMWARE_UPGRADE should be set to 1
 */  
#if RSI_FIRMWARE_UPGRADE
#ifdef RSI_DEBUG_PRINT
  RSI_DPRINT(RSI_PL1,"\r\nStarting SPI fwupgrade ");
#endif
  retval = rsi_fwupgrade();
  if(retval !=0)
  {
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL2,"\r\nImage Upgradation Error=%02x", retval);
#endif    
	return -2;
  }
/*
 * shutdown the platform specific hard ware
 */
#endif

/* 
 * Now for the following steps to be done, RSI_FIRMWARE_UPGRADE macro should be set to zero
 */
 

/*
 * Soft boot loader
 */ 
  retval = rsi_bootloader();
  if(retval!=0)
  {
    return -3;  // if there is any interface level failure, we will return from main
  }

/*
 * Sending band command
 */
  retval = rsi_band(ptrStrApi->band);
  if(retval!=0)
  {
    return -4;  // if there is any interface level failure, we will return from main
  }
  else
  {
    /* instead of waiting for response event, application can do other tasks */
    RSI_RESPONSE_TIMEOUT(RSI_BANDTIMEOUT);
    rsi_read_packet(ptrCmdRspFrame);
    rsi_clearPktIrq();
  }

/*
 * Sending mode select command,
 * As all the even numbered modules contain TCP /IP stack pass '22' to it.
 */
  retval = rsi_spi_mode_sel(22);
  if(retval!=0)
  {
    return -5;  // if there is any interface level failure, we will return from main
  }
  else
  {
    /* instead of waiting for response event, application can do other tasks */
    RSI_RESPONSE_TIMEOUT(RSI_MODESEL_TIMEOUT);
    rsi_read_packet(ptrCmdRspFrame);
    rsi_clearPktIrq();
  }

/*
 * Sending Init Command
 */
  retval = rsi_init();
  if(retval!=0)
  {
    return -6;  // if there is any interface level failure, we will return from main
  }
  else
  {
    /* instead of waiting for response event, application can do other tasks */
    RSI_RESPONSE_TIMEOUT(RSI_INITTIMEOUT);
    rsi_read_packet(ptrCmdRspFrame);
    rsi_clearPktIrq();
  }
    
  
/*
 * Sending Scan Command
 */
  retval = rsi_scan(&ptrStrApi->uScanFrame);
  if(retval!=0)
  {
    return -7;  // if there is any interface level failure, we will return from main
  }
  else
  {
    /* instead of waiting for response event, application can do other tasks */
    RSI_RESPONSE_TIMEOUT(RSI_SCANTIMEOUT);
    rsi_read_packet(ptrCmdRspFrame);
    rsi_clearPktIrq();
  }
  
 
/*
 * Sending Join Command
 */
  retval = rsi_join(&ptrStrApi->uJoinFrame);
  if(retval!=0)
  {
    return -8;  // if there is any interface level failure, we will return from main
  }
  else
  {
    /* instead of waiting for response event, application can do other tasks */
    RSI_RESPONSE_TIMEOUT(RSI_JOINTIMEOUT);
    rsi_read_packet(ptrCmdRspFrame);
    rsi_clearPktIrq();
  }
 
  
/*
 * Sending Ipconfig Command
 */
  retval = rsi_ipparam_set(&ptrStrApi->uIpparamFrame);
  if(retval!=0)
  {
    return -9;  // if there is any interface level failure, we will return from main
  }
  else
  {
    /* instead of waiting for response event, application can do other tasks */
    RSI_RESPONSE_TIMEOUT(RSI_IPPTIMEOUT);
    rsi_read_packet(ptrCmdRspFrame);
    rsi_clearPktIrq();
  }

/*
 * Sending socket create Command, here we are creating a socket of type TCP client
 */
  retval = rsi_socket(ptrSocketFrame);
  if(retval!=0)
  {
    return -10;  // if there is any interface level failure, we will return from main
  }
  else
  {
    /* instead of waiting for response event, application can do other tasks */
    RSI_RESPONSE_TIMEOUT(RSI_SOPTIMEOUT);
    rsi_read_packet(ptrCmdRspFrame);
    rsi_clearPktIrq();
  }
    RSI_DPRINT(RSI_PL3, "ptrSocketFrame->destIpaddr:%d.%d.%d.%d",
          ptrSocketFrame->socketFrameSnd.destIpaddr[3],
          ptrSocketFrame->socketFrameSnd.destIpaddr[2],
          ptrSocketFrame->socketFrameSnd.destIpaddr[1],
          ptrSocketFrame->socketFrameSnd.destIpaddr[0]
        );
  RSI_DPRINT(RSI_PL3, "ptrSocketFrame->socketType:%s",
             (*(uint16 *)(ptrSocketFrame->socketFrameSnd.socketType)==0)?"TCP Client":"UDP Client/TCP Server"
            );
  RSI_DPRINT(RSI_PL3, "ptrCmdRspFrame->socketFrameRcv.errCode:%08X", *(unsigned*)(ptrCmdRspFrame->socketFrameRcv.errCode));
/*
 * Configuring the power mode for the wifi module.
 */
  //retval = rsi_power_mode((uint8 *)ptrStrApi->powerMode);
    
  //retval = rsi_pwrsave_hold(); // to send data in power mode 1. For more, Refer to PRM and API documentation
  return 0;
}
