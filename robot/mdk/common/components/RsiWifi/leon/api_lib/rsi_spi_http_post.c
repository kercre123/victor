/**
 * @file
 * @version		2.3.0.0
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
 * @brief SPI HTTP, Post the data for HTTP of the post URL
 *
 * @section Description
 * This file contains the SPI HTTP POST function.
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
 * @fn           int16 rsi_spi_http_post(void)
 * @brief        SPI, HTTP Post command 
 * @param[in]    rsi_uHttpReq *uHttpReqFrame, pointer to http request frame
 * @param[out]   none
 * @return       errCode
 *               -1 = SPI busy / Timeout
 *               -2 = SPI Failure
 *               0  = SUCCESS
 * @section description  
 * This API is used to transmit an HTTP POST request to an HTTP server.
 * The HTTP POST Request can be issued only in a serial manner meaning 
 * only if HTTP response is obtained for the current request, the next 
 * Request can be issued
 */
int16 rsi_spi_http_post(rsi_uHttpReq *uHttpReqFrame)
{
	int16						retval;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nGet HTTP Start");
#endif
#ifdef RSI_LITTLE_ENDIAN
   *(uint16 *)&uHttpReqFrame->uHttpReqBuf = RSI_REQ_HTTP_POST;
#else
   rsi_uint16To2Bytes((uint8 *)&uHttpReqFrame->uHttpReqBuf, RSI_REQ_HTTP_POST);
#endif  
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdReqHttp,(uint8 *)uHttpReqFrame,sizeof(rsi_uHttpReq));
	return retval;
}
