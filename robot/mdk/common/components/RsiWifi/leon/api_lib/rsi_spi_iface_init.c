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
 * @brief SPI, INIT, initiaizes the SPI hardware interface in the module
 *
 * @section Description
 * This file contains the SPI Initialization function.
 * This function enables the SPI interface on the Wi-Fi chip.
 * This function is only run once during startup, after power-on, or reset
 *
 *
 */


/**
 * Includes
 */
#include "rsi_global.h"
#include <stdio.h>


/**
 * Global Variables
 */


/*=============================================*/
/**
 * @fn                  int16 rsi_spi_iface_init(void)
 * @brief               Start the SPI interface
 * @param[in]           none
 * @param[out]          none
 * @return              errCode
 *                      -1 = SPI busy / Timeout
 *                      -2 = SPI Failure
 *                      0  = SUCCESS
 * @section description         
 * This API initializes the Wi-Fi moduleCs Slave SPI interface.
 */
int16 rsi_spi_iface_init(void)
{	
	uint8                   txCmd[2];												
	int16                   retval = 0;												
	uint16                  txLen;													
	uint16                  timeout;												
	uint8                   localBuf[4];

	txLen			= sizeof(txCmd);	// number of bytes to send
	timeout			= 10;			// 10ms timeout on command, nothing magic, just a reasonable number
	RSI_RESET_TIMER1;			        // init the timer counter
	while(1) {
		if (RSI_INC_TIMER_1 > timeout) {			
			retval = RSI_BUSY;
#ifdef RSI_DEBUG_PRINT
			RSI_DPRINT(RSI_PL15,"\r\nspi_iface_init Failure due to timeout");
#endif
			break;
		}

		txCmd[0]	= RSI_C1_INIT_CMD;			
		txCmd[1]	= 0x00;								


		retval = rsi_spiSend(txCmd, txLen,localBuf,RSI_MODE_8BIT);
		if (localBuf[1] == RSI_SPI_SUCCESS) {									
			retval = RSI_SUCCESS;														
			break;
		}
		else {
#ifdef RSI_DEBUG_PRINT
			RSI_DPRINT(RSI_PL15,"SpiInitErr=%02x ", (uint16)localBuf[1]);
#endif      
			retval = RSI_BUSY;
		}
		rsi_delayMs(20);			// Not necessary
	}

	return retval;
}


