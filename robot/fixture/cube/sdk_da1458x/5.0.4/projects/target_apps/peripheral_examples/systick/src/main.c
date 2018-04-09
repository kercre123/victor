/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief UART example for DA14580/581 SDK
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
#include "systick.h"
#include "gpio.h"

#define SYSTICK_PERIOD_US   1000000     // period for systick timer in us, so 1000000ticks = 1second
#define SYSTICK_EXCEPTION   1           // generate systick exceptions
int i = 0;

void system_init(void);
void systick_isr(void); 
void systick_test(void);

/**
 ****************************************************************************************
 * @brief  Main routine of the UART example
 * 
 ****************************************************************************************
 */
int main (void)
{
    system_init();
    periph_init();
    systick_test();
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
 * @brief Systick ISR routine 
 *
 * 
 ****************************************************************************************
 */
void systick_isr(void)
{
    if (i == 0)
    {
        GPIO_SetActive(LED_PORT, LED_PIN);
        i = 1;
    }
    else
    {
        GPIO_SetInactive(LED_PORT, LED_PIN);
        i = 0;
    }
}

/**
 ****************************************************************************************
 * @brief Systick Test routine 
 *
 * 
 ****************************************************************************************
 */
void systick_test(void)
{
    systick_register_callback(systick_isr);
    // Systick will be initialized to use a reference clock frequency of 1 MHz
    systick_start(SYSTICK_PERIOD_US, SYSTICK_EXCEPTION);
}
