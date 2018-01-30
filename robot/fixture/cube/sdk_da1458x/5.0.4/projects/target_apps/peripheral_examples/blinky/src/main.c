/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Blinky example for DA14580/581 SDK
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
#include "gpio.h"

#define LED_OFF_THRESHOLD 10000
#define LED_ON_THRESHOLD  400000

void system_init(void);
void blinky_test(void);

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
    blinky_test();
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
 * @brief Blinky test fucntion
 *
 * 
 ****************************************************************************************
 */

void blinky_test(void)
{
    int i=0;

    // Select function of the port P1.0 to pilot the LED 
    printf_string("\n\r\n\r");
    printf_string("***************\n\r");
    printf_string("* BLINKY DEMO *\n\r");
    printf_string("***************\n\r");

    while(1) {
        i++;
        if (LED_OFF_THRESHOLD   == i) {
            GPIO_SetActive( LED_PORT, LED_PIN);
            printf_string("\n\r *LED ON* ");
        }
        if (LED_ON_THRESHOLD == i) {
            GPIO_SetInactive(LED_PORT, LED_PIN);
            printf_string("\n\r *LED OFF* ");
        }
				if (i== 2*LED_ON_THRESHOLD){
					i=0;
				}
    }
}
