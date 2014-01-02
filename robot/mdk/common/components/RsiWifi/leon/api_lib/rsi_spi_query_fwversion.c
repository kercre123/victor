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
 * @brief QUERY FIRMWARE VERSION, Get the status of an existing connection
 *
 * @section Description
 * This file contains the QUERY FIRMWARE VERSION function.
 *
 *
 */


/**
 * Includes
 */
#include "rsi_global.h"



#include <stdio.h>
#include <string.h>


/*=====================================================*/
/**
 * @fn           int16 rsi_query_fwversion(void)
 * @brief        SPI, QUERY FIRMWARE VERSION 
 * @param[in]    none
 * @param[out]   none
 * @return       errCode
 *               -1 = SPI busy / Timeout
 *               -2 = SPI Failure
 *               0  = SUCCESS
 * @section description  
 * This API is used to query the firmware version of the Wi-Fi module.
 */
int16 rsi_query_fwversion(void)
{
	int16						retval;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nQuery FWVersion Start");
#endif
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdQryFwVer,(uint8 *)rsi_dFrmQryFwVer,sizeof(rsi_dFrmQryFwVer));
	return retval;
}




