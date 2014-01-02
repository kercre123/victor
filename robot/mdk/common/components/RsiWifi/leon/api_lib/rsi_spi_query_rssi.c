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
 * @brief SPI, RSSI Query Function
 *
 * @section Description
 * This file contains the RSSI function.
 *
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
 * @fn          int16 rsi_query_rssi(void)
 * @brief       Sends the RSSI Query command to the Wi-Fi module
 * @param[in]   none
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description	 
 * This API is used to query the RSSI value of the Access Point to which 
 * the Wi-Fi module is connected. It should be called after successfully 
 * connecting to an Access Point using the rsi_join API. 
 * 
 * @section prerequisite 
 * rsi_join should be done successfully. 				
 * 
 */
int16 rsi_query_rssi(void)
{
	int16						retval;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nRSSI Start");
#endif
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdRssi,(uint8 *)&rsi_dFrmRssi,sizeof(rsi_dFrmRssi));
	return retval;
}



