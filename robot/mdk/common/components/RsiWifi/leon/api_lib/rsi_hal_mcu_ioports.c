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
 * 
 * @section Description
 * This file contains API to control different pins of the microcontroller 
 * which interface with the module and other components related to the module. 
 *
 */

/**
 * Includes
 */
#include "rsi_global.h"
#include "DrvGpio.h"

/**
 * Global Variales
 */



/*====================================================*/
/**
 * @fn          void rsi_moduleReset(uint8 tf)
 * @brief       Sets or clears the reset pin on the wifi module
 * @param[in]   uint8 tf, true or false, true sets reset, false clears reset
 * @param[out]  none
 * @return      none
 * @section description 
 * This HAL API is used to set or clear the active-low reset pin of the Wi-Fi module.
 */
void rsi_moduleReset(uint8 tf)
{	
  DrvGpioSetPin( RSI_RESET_PIN, !tf);
}

/*====================================================*/
/**
 * @fn          void rsi_modulePower(uint8 tf)
 * @brief       Turns on or off the power to the wifi module
 * @param[in]   uint8 tf, true or false, true turns on power, false turns off power
 * @param[out]  none
 * @return      none
 * @section description 
 * This HAL API is used to turn on or off the power to the Wi-Fi module.
 */
void rsi_modulePower(uint8 tf)
{	
	if (tf == RSI_TRUE) {
        ;//there's no pin to this
	}
	else {
        ;
	}
}

