/**
 ****************************************************************************************
 *
 * @file timer.c
 *
 * @brief Software Timer routines
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "global_io.h"
#include "gpio.h"
#include <core_cm0.h>

void sw_timer_init(void)
{
    SetWord16(P28_MODE_REG,PID_PWM0|OUTPUT); // set pin as PWM  
    SetWord16(P29_MODE_REG,PID_PWM1|OUTPUT); // set pin as PWM  
    SetBits16(CLK_PER_REG, TMR_DIV, 3);     // 2 MHz input clock
    SetBits16(CLK_PER_REG, TMR_ENABLE, 1);  // enable clock
    SetBits16(TIMER0_CTRL_REG,PWM_MODE,0);  // normal PWM output
    SetBits16(TIMER0_CTRL_REG,TIM0_CLK_SEL,1);  // Select 2MHz input clock
    SetBits16(TIMER0_CTRL_REG,TIM0_CLK_DIV,0);  // Div by 10 input clock. so 200KHz for OnCounter

    // PWM DC 50% with freq 2MHz/5K = 400Hz  
    SetWord16(TIMER0_RELOAD_M_REG,2500);
    SetWord16(TIMER0_RELOAD_N_REG,2500);

    SetWord16(TIMER0_ON_REG,50000); // one IRQ per 250 ms,  50000/200KHz
    NVIC_EnableIRQ(SWTIM_IRQn);     // enable interrupt
    SetBits16(TIMER0_CTRL_REG,TIM0_CTRL,1); // Enable timer
}


void sw_timer_release(void)
{
    SetBits16(TIMER0_CTRL_REG,TIM0_CTRL,0);     // Disable Timer 
    NVIC_DisableIRQ(SWTIM_IRQn);                // disable interrupt
    SetWord16(P28_MODE_REG,PID_GPIO|INPUT);     // set pin as GPIO input  
    SetBits16(CLK_PER_REG, TMR_ENABLE, 0);      // disable clock
    SetWord16(TIMER0_RELOAD_M_REG,0);           // set zero to M reg
    SetWord16(TIMER0_RELOAD_N_REG,0);           // set zero to N reg
    SetWord16(TIMER0_ON_REG,0);                 // set zero to On reg
    SetBits16(CLK_PER_REG, TMR_ENABLE, 10);     // close clock
}
