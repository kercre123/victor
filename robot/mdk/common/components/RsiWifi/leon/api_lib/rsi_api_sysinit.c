/** 
 * @file
 * @version		2.2.0.0
 * @date 		2011-Feb-25
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief SYS INIT, Top level system initializations
 * 
 * @section Description
 * Any initializations which are not module or platform specific are done here
 * Any plaform or module specific initializations are called from here
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


/*===========================================================*/
/**
 * @fn          int16 rsi_sys_init()
 * @brief       SYSTEM INIT, Initializes the system specific hardware
 * @param[in]	none
 * @param[out]	none
 * @return      errCode
 *              -2 = SPI failure
 *		-1 = SPI busy / Timeout
 *		0  = OK
 * @section description 
 * This API is used to initialize the module and its SPI interface.
 */
int16 rsi_sys_init()
{
	int16						retval;			// generic return value

	// cycle the power to the module
	retval = rsi_module_power_cycle();
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT( RSI_PL3,"\r\n\nSPI Interface Init ");
#endif  
	// Init the module SPI interface, can only be done once after power on
	retval =rsi_spi_iface_init();
	if(retval != 0)
	{
		return retval;
	}
	// Initialize the interrupt variables
	rsi_strIntStatus.mgmtPacketPending	= RSI_FALSE;			// TRUE for management packet pending (reserved for future)
	rsi_strIntStatus.dataPacketPending	= RSI_FALSE;			// TRUE for data/mgmt packet pending in module
	rsi_strIntStatus.powerIrqPending	  = RSI_FALSE;			// TRUE for power interrupt pending in the module
	rsi_strIntStatus.bufferFull		      = RSI_FALSE;		// TRUE=Cannot send data, FALSE=Ok to send data
	rsi_strIntStatus.bufferEmpty		    = RSI_TRUE;			// TRUE, Tx buffer empty
	return retval;
}



/*===========================================================*/
/**
 * @fn          int16 rsi_module_power_cycle(void)
 * @brief       MODULE POWER ON, Power cycles the module
 * @param[in]   none
 * @param[out]  none
 * @return	errCode
 *		-1 = Error
 *		0  = OK
 * @section description 
 * This API is used to power cycle the module. This API is valid only if 
 * there is a power gate, external to the module, which is controlling the 
 * power to the module using a GPIO signal of the MCU.
 */
int16 rsi_module_power_cycle(void)
{
	rsi_modulePower(RSI_FALSE);						//@ Start off with power off
	rsi_moduleReset(RSI_TRUE);						// Start off in reset
	
	rsi_delayMs(10);							// Let everything stabilize
	
	rsi_modulePower(RSI_TRUE);						// Turn on the Power
	
	rsi_delayMs(20);							// Let the power stabilize for 100ms
	
	rsi_moduleReset(RSI_FALSE);						// Release reset
	return 0;
}

