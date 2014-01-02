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
 * @brief SPI, JOIN, implements the spi version of the join command
 *
 * @section Description
 * This file contains the SPI Join function.
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

/*=================================================*/
/**
 * @fn          int16 rsi_join(rsi_uJoin *uJoinFrame)
 * @brief       Sends the Join command to the Wi-Fi module via SPI
 * @param[in]   rsi_uJoin *uJoinFrame, Pointer to join structure
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This API is used to connect the Wi-Fi module to an Access Point. 
 * This API should be called only after rsi_scan API.
 *
 * @section prerequisite 
 * The rsi_scan for the access point should be done sussessfully.
 */
int16 rsi_join(rsi_uJoin *uJoinFrame)
{
	int16					retval;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\n Join Start");
#endif
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdJoin,(uint8 *)uJoinFrame,sizeof(rsi_uJoin));
	return retval;
}


