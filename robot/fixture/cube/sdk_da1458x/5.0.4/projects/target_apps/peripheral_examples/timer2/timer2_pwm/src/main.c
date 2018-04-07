/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Timer2 example for DA14580/581 SDK
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
#include <stdio.h>
#include <stdbool.h>
#include "global_io.h"
#include "common_uart.h"
#include "user_periph_setup.h"
#include "global_io.h"
#include "pwm.h"

#define TIMER0_TEST_EXPIRATION_COUNTER_RELOAD_VALUE 50

void system_init(void);
void timer2_test(void);
void simple_delay(void);

/**
 ****************************************************************************************
 * @brief  Main routine of the timer2 example 
 * 
 ****************************************************************************************
 */
int main (void)
{
    system_init();
    periph_init();
    timer2_test();
    while(1);
}

/**
 ****************************************************************************************
 * @brief System Initiialization 
 *
 * 
 ****************************************************************************************
 */

void system_init(void)
{
    SetWord16(CLK_AMBA_REG, 0x00);                 // set clocks (hclk and pclk ) 16MHz
    SetWord16(SET_FREEZE_REG,FRZ_WDOG);            // stop watch dog    
    SetBits16(SYS_CTRL_REG,PAD_LATCH_EN,1);        // open pads
    SetBits16(SYS_CTRL_REG,DEBUGGER_ENABLE,1);     // open debugger
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP,0);       // exit peripheral power down
}

void timer2_test(void)
{
    uint8_t i;
    printf_string("\n\r\n\r***************");
    printf_string("\n\r* TIMER2 TEST *\n\r");
    printf_string("***************\n\r");


    //Due to the fact that P06(WKUP_TEST_BUTTON_1_PORT/PIN) is also used in another test
    //(to detect K2 keypress in quadrature decoder test) in this h/w setup,
    //we need to ensure that it is configured correctly for this test
    GPIO_ConfigurePin(PWM2_PORT, PWM2_PIN, OUTPUT, PID_PWM2, true);

    //Enables TIMER0,TIMER2 clock
    set_tmr_enable(CLK_PER_REG_TMR_ENABLED);

    //Sets TIMER0,TIMER2 clock division factor to 8
    set_tmr_div(CLK_PER_REG_TMR_DIV_8);

    // initialize PWM with the desired settings (sw paused)
    timer2_init(HW_CAN_NOT_PAUSE_PWM_2_3_4, PWM_2_3_4_SW_PAUSE_ENABLED, PWM_FREQUENCY);
    // note: timer2_set_pwm_frequency() is called from timer2_init()
    //       the function timer2_set_pwm_frequency() can be used to set the timer2 frequency individually

    printf_string("\n\rTIMER2 started!");
    for(i = 0 ; i < 0xFF ; i++ )
    {
        // set pwm2 duty cycle
        timer2_set_pwm2_duty_cycle((i * PWM2_DUTY_STEP) & BIT_MASK);
        
        // set pwm3 duty cycle
        timer2_set_pwm3_duty_cycle(PWM3_MAX_DUTY - ((i * PWM34_DUTY_STEP) & BIT_MASK) + 1);
        
        // set pwm4 duty cycle
        timer2_set_pwm4_duty_cycle((i * PWM34_DUTY_STEP) & BIT_MASK);
        
        // release sw pause to let pwm2,pwm3,pwm4 run
        timer2_set_sw_pause(PWM_2_3_4_SW_PAUSE_DISABLED);

        // delay approx 1 second to observe change of duty cycle
        simple_delay();
        timer2_set_sw_pause(PWM_2_3_4_SW_PAUSE_ENABLED);
    }

    timer2_stop();
    
    // Disable TIMER0, TIMER2 clocks
    set_tmr_enable(CLK_PER_REG_TMR_DISABLED);
    printf_string("\n\rTIMER2 stopped!\n\r");
    printf_string("\n\rEnd of test\n\r");

} 

void simple_delay(void)
{
    uint32_t i;
    
    for(i = 0xBFFFF ; i != 0 ; i--);
}
