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
 * @brief SPI, DISCONNECT Function
 *
 * @section Description
 * This file contains the SPI DISCONNECT function.
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


/*===================================================*/
/**
 * @fn         int16 rsi_disconnect()
 * @brief      Sends the DISCONNECT command to the Wi-Fi module
 * @param[in]  none	
 * @param[out] none	
 * @return     errCode
 *	       -1 = SPI busy / Timeout
 *             -2 = SPI Failure
 *             0  = SUCCESS
 *
 * @section description 
 * This function is used to disconnect the moduleCs Wi-Fi connection.
 * @section prerequisite 
 * Wi-Fi connection should be successful
 */
int16 rsi_disconnect()
{
	int16						retval;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nDisconnect Start");
#endif
	retval=rsi_execute_cmd((uint8 *)rsi_frameCmdDisconnect,(uint8 *)rsi_dFrmDisconnect,sizeof(rsi_dFrmDisconnect));
	return retval;
}


