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
 * @section brief  
 * POWER MODE, Sets the POWER MODE via the SPI interface
 * This file contains the SPI Power Mode setting function.
 *
 *
 */

/**
 * Includes
 */
#include "rsi_global.h"

#include <stdio.h>
#include <string.h>

/**
 * Global Variables
 */
volatile rsi_powerstate rsi_pwstate = { 0, 1 };
/*===========================================================================*/
/**
 * @fn          int16 rsi_power_mode(uint8 powerMode)
 * @brief       Sends the SPI POWER MODE command to the Wi-Fi module
 * @param[in]   uint8 powerMode, power mode value	
 * @param[out]	none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 *
 * @section description 
 * This API is used to set different power save modes of the module. Please 
 * refer to the Software Programming Reference Manual for more information on 
 * these modes. This API should be called only after rsi_init API.
 * 
 * @section prerequisite  
 * Init command should be complete before this command.
 */
int16 rsi_power_mode(uint8 powerMode)
{
    int16 retval;
    rsi_uPower uPowerFrame;
    uPowerFrame.powerFrameSnd.powerVal[0] = powerMode;
    uPowerFrame.powerFrameSnd.powerVal[1] = 0;
    rsi_pwstate.current_mode = powerMode;
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\n\nPower save command");
#endif
    retval = rsi_execute_cmd((uint8 *) rsi_frameCmdPower, (uint8 *) &uPowerFrame,
            sizeof(rsi_uPower));
    return retval;
}

/*===========================================================================*/
/**
 * @fn          int16 rsi_pwrsave_hold(void)
 * @section brief       
 * This function used to temprorly hold power mode1 to send packet.after sending 
 * the packets application should call rsi_pwsave_clear to move module to full powersave.
 * @param[in]   none	
 * @param[out]	none
 * @return      errCode
 *              0  = OK
 *
 * @section description
 * This API is used to temporarily hold the module from going into sleep mode in 
 * Power Mode 1 (please refer to the Software Programming Reference Manual for more information 
 * on Power Mode 1) C this is done when the application has to send packets or commands to 
 * the module. After sending the packets/commands the application should call the 
 * rsi_pwrsave_continue API to move module back to full power save mode. This function 
 * is useful only for Power Mode 1.
 * 
 * @prerequisite  should use only for powermode 1. 
 */
int16 rsi_pwrsave_hold(void)
{
    rsi_pwstate.ack_pwsave = 0;
    return 0;
}

/*===========================================================================*/
/**
 * @fn          int16 rsi_pwrsave_continue(void)
 * @brief       To move the module to full power save mode.
 * @param[in]   none
 * @param[out]  none
 * @return      errCode
 *              0  = OK
 * @section Description
 * This API is used to move the module back to full power save mode after the 
 * data/command packets are transmitted by the application, which follows the call 
 * to the rsi_pwrsave_hold API. This API may be used only in Power Mode 1.
 *
 * @section prerequisite  
 * should use only for powermode 1. 
 */
int16 rsi_pwrsave_continue(void)
{
//#ifdef RSI_INTERRUPTS
    if (rsi_checkPowerModeIrq() == RSI_TRUE)
    {
        // Make the module to go to sleep
        rsi_clear_interrupt(RSI_ICLEAR_PWR);
        rsi_strIntStatus.powerIrqPending = RSI_FALSE;
        // Unmask the interrupt
        rsi_set_intr_mask(RSI_IMASK_PWR_DISABLE);
    }
//#endif
    rsi_pwstate.ack_pwsave = 1;
    return 0;
}

