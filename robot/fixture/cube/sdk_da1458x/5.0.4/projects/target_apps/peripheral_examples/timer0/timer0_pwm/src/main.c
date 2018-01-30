/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Timer0 PWM example for DA14580/581 SDK
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

#include "global_io.h"
#include "common_uart.h"
#include "user_periph_setup.h"
#include "pwm.h"

// PWM settings
#define TIMER_ON        1000
#define PWM_HIGH        500
#define PWM_LOW         200
#define ALL_NOTES       26

void system_init(void);
void timer0_pwm_test(void);

#define TIMER0_PWM_TEST_EXPIRATION_COUNTER_RELOAD_VALUE 50

// table with values used for setting different PWM duty cycle to change the "melody" played by buzzer
const uint16_t notes[ALL_NOTES] = {1046,987,767,932,328,880,830,609,783,991,739,989,698,456,659,255,622,254,587,554,365,523,251,493,466,440};
uint16_t timer0_pwm_test_expiration_counter;

/**
 ****************************************************************************************
 * @brief  Main routine of the timer0 pwm example
 * 
 ****************************************************************************************
 */
int main (void)
{
    system_init();
    periph_init();
    timer0_pwm_test();
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

/**
 ****************************************************************************************
 * @brief  User callback function called by SWTIM_IRQn interupt. Function is changing
 *         "note" played by buzzer by changing PWM duty cycle.
 * 
 ****************************************************************************************
 */ 
void pwm0_user_callback_function(void)
{ 
    static uint8_t n = 0;
    static uint8_t i = 0;

    if (n == 10) //change "note" every 10 * 32,768ms
    {
        n = 0;
        // Change note and set ON-counter to Ton = 1/2Mhz * 65536 = 32,768ms
        timer0_set_pwm_on_counter(0xFFFF);
        timer0_set_pwm_high_counter(notes[i]/3 * 2);
        timer0_set_pwm_low_counter(notes[i]/3);

        printf_string("*");
        
        // limit i iterator to max index of table of notes
        i = ++i % (ALL_NOTES - 1);
        
        if (timer0_pwm_test_expiration_counter) 
        {
            timer0_pwm_test_expiration_counter--;
        }
    }
    n++;
}


/**
 ****************************************************************************************
 * @brief  PWM test function
 * 
 ****************************************************************************************
 */
void timer0_pwm_test(void)
{
    printf_string("\n\r\n\r");
    printf_string("*******************\n\r");
    printf_string("* TIMER0 PWM TEST *\n\r");
    printf_string("*******************\n\r");
     
    //Enables TIMER0,TIMER2 clock
    set_tmr_enable(CLK_PER_REG_TMR_ENABLED);

    //Sets TIMER0,TIMER2 clock division factor to 8, so TIM0 Fclk is F = 16MHz/8 = 2Mhz
    set_tmr_div(CLK_PER_REG_TMR_DIV_8);

    // initialize PWM with the desired settings
    timer0_init(TIM0_CLK_FAST, PWM_MODE_ONE, TIM0_CLK_NO_DIV);

    // set pwm Timer0 'On', Timer0 'high' and Timer0 'low' reload values
    timer0_set(TIMER_ON, PWM_HIGH, PWM_LOW);

    // register callback function for SWTIM_IRQn irq
    timer0_register_callback(pwm0_user_callback_function);

    // enable SWTIM_IRQn irq
    timer0_enable_irq();

    timer0_pwm_test_expiration_counter = TIMER0_PWM_TEST_EXPIRATION_COUNTER_RELOAD_VALUE;

    printf_string("\n\rTIMER0 starts!");
    printf_string("\n\rYou can hear the sound produced by PWM0 or PWM1");
    printf_string("\n\rif you attach a buzzer on pins P0_2 or P0_3 respectively.");
    printf_string("\n\rPlaying melody. Please wait...\n\r");

    // start pwm0
    timer0_start();

    while (timer0_pwm_test_expiration_counter);

    timer0_stop();

    // Disable TIMER0, TIMER2 clocks
    set_tmr_enable(CLK_PER_REG_TMR_DISABLED);

    printf_string("\n\rTIMER0 stopped!\n\r");
    printf_string("\n\rEnd of test\n\r");
}
