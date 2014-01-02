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
 * @brief SPI, FEATURE SELECT, Selects the Features based on the bitmap value
 * sent via the SPI interface
 *
 * @section Description
 * This file contains the SPI feature select function.
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
 * @fn          int16 rsi_spi_feat_sel(uint8 *bitmap)
 * @brief       Sends the Feature Select command to the Wi-Fi module
 * @param[in]   uint8 *bitmap, Pointer to the feature select bitmap
 *              array
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description
 * This API is used to select features by sending a bitmap value via SPI
 * interface. This API is used to configure the module to RS21 (NO TCP) or RS22(With TCP).
 * By default(Without calling this API), the module operates as RS22.
 * For the information of the bitmap value that needs to be send to enable 
 * particular feature refer to the software PRM.
 * This API has to be called only before or after the rsi_band API.
 */
int16 rsi_spi_feat_sel(uint8 bitmap)
{
	int16               retval;
	rsi_uFeatSel        uFeatSelFrame;
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nFeatureSelect Start");
#endif
    memset(&uFeatSelFrame,0,sizeof(rsi_uFeatSel));
	uFeatSelFrame.FeatSelFrameSnd.FeatSelBitmap[0] = bitmap;
	retval =rsi_execute_cmd((uint8 *)rsi_frameCmdFeatSel,(uint8 *)&uFeatSelFrame,sizeof(rsi_uFeatSel));
	return retval;
}


