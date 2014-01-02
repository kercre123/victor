/**
 * @file
 * @version		2.3.0.0
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
 * This file contains the list of functions for configuring the microcontroller interrupts. 
 * Following are list of APICs which need to be defined in this file.
 *
 */

/**
 * Includes
 */
#include <DrvGpio.h>
#include <DrvIcb.h>

#include "rsi_global.h"

/*===================================================*/
/**
 * @fn          void rsi_spiIrqStart(void)
 * @brief       Starts and enables the SPI interrupt
 * @param[in]   none
 * @param[out]  none
 * @return      none
 * @section description 
 * This HAL API should contain the code to initialize the register related to interrupts.
 */
#ifndef GPIO_INTERRUPT_LEVEL
#define GPIO_INTERRUPT_LEVEL 0
#endif


void rsi_spiIrqStart(void)
{
    DrvGpioIrqConfig(RSI_IRQ_PIN, D_GPIO_IRQ_SRC_1, POS_LEVEL_INT, GPIO_INTERRUPT_LEVEL,
            (void *) rsi_intHandler);
}

/*===================================================*/
/**
 * @fn          void rsi_spiIrqDisable(void)
 * @brief       Disables the SPI Interrupt
 * @param[in]   none
 * @param[out]  none
 * @return      none
 * @section description 
 * This HAL API should contain the code to disable interrupts.
 */
void rsi_spiIrqDisable(void)
{
    DrvIcbDisableIrq(D_GPIO_IRQ_SRC_1);
}

/*===================================================*/
/**
 * @fn          void rsi_spiIrqEnale(void)
 * @brief       Enables the SPI interrupt
 * @param[in]   none	
 * @param[out]  none
 * @return      none
 * @section description 
 * This HAL API should contain the code to enable interrupts.
 */

void rsi_spiIrqEnable(void)
{
    DrvIcbEnableIrq(D_GPIO_IRQ_SRC_1);
}

/*===================================================*/
/**
 * @fn           void rsi_spiIrqClearPending(void)
 * @brief        Clears the pending SPI interrupt
 * @param[in]    none
 * @param[out]   none
 * @return       none
 * @section description  
 * This HAL API should contain the code to clear the handled interrupts.
 */
void rsi_spiIrqClearPending(void)
{
    DrvIcbIrqClear(D_GPIO_IRQ_SRC_1);
}

