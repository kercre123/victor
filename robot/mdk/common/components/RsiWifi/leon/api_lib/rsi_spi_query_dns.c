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
 * @brief SPI, DNS QUERY, Get the DNS address for the domain name
 *
 * @section Description
 * This file contains the DNS query function.
 *
 *
 */


/**
 * Includes
 */
#include "rsi_global.h"


#include <stdio.h>
#include <string.h>

/*=================================================================================*/
/**
 * @fn              int16 rsi_query_dns(rsi_uDns   *uDnsFrame)
 * @brief           QUERY dns address for the domain name given through the SPI interface
 * @param[in]       rsi_uDns   *uDnsFrame, pointer to the Dns frame structure
 * @param[out]      none
 * @return          errCode
 *                  -1 = SPI busy / Timeout
 *                  -2 = SPI Failure
 *		    0  = SUCCESS
 * @section description     
 * This API is used to query the dns ip addresses of the Domain name given.
 */
int16 rsi_query_dns(rsi_uDns  *uDnsFrame)
{
  int16					  retval;
#ifdef RSI_DEBUG_PRINT
  RSI_DPRINT(RSI_PL3,"\r\n\nDNS Query Start");
#endif
#ifdef RSI_LITTLE_ENDIAN
  *(uint16 *)&uDnsFrame->uDnsBuf = RSI_REQ_DNS_GET;
#else
  rsi_uint16To2Bytes((uint8 *)&uDnsFrame->uDnsBuf, RSI_REQ_DNS_GET);
#endif  
  retval =rsi_execute_cmd((uint8 *)rsi_frameCmdDnsGet,
                         (uint8 *)uDnsFrame,sizeof(rsi_uDns));
  return retval;
}




