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
 * @brief SPI, MODE SELECT, Sets the MODE (21/22) value via the SPI interface
 *
 * @section Description
 * This file contains the SPI module TCP/NO TCP mode Select function.
 *
 *
 */



/**
 * Includes
 */
#include "rsi_global.h"

#include <stdio.h>
#include <string.h>

/*==============================================*/
/**
 * @fn          int16 rsi_spi_mode_sel(uint8 mode)
 * @brief       Sends the ModeSelect command to the Wi-Fi module
 * @param[in]   uint8 mode, mode value to configure,21 for RS21 i.e NO TCP
 *              and 22 for RS22 i.e with TCP/IP stack.
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description
 * This API is used to select module mode.
 * This API is used to configure the module to RS21 (NO TCP) or RS22(With TCP).
 * By default(Without calling this API), the module operates as RS22.
 * This API has to be called only before or after the rsi_band API.
 */
int16 rsi_spi_mode_sel(uint8 mode)
{
	int16               retval;
	rsi_uModeSel        uModeSelFrame;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nModeSelect Start");
#endif
	uModeSelFrame.ModeSelFrameSnd.ModeSelVal = mode;
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdModeSel,(uint8 *)&uModeSelFrame,sizeof(rsi_uModeSel));
	return retval;
}


