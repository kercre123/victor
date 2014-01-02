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
 * @brief HAL,  Interrupt handeling
 *
 * @section Description
 * This file contain interrupt handler to spi interrupt
 *
 */

/**
 * Includes
 */
#include "rsi_global.h"
#include "swcLeonUtils.h"

/**
 * Global Variables
 */

/*===================================================*/
/**
 * @fn		void rsi_intHandler(void)
 * @brief	Sets the standard interrupt flags based on the interrupt received from the module 
 * @param[in]	none
 * @param[out]	none
 * @return	none
 * @section description 
 * When the MCU is configured for Interrupt mode. this API should be called in the 
 * Interrupt Service Routine service the interrupt from the Wi-Fi module. The interrupt 
 * signal from the Wi-Fi module is an active-high level-sensitive interrupt. 
 * So, it is recommended that the interrupt be disabled first, then this API be called 
 * and then the interrupt be enabled.In polling mode the application should call this 
 * API explicitly to update the events from the module. This API reads the content 
 * of interrupt status register and updates the events accordingly.
 * 
 * @section prerequisite 
 * MCU interrupt should be enabled, also called from polling mode
 */

void rsi_intHandler(void)
{

    rsi_strIntStatus.isrRegLiteFi = rsi_irqstatus();
#ifdef RSI_DEBUG_PRINT
    if(rsi_strIntStatus.isrRegLiteFi != 0)
    {
        RSI_DPRINT(RSI_PL3,"\r\nHost Interrupt Register=%02x ", (uint16)rsi_strIntStatus.isrRegLiteFi);
    }
#endif
    if ((rsi_pwstate.current_mode == 1) && (rsi_strIntStatus.isrRegLiteFi & RSI_IRQ_PWRMODE))
    {
        if (rsi_pwstate.ack_pwsave == 1)
        {
            rsi_clear_interrupt(RSI_ICLEAR_PWR);
            rsi_set_intr_mask(RSI_IMASK_PWR_DISABLE);
            rsi_strIntStatus.powerIrqPending = RSI_FALSE;
        }
        else
        {
            rsi_set_intr_mask(RSI_IMASK_PWR_ENABLE);
            rsi_strIntStatus.powerIrqPending = RSI_TRUE;
        }

#ifdef RSI_INTERRUPTS
        rsi_spiIrqClearPending();
      //  rsi_spiIrqEnable();
        // to enable interrupts - call it if the interrupts are disabled at the moment
#endif
    }
    if (rsi_strIntStatus.isrRegLiteFi & RSI_IRQ_BUFFERFULL)
    {
        rsi_strIntStatus.bufferFull = RSI_TRUE;
    }
    else
    {
        rsi_strIntStatus.bufferFull = RSI_FALSE;
    }
    if (rsi_strIntStatus.isrRegLiteFi & RSI_IRQ_BUFFEREMPTY)
    {
        rsi_strIntStatus.bufferEmpty = RSI_TRUE;
    }
    else
    {
        rsi_strIntStatus.bufferEmpty = RSI_FALSE;
    }
    if (rsi_strIntStatus.isrRegLiteFi & RSI_IRQ_DATAPACKET)
    {
        rsi_strIntStatus.dataPacketPending = RSI_TRUE;
    }
    else
    {
        rsi_strIntStatus.dataPacketPending = RSI_FALSE;
    }
    if (rsi_strIntStatus.isrRegLiteFi & RSI_IRQ_MGMTPACKET)
    {
        rsi_strIntStatus.mgmtPacketPending = RSI_TRUE;
    }
    else
    {
        rsi_strIntStatus.mgmtPacketPending = RSI_FALSE;
    }

}

