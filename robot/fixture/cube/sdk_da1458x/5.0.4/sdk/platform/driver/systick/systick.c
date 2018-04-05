/**
****************************************************************************************
*
* @file systick.c
*
* @brief SysTick driver.
*
* Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
* program includes Confidential, Proprietary Information and is a Trade Secret of 
* Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
* unless authorized in writing. All Rights Reserved.
*
* <bluetooth.support@diasemi.com> and contributors.
*
****************************************************************************************
*/

#include <stdio.h>
#include "systick.h"
#include "ARMCM0.h"

systick_callback_function_t callback_function = NULL;

/**
 ****************************************************************************************
 * @brief Register Callback function for SysTick exception.
 *
 * @param[in] callback  Callback function's reference.
 *
 * @return void
 ****************************************************************************************
 */
void systick_register_callback(systick_callback_function_t callback)
{
    callback_function = callback;
}

void $Sub$$SysTick_Handler(void)
{
    if (callback_function != NULL)
        callback_function();
}

/**
 ****************************************************************************************
 * @brief Function to start the SysTick timer
 *
 * @param[in] usec  	the duration of the countdown
 * @param[in] exception set to TRUE to generate an exception when the timer counts down
 *                      to 0, FALSE not to
 *
 * @return void
 ****************************************************************************************
 */
void systick_start(uint32_t usec, uint8_t exception)
{   
    SetBits32(&SysTick->CTRL, SysTick_CTRL_ENABLE_Msk, 0);          // disable systick
    SetBits32(&SysTick->LOAD, SysTick_LOAD_RELOAD_Msk, usec-1);     // set systick timeout based on 1MHz clock
    SetBits32(&SysTick->VAL,  SysTick_VAL_CURRENT_Msk, 0);          // clear the Current Value Register and the COUNTFLAG to 0
    SetBits32(&SysTick->CTRL, SysTick_CTRL_TICKINT_Msk, exception); // generate or not the SysTick exception
    SetBits32(&SysTick->CTRL, SysTick_CTRL_CLKSOURCE_Msk, 0);       // use a reference clock frequency of 1 MHz
    SetBits32(&SysTick->CTRL, SysTick_CTRL_ENABLE_Msk, 1);          // enable systick
}

/**
 ****************************************************************************************
 * @brief Function to stop the SysTick timer
 *
 * @param[in] void
 *
 * @return void
 ****************************************************************************************
 */
void systick_stop(void)
{
    SetBits32(&SysTick->VAL,  SysTick_VAL_CURRENT_Msk, 0);            // clear the Current Value Register and the COUNTFLAG to 0
    SetBits32(&SysTick->CTRL, SysTick_CTRL_ENABLE_Msk, 0);            // disable systick
}

/**
 ****************************************************************************************
 * @brief Function to create a delay
 *
 * @param[in] usec the duration of the delay
 *
 * @return void
 ****************************************************************************************
 */
void systick_wait(uint32_t usec) 
{
    systick_start(usec, false);

    // wait for the counter flag to become 1
    // Because the flag is cleared automatically when the register is read, there is no need to clear it
    while (!GetBits32(&SysTick->CTRL, SysTick_CTRL_COUNTFLAG_Msk));

    systick_stop();
}

/**
 ****************************************************************************************
 * @brief Function to read the current value of the timer
 *
 * @param[in] void
 *
 * @return the current value of the timer
 ****************************************************************************************
 */
uint32_t systick_value(void)
{
    return GetBits32(&SysTick->VAL, SysTick_VAL_CURRENT_Msk);
}
