/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Timer0 general example for DA14580/581 SDK
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

// Timer0 settings
#define NO_PWM            0x0                       // PWM not used
#define RELOAD_100MS      20000                     // reload value fo 100ms
#define TEST_LENGTH_SEC   10                        // length of the test in seconds

void system_init(void);
void timer0_general_test(uint8_t times_seconds);
  
uint8_t timeout_expiration;
 
/**
 ****************************************************************************************
 * @brief  Main routine of the timer0 general example
 * 
 ****************************************************************************************
 */
int main (void)
{
    system_init();
    periph_init();
    GPIO_SetActive(LED_PORT, LED_PIN);
    timer0_general_test(TEST_LENGTH_SEC);
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
 * @brief  Timer0 general user callback function
 * 
 ****************************************************************************************
 */ 
void timer0_general_user_callback_function(void) 
{
    static uint8_t n = 0;

    // when pass  10 * 100ms
    if ( 10 == n ) 
    {
        n = 0;
        timeout_expiration--;
        if (GPIO_GetPinStatus(LED_PORT, LED_PIN))
        {
            GPIO_SetInactive(LED_PORT, LED_PIN);
        }
        else
        {
            GPIO_SetActive(LED_PORT, LED_PIN);
        }
     }
     n++;
}

/**
 ****************************************************************************************
 * @brief  Timer0 general test function
 * @param[in] times_seconds:  test length in seconds 
 ****************************************************************************************
 */
void timer0_general_test(uint8_t times_seconds)
{
    printf_string("\n\r\n\r");
    printf_string("***********************\n\r");
    printf_string("* TIMER0 GENERAL TEST *\n\r");
    printf_string("***********************\n\r");  

    // Stop timer for enter settings
    timer0_stop();

    timeout_expiration = times_seconds;

    // register callback function for SWTIM_IRQn irq
    timer0_register_callback(timer0_general_user_callback_function);

    // Enable TIMER0 clock
    set_tmr_enable(CLK_PER_REG_TMR_ENABLED);

    // Sets TIMER0,TIMER2 clock division factor to 8, so TIM0 Fclk is F = 16MHz/8 = 2Mhz
    set_tmr_div(CLK_PER_REG_TMR_DIV_8);

    // clear PWM settings register to not generate PWM
    timer0_set_pwm_high_counter(NO_PWM);
    timer0_set_pwm_low_counter(NO_PWM);
    
    // Set timer with 2MHz source clock divided by 10 so Fclk = 2MHz/10 = 200kHz
    timer0_init(TIM0_CLK_FAST, PWM_MODE_ONE, TIM0_CLK_DIV_BY_10);

    // reload value for 100ms (T = 1/200kHz * RELOAD_100MS = 0,000005 * 20000 = 100ms)
    timer0_set_pwm_on_counter(RELOAD_100MS);

    // Enable SWTIM_IRQn irq
    timer0_enable_irq();

    printf_string("\n\rLED will change state every second.\n\r");
    printf_string("\n\rTest will run for: ");
    printf_byte(timeout_expiration);
    printf_string(" seconds.\n\r");
    
    // Start Timer0
    timer0_start();
    printf_string("\n\rTIMER0 started!");

    // Wait for desired number of seconds
    while (timeout_expiration);

    // Disable TIMER0, TIMER2 clocks
    set_tmr_enable(CLK_PER_REG_TMR_DISABLED);
    printf_string("\n\rTIMER0 stopped!\n\r");
    printf_string("\n\rEnd of test\n\r");
}
