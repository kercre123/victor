/**
 * @file
 * @version		2.2.0.0
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
 * @brief SPI, SET MAC ADDRESS
 * @section Description
 * This file contains function to set mac address.
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
 * @fn          int16 rsi_set_mac_addr(uint8 *macAddress)
 * @brief       Sends the set MAC Address command to the Wi-Fi module via SPI
 * @param[in]	uint8 *macAddress
 * @param[out]	none
 * @return      errCode
 *		-1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description  
 * This API is used to override the MAC address provided by the module. It should be 
 * called before the rsi_join API.
 * @section prerequisite  
 * should set before join command
 */
int16 rsi_set_mac_addr(uint8 *macAddress)
{
	int16                                   retval;
	rsi_uSetMacAddr                         uSetMacAddrFrame;	// frame sent as the join command
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\nSetMacAddr Start ");
#endif
#ifdef RSI_LITTLE_ENDIAN
	*(uint16 *) uSetMacAddrFrame.setMacAddrFrameSnd.setMacAddrCmd = *(uint16 *) rsi_dFrmSetMacAddr; 
#else
	memcpy(&uSetMacAddrFrame, rsi_dFrmSetMacAddr, RSI_DFRAMECMDLEN);
#endif
	//send the MAC Address
	memcpy(uSetMacAddrFrame.setMacAddrFrameSnd.macAddr,macAddress, RSI_MACADDRLEN);
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdSetMacAddr,(uint8 *)&uSetMacAddrFrame,sizeof(rsi_uSetMacAddr));
	return retval;
}


