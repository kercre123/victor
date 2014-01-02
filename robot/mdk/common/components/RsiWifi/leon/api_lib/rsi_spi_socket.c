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
 * @brief SPI, SOCKET, Create the socket connection using the spi interface
 *
 * @section Description
 * This file contains the SPI SOCKET function.
 *
 *
 */

/**
 * Includes
 */
#include "rsi_global.h"
#include <stdio.h>
#include <string.h>


/**
 * Global Variables
 */

/*===========================================================================
 *
 * @fn          int16 rsi_socket(rsi_uSocket *uSocketFrame)
 * @brief       Sends the SOCKET open command to the Wi-Fi module
 * @param[in]   rsi_uSocket *uSocketFrame, pointer to socket create structure
 * @param[out]	none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This API is used to open a TCP/UDP Server/Client socket in the Wi-Fi module. 
 * It has to be called only after the module has been assigned an IP address using the 
 * rsi_ipparam_set API.
 * @section prerequisite 
 * WiFi Connection should be established before opening the sockets
 */
int16 rsi_socket(rsi_uSocket *uSocketFrame)
{
	int16					retval;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nSocket Open Start");
#endif
	// Copy in the SOCKET Frame Data from the global copy
#ifdef RSI_LITTLE_ENDIAN
	*(uint16 *)&uSocketFrame->uSocketBuf = RSI_REQ_SOCKET_CREATE;
#else
	rsi_uint16To2Bytes((uint8 *)&uSocketFrame->uSocketBuf, RSI_REQ_SOCKET_CREATE);
#endif  
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdSocket,(uint8 *)uSocketFrame,sizeof(rsi_uSocket));
	return retval;
}

