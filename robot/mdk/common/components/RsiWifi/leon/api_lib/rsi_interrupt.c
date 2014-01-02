/**
 * @file
 * @version		2.1.0.0
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
 * @brief Top level set INTERRUPT handler
 * 
 * @section Description
 * This file contains the top level Interrupt handler.
 * The non hardware aspects of the interrupts are handled here
 * These functions are only valid in the SPI context
 *
 */


/**
 * @Interrupt Definitions
 *
 * @0x2200000	= Interrupt Mask/Clear Register base address
 * @0x08		= INTR_MASK Register offset
 * @0x10		= INTR_CLEAR Register offset
 *
 * @0x08000000	= Interrupt & Spi Mode Register base address
 * @0x00		= SPI_HOST_INTR Register offset (read the interrupt status from here)
 * @0x08		= SPI_MODE Register offset
 *
 * @Interrupt Register (SPI_HOST_INTR) bits
 * @Bit 0		Buffer Full	
 * If NOT set, you can send data, masked by default, must be polled before sending data
 * @Bit 1		Buffer Empty	
 * Buffer Empty, Only becomes active after a buffer full event
 * @Bit 2		Management Packet Pending   
 * Indicates a MANAGEMENT packet is pending, Self Clearing, cleared after management packt is read
 * @Bit 3		Data Packet Pending	    
 * Indicates a DATA packet is  pending, Self Clearing, cleared after data packet is read
 * @Bit 4		NA
 * @Bit 5		
 * Power Mode If set, module is awake and requesting to shut down, setting bit 5 of the INTR_CLEAR register
 * clears the interrupt and allows the module to shut down. Switching to Power Mode 0, clears this interrupt.
 * @Bit 6		NA
 * @Bit 7		NA
 */



/**
 * Includes
 */
#include "rsi_global.h"

/**
 * Global Variables
 */


/*===================================================*/
/**
 * @fn          uint8 rsi_checkPktIrq(void)
 * @brief       Returns the status of the received data interrupt
 * @param[in]   none
 * @param[out]  none
 * @return      uint8 rsi_strIntStatus.dataPacketPending
 * @section description
 * This API is used to read the status of the data/command 
 * packet pending interrupt event.It is the responsibility of the Application 
 * to monitor data pending event regularly by calling this function, 
 * If any packet is pending application to call to rsi_read_packet API to retrieve 
 * the pending packet from module and handle accordingly.
 */
uint8 rsi_checkPktIrq(void)
{
#ifdef RSI_POLLED
	rsi_intHandler();
#endif
	return rsi_strIntStatus.dataPacketPending;
}


/*===================================================*/
/**
 * @fn         void rsi_clearPktIrq(void)
 * @brief      Clears the received data interrupt flag
 * @param[in]  none
 * @param[out] none
 * @return     none
 * @section description
 * This API is used to clear data packet/command pending interrupt event. 
 * It should be called by the application after servicing the event when 
 * it is detected using the rsi_checkPktIrq API.
 */
void rsi_clearPktIrq(void)
{
	rsi_strIntStatus.dataPacketPending = RSI_FALSE;
}


/*===================================================*/
/**
 * @fn          uint8 rsi_checkBufferFullIrq(void)
 * @brief       Returns the status of the Tx buffer full interrupt flag
 * @param[in]   none
 * @param[out]  none
 * @return      uint8 rsi_strIntStatus.bufferFull
 * @section description
 * This API is used to read the status of the Buffer Full event. 
 * If the buffer full event is set then the application should not send 
 * any packet/command to the module until it is cleared.
 */
uint8 rsi_checkBufferFullIrq(void)
{
	// We always have to poll for buffer full
	rsi_intHandler();

	return rsi_strIntStatus.bufferFull;
}

/*===================================================*/
/**
 * @fn          uint8 rsi_checkIrqStatus(void)
 * @brief       Returns the raw interrupt status register
 * @param[in]   none
 * @param[out]  none
 * @return      uint8 rsi_strIntStatus.isrRegLiteFi
 * @section description 
 * This API is used to read the bitmap of all the pending 
 * events (value of the interrupt status register) from the Wi-Fi module.
 * This application can use this API to monitor all the events at a time.
 */
uint8 rsi_checkIrqStatus(void)
{
#ifdef RSI_POLLED
	rsi_intHandler();
#endif
	return rsi_strIntStatus.isrRegLiteFi;
}

/*===================================================*/
/**
 * @fn          uint8 rsi_checkPowerModeIrq(void)
 * @brief       Returns the status of the Power Mode Interrupt
 * @param[in]   none
 * @param[out]  none
 * @return      uint8 rsi_strIntStatus.powerIrqPending
 * @section description This API is used to read the status of the power save interrupt event. 
 * It is used only in Power Mode 1 (please refer to the Software Programming Reference Manual 
 * for more information on Power Modes).When this event is raised then only application 
 * can send command/data to module in powesave mode1.
 */
uint8 rsi_checkPowerModeIrq(void)
{
#ifdef RSI_POLLED
	rsi_intHandler();
#endif
	return rsi_strIntStatus.powerIrqPending;
}


