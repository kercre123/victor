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
 *
 * @section Description
 * This file implements MCU related timer functions.
 *
 */


/**
 * Include files
 */
#include "rsi_global.h"


#include <stdlib.h>
#include <stdio.h>
#include <DrvTimer.h>



/**
 * Global define
 */


/*===============================================================*/
/**
 * @fn          void rsi_delayMs (uint16 delay)
 * @brief       Delays by an integer number of milliseconds
 * @param[in]   delay Number of milliseconds to delay
 * @param[out]  none
 * @return      none
 * @section description 
 * This HAL API should contain the code to introduce a delay in milliseconds.
 */
void rsi_delayMs (uint16 delay)
{
    SleepMs( delay );
}


/*===============================================================*/
/**
 * @fn          void rsi_delayUs(uint16 delay)
 * @brief       Delays by an integer number of microseconds
 * @param[in]   uint16 delay, Number of microseconds to delay
 * @param[out]  none
 * @return      none
 * @section description 
 * This HAL API should contain the code to introduce a delay in microseconds.
 */
void rsi_delayUs (uint16 delay)
{

    SleepMicro(delay);
}
