/**
 * @file
 * @version		2.3.0.0
 * @date 		2011-November-11
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief QUERY Mac address, this file contain API to query mac address.
 *
 * @section Description
 * This file contains the QUERY MAC ADDRESS function.
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
 * @fn           int16 rsi_query_macaddress(void)
 * @brief        SPI, QUERY MAC address 
 * @param[in]    none
 * @param[out]   none
 * @return       errCode
 *               -1 = SPI busy / Timeout
 *               -2 = SPI Failure
 *               0  = SUCCESS
 * @section description  
 * This API is used to query the mac address of the Wi-Fi module.
 */
int16 rsi_query_macaddress(void)
{
	int16						retval;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nQuery mac address Start");
#endif
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdQryMacAddress,(uint8 *)rsi_dFrmQryMacAddress,sizeof(rsi_dFrmQryMacAddress));
	return retval;
}




