/**
 ****************************************************************************************
 *
 * @file app_button_led.c
 *
 * @brief Push Button and LED user interface
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */
 
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "global_io.h"
#include "gpio.h"
#include <core_cm0.h>
#include "app_button_led.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/

/**
 ****************************************************************************************
 * @brief Start red led blinking
 ****************************************************************************************
 */
void red_blink_start(void)
{
  SetWord16(P13_MODE_REG,PID_PWM0|OUTPUT);   // set pin as PWM  
  SetBits16(CLK_PER_REG, TMR_DIV, 3);        // 2 MHz input clock
  SetBits16(CLK_PER_REG, TMR_ENABLE, 1);     // enable clock
  SetBits16(TIMER0_CTRL_REG,PWM_MODE,0);     // normal PWM output
  SetBits16(TIMER0_CTRL_REG,TIM0_CLK_SEL,0); // Select 32kHz input clock
  SetBits16(TIMER0_CTRL_REG,TIM0_CLK_DIV,0); // Div by 10 input clock. so 200KHz for OnCounter

  SetWord16(TIMER0_RELOAD_M_REG,16000);      // PWM DC 50% with freq 2MHz/5K = 400Hz  
  SetWord16(TIMER0_RELOAD_N_REG,16000);

  SetBits16(TIMER0_CTRL_REG,TIM0_CTRL,1);    // Enable timer
}

/**
 ****************************************************************************************
 * @brief Start slow green led blinking
 ****************************************************************************************
 */
void green_blink_slow(void)
{
  SetWord16(P12_MODE_REG,PID_PWM0|OUTPUT);   // set pin as PWM  
  SetBits16(CLK_PER_REG, TMR_DIV, 3);        // 2 MHz input clock
  SetBits16(CLK_PER_REG, TMR_ENABLE, 1);     // enable clock
  SetBits16(TIMER0_CTRL_REG,PWM_MODE,0);     // normal PWM output
  SetBits16(TIMER0_CTRL_REG,TIM0_CLK_SEL,0); // Select 32kHz input clock
  SetBits16(TIMER0_CTRL_REG,TIM0_CLK_DIV,0); // Div by 10 input clock. so 200KHz for OnCounter

  SetWord16(TIMER0_RELOAD_M_REG,24000);      // PWM DC 50% with freq 2MHz/5K = 400Hz  
  SetWord16(TIMER0_RELOAD_N_REG,24000);

  SetBits16(TIMER0_CTRL_REG,TIM0_CTRL,1);    // Enable timer
}

/**
 ****************************************************************************************
 * @brief Start fast green led blinking
 ****************************************************************************************
 */
void green_blink_fast(void)
{
  SetWord16(P12_MODE_REG,PID_PWM0|OUTPUT);   // set pin as PWM  
  SetBits16(CLK_PER_REG, TMR_DIV, 3);        // 2 MHz input clock
  SetBits16(CLK_PER_REG, TMR_ENABLE, 1);     // enable clock
  SetBits16(TIMER0_CTRL_REG,PWM_MODE,0);     // normal PWM output
  SetBits16(TIMER0_CTRL_REG,TIM0_CLK_SEL,0); // Select 32kHz input clock
  SetBits16(TIMER0_CTRL_REG,TIM0_CLK_DIV,0); // Div by 10 input clock. so 200KHz for OnCounter

  SetWord16(TIMER0_RELOAD_M_REG,8000);       // PWM DC 50% with freq 2MHz/5K = 400Hz  
  SetWord16(TIMER0_RELOAD_N_REG,8000);

  SetBits16(TIMER0_CTRL_REG,TIM0_CTRL,1);    // Enable timer
}

/**
 ****************************************************************************************
 * @brief Turn off blinking led
 ****************************************************************************************
 */
void turn_off_led(void)
{
  SetBits16(TIMER0_CTRL_REG,TIM0_CTRL,0);    // Disable Timer 
  SetWord16(P12_MODE_REG,PID_GPIO|INPUT);    // set pin as GPIO input
  SetWord16(P13_MODE_REG,PID_GPIO|INPUT);    // set pin as GPIO input  	
  SetBits16(CLK_PER_REG, TMR_ENABLE, 0);     // disable clock
  SetWord16(TIMER0_RELOAD_M_REG,0);          // set zero to M reg
  SetWord16(TIMER0_RELOAD_N_REG,0);          // set zero to N reg
  SetWord16(TIMER0_ON_REG,0);                // set zero to On reg
  SetBits16(CLK_PER_REG, TMR_ENABLE, 10);    // close clock
}

/**
 ****************************************************************************************
 * @brief Register and enable button pressed interrupt
 ****************************************************************************************
 */
void app_button_enable(void)
{   
  NVIC_DisableIRQ(GPIO1_IRQn);
  
  // set isr callback for button pressed interrupt
  GPIO_RegisterCallback(GPIO1_IRQn, button_isr);
  
  //Push Button input
  GPIO_EnableIRQ(GPIO_PORT_1, GPIO_PIN_1, GPIO1_IRQn, true, true, 0);
}

/**
 ****************************************************************************************
 * @brief Button pressed interrupt service routine
 ****************************************************************************************
 */
void button_isr(void)
{
    NVIC_DisableIRQ(GPIO1_IRQn);    // Disable this interrupt
    GPIO_ResetIRQ(GPIO1_IRQn);      // clear pending interrupt
    
    turn_off_led();
    
    NVIC_ClearPendingIRQ(GPIO1_IRQn);	// Clear interrupt requests while disabled
    NVIC_EnableIRQ(GPIO1_IRQn);       // Enable this interrupt
}
