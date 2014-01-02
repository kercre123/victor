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
 * @brief SPI, SOCKET, CLOSE, Close an open socket
 *
 * @section Description
 * This file contains the SPI SOCKET CLOSE function.
 *
 *
 */


/**
 * Includes
 */
#include "rsi_global.h"
#include <stdio.h>
#include <string.h>

/**===========================================================================
 * @fn          int16 rsi_socket_close(uint16 socketDescriptor)
 * @brief       Closes an opensocket
 * @param[in]   uint16 socketDescriptor, socket to close
 * @param[out]	none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 *       
 * @section description  
 * This API is used to close an already open socket.
 * @section prerequisite 
 * Socket with the given descriptor should already been created
 */
int16 rsi_socket_close(uint16 socketDescriptor)
{
	int16                                   retval;
	rsi_uSocketClose                        uSocketCloseFrame;
	// Copy the command frame data to a local buffer
	memcpy(uSocketCloseFrame.uSocketCloseBuf, rsi_dFrmSocketClose, sizeof(rsi_uSocketClose));
#ifdef RSI_LITTLE_ENDIAN  
	*(uint16 *)uSocketCloseFrame.socketCloseFrameSnd.socketDescriptor = socketDescriptor;
#else
	rsi_uint16To2Bytes((uint8 *)&uSocketCloseFrame.socketCloseFrameSnd.socketDescriptor, socketDescriptor);
#endif
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdSocketClose,(uint8 *)&uSocketCloseFrame,sizeof(rsi_uSocketClose));
	return retval;
}

/* [] END OF FILE */

