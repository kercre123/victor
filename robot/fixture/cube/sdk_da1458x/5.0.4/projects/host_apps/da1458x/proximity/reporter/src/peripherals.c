/**
 ****************************************************************************************
 *
 * @file peripherals.c
 *
 * @brief Peripherals initialization fucntions
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
#include "uart.h"
#include "gpio.h"
#include <core_cm0.h>
#include "peripherals.h"
#include "user_periph_setup.h"


  
 /**
 ****************************************************************************************
 * @brief Enable pad's and peripheral clocks assuming that peripherals' power domain is down.
 *        The Uart and SPi clocks are set. 
 * 
 ****************************************************************************************
 */
void periph_init(void)  // set spi, uart serial clks
{
    // system init
    SetWord16(CLK_AMBA_REG, 0x00);                // set clocks (hclk and pclk ) 16MHz
    SetBits16(CLK_PER_REG, SPI_DIV, 2);           // set divider for SPI clock to 4 (16/4=4MHz)
    SetWord16(SET_FREEZE_REG,FRZ_WDOG);           // stop watch dog
    SetBits16(SYS_CTRL_REG,PAD_LATCH_EN,1);       // open pads
    SetBits16(SYS_CTRL_REG,DEBUGGER_ENABLE,1);    // open debugger
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP,0);      // exit peripheral power down
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));  
    // Initialize UART component
#ifdef UART_ENABLED
    uart_init(9,3);
#endif
    //Init pads
    set_pad_functions();
} 
 
/**
 ****************************************************************************************
 * @brief set gpio port function mode
 * 
 ****************************************************************************************
 */
void set_pad_functions(void)       
{
#ifdef UART_ENABLED
    GPIO_ConfigurePin( UART_GPIO_PORT, UART_TX_PIN, OUTPUT, PID_UART1_TX, false );
    GPIO_ConfigurePin( UART_GPIO_PORT, UART_RX_PIN, INPUT, PID_UART1_RX, false );
#endif

    // configure 5-wire SPI pins
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_CS_PIN, OUTPUT, PID_SPI_EN, true );
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_CLK_PIN, OUTPUT, PID_SPI_CLK, false );
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DO_PIN, OUTPUT, PID_SPI_DO, false );	
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DI_PIN, INPUT, PID_SPI_DI, false );
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DREADY_PIN, INPUT_PULLDOWN, PID_GPIO, false);

    // configure button
    GPIO_ConfigurePin( GPIO_PORT_1, GPIO_PIN_1, INPUT_PULLUP, PID_GPIO, true);
}
