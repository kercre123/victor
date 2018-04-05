/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief UART2 example main file
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

#include "datasheet.h"
#include "core_cm0.h"
#include "global_io.h"
#include "user_periph_setup.h"
#include "uart2_print_string.h"

void system_init(void);
void uart2_test(void);

/**
 ****************************************************************************************
 * @brief  Main routine of the UART example
 * 
 ****************************************************************************************
 */
int main(void)
{
    system_init();
    periph_init();
    uart2_test();
    while(1);
}

/**
 ****************************************************************************************
 * @brief System Initiialization 
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
 * @brief UART2 test entry point
 *
 ****************************************************************************************
 */
void uart2_test(void)
{
    void uart2_write_test(void);
    void uart2_read_test(void);
    void uart2_loopback_test(void);
    
    uart2_print_string("\r\n");
    uart2_print_string("*********************\r\n");
    uart2_print_string("UART2 driver example.\r\n");
    uart2_print_string("*********************\r\n");
    uart2_print_string("\r\n");
    
    //
    // UART2 asynchronous write example
    //
    uart2_print_string("uart2_write_test: \r\n");
    
    uart2_write_test();

    uart2_print_string("\r\n");

    //
    // UART2 asynchronous read example
    //
    uart2_print_string("uart2_read_test: \r\n");
    
    uart2_read_test();
    
    uart2_print_string("\r\n");
    
    //
    // UART2 loopback example
    // 
    uart2_print_string("uart2_loopback_test: \r\n");
    
    uart2_loopback_test();
    
    uart2_print_string("\r\n");
}
