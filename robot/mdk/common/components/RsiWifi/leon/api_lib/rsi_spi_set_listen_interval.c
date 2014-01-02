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
 * @brief SPI, SET LISTEN INTERVAL
 * @section Description            
 * This file contains function to set listen interval for the WiFi 
 * when the power save is enabled.
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
 * @fn	        int16 rsi_set_listen_interval(uint8 *listeninterval)
 * @brief	Sends the set LISTEN INTERVAL command to the Wi-Fi module via SPI
 * @param[in]	uint8 *listeninterval, listen interval value
 * @param[out]	none
 * @return	errCode
 *		-1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *		0  = SUCCESS
 * @section description  
 * This API is used to set the given LISTEN INTERVAL to the WiFi module
 * when the module is in power save mode
 * @section prerequisite  
 * This is used when the module is in power save mode
 */
int16 rsi_set_listen_interval(uint8 *listeninterval)
{
	int16						retval;
	rsi_uSetListenInterval				uSetListenIntervalFrame;														// frame sent as the join command
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\nSetListenInterval Start ");
#endif
#ifdef RSI_LITTLE_ENDIAN
	*(uint16 *) uSetListenIntervalFrame.setListenIntervalFrameSnd.setListenIntervalCmd = 
								*(uint16 *) rsi_dFrmSetListenInterval; 
#else
	memcpy(&uSetListenIntervalFrame, rsi_dFrmSetListenInterval, RSI_DFRAMECMDLEN);
#endif
	//send the Listen Interval
	memcpy(uSetListenIntervalFrame.setListenIntervalFrameSnd.interval, listeninterval, 
			sizeof(uSetListenIntervalFrame.setListenIntervalFrameSnd.interval));
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdSetListenInterval,
			      (uint8 *)&uSetListenIntervalFrame,sizeof(rsi_uSetListenInterval));
	return retval;
}


