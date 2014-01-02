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
 * @brief SPI, BAND, Sets the BAND value via the SPI interface
 *
 * @section Description
 * This file contains the SPI Band function.
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
 * @fn          int16 rsi_band(uint8 band)
 * @brief       Sends the Band command to the Wi-Fi module
 * @param[in]   uint8 band, band value to configure,0 for 2.4GHz and 1 for 5GHz.
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This API is used to select the 2.4 GHz mode or 5GHz mode. 
 * This API is required only for the RS9110-N-11-26 and RS9110-N-11-28 modules 
 * which have an option of operating in either the 2.4GHz or 5GHz modes. 
 * It is optional for the RS9110-N-11-22 and RS9110-N-11-24 modules 
 * which can operate in the 2.4GHz band only. By default, all modules operate 
 * in the 2.4GHz mode. This API has to be called only after the rsi_bootloader API.
 */
int16 rsi_band(uint8 band)
{
	int16               retval;
	rsi_uBand           uBandFrame;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nBand Start");
#endif
	uBandFrame.bandFrameSnd.bandVal = band;
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdBand,(uint8 *)&uBandFrame,sizeof(rsi_uBand));
	return retval;
}


