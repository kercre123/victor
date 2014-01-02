/*
 * @file
 * @version		  2.3.0.0
 * @date 		  2011-November-11
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief SPI, STORE CONFIG, Store Config Feature via the SPI interface
 *
 * @section Description
 * This file contains the SPI Store config function.
 *
 *
 */
 
 /**
 * Includes
 */
#include "rsi_global.h"

#include <stdio.h>
#include <string.h>


/*===========================================================================
 *
 * @fn			int16 rsi_store_config_enable(uint8 uFlagval)
 * @brief		Sends the Band command to the Wi-Fi module
 * @param[in]   uint8 uFlagval, config flag value for store configuration,0 for disable and 1 for enable.
 * @param[out]	none
 * @return		errCode
 *				-1 = SPI busy / Timeout
 *             	-2 = SPI Failure
 *				0  = SUCCESS
 * @description : This API is used to enable or disable the store configuration feature.  
 * This API has to be called only after the rsi_join API success.
 */
int16 rsi_store_config_enable(uint8 uFlagval)
{
  int16			retval;
  rsi_uCfgEnable				uCfgEnableFrame;
  #ifdef RSI_DEBUG_PRINT
  RSI_DPRINT(RSI_PL3,"\r\n\nCfg Enable Start");
  #endif
  uCfgEnableFrame.CfgEnableFrameSnd.uFlagval = uFlagval;
  retval =rsi_execute_cmd((uint8 *)rsi_frameCmdCfgEnable,(uint8 *)&uCfgEnableFrame,sizeof(rsi_uCfgEnable));
  return retval;
}


/*==============================================*/
/**
 * @fn				int16 rsi_store_config_save(void)
 * @brief			Stores the present configuration of WiFi device
 * @param[in]      none
 * @param[out]      none
 * @return			errCode
 *					-1 = SPI busy / Timeout
 *             		-2 = SPI Failure
 *					0  = SUCCESS
 * @description 	This API stores the present configuration of the 
 * device. This API has to be called only after the rsi_join API.
 */
int16 rsi_store_config_save(void)
{
  int16						retval;
#ifdef RSI_DEBUG_PRINT
  RSI_DPRINT(RSI_PL3,"\r\n\nStore config save Start");
#endif
  retval =rsi_execute_cmd((uint8 *)rsi_frameCmdCfgSave,NULL,0);
  return retval;
}

/*===================================================*/
/**
 * @fn		 int16 rsi_store_config_get(void)
 * @brief	 Sends the store config get command to the Wi-Fi module
 * @param[in]	 none
 * @param[out]	none
 * @return		errCode
 *				-1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *				 0  = SUCCESS
 * @description	 This function is used to get the configuration parameters stored 
 * in the Wi-Fi module.
 * 
 * @prerequisite rsi_store_config_save should be done successfully. 				
 * 
 */
int16 rsi_store_config_get(void)
{
  int16						retval;
#ifdef RSI_DEBUG_PRINT
  RSI_DPRINT(RSI_PL3,"\r\n\nstore config get start");
#endif
  retval =rsi_execute_cmd((uint8 *)rsi_frameCmdCfgGet,NULL,0);
  return retval;
}
