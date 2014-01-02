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
 * @brief SPI, IP PARAMETERS, Set Network parameters, IP Address, Port Number
 *
 * @section Description
 * This file contains the SPI TCPIP function.
 *
 *
 */



/**
 * Includes
 */
#include "rsi_global.h"



#include <stdio.h>
#include <string.h>


/*===============================================*/
/**
 * @fn          int16 rsi_ipparam_set(rsi_uIpparam *uIpparamFrame)
 * @brief       Sends the IPCONFIG (TCP/IP configure)command to the Wi-Fi module
 * @param[in]   rsi_uIpparam *uIpparamFrame, pointer to ip configuration stucture
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description  
 * This API is used to configure the IP address, Subnet Mask 
 * and Gateway IP address for the module in manual mode or DHCP mode. 
 * The Wi-Fi module should be successfully connected to an Access point 
 * (using the rsi_join API) before calling this API. If DHCP mode is enabled, then 
 * it has to be ensured that a DHCP server is present in the network.
 * 
 * @section prerequisite 
 * rsi_join should be done successfully
 */
int16 rsi_ipparam_set(rsi_uIpparam *uIpparamFrame)
{
	int16						retval;
	// frame sent to set the IP parameters
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\n IPconfiguration Start");
#endif
	// Copy in the IPPARAM Frame Data from the global copy
#ifdef RSI_LITTLE_ENDIAN
	*(uint16 *)uIpparamFrame->uIpparamBuf = RSI_REQ_IPPARAM_CONFIG;
#else
	rsi_uint16To2Bytes((uint8 *)uIpparamFrame->uIpparamBuf, RSI_REQ_IPPARAM_CONFIG);	
#endif  
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdIpparam,(uint8 *)uIpparamFrame,sizeof(rsi_uIpparam));
	return retval;
}



