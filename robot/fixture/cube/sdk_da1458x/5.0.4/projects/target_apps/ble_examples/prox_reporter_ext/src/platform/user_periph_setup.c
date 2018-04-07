/**
 ****************************************************************************************
 *
 * @file periph_setup.c
 *
 * @brief Peripherals setup and initialization. 
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

#include "rwip_config.h"        // SW configuration
#include "user_periph_setup.h"  // periphera configuration
#include "global_io.h"
#include "gpio.h"
#include "uart.h"               // UART initialization


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if DEVELOPMENT_DEBUG
/**
 ****************************************************************************************
 * @brief GPIO_reservations. Globally reserved GPIOs
 *
 * @return void 
 ****************************************************************************************
*/
void GPIO_reservations(void)
{
    RESERVE_GPIO(UART1_TX,        UART1_TX_GPIO_PORT,   UART1_TX_GPIO_PIN,   PID_UART1_TX);
    RESERVE_GPIO(UART1_RX,        UART1_RX_GPIO_PORT,   UART1_RX_GPIO_PIN,   PID_UART1_RX);
    #if !HW_CONFIG_USB_DONGLE
    RESERVE_GPIO(UART1_RTS,       UART1_RTSN_GPIO_PORT, UART1_RTSN_GPIO_PIN, PID_UART1_RTSN);
    RESERVE_GPIO(UART1_CTS,       UART1_CTSN_GPIO_PORT, UART1_CTSN_GPIO_PIN, PID_UART1_CTSN);
    #endif
    #ifdef CFG_WAKEUP_EXT_PROCESSOR	
    // external MCU wakeup GPIO
    RESERVE_GPIO(EXT_WAKEUP_GPIO, EXT_WAKEUP_PORT,      EXT_WAKEUP_PIN,      PID_GPIO );    
    #endif
}
#endif

/**
 ****************************************************************************************
 * @brief Map port pins
 *
 * The Uart and SPI port pins and GPIO ports(for debugging) are mapped
 ****************************************************************************************
 */
void set_pad_functions(void)        // set gpio port function mode
{
    GPIO_ConfigurePin(UART1_TX_GPIO_PORT,   UART1_TX_GPIO_PIN,   OUTPUT, PID_UART1_TX,   false);
    GPIO_ConfigurePin(UART1_RX_GPIO_PORT,   UART1_RX_GPIO_PIN,   INPUT,  PID_UART1_RX,   false);
#if !HW_CONFIG_USB_DONGLE
    GPIO_ConfigurePin(UART1_RTSN_GPIO_PORT, UART1_RTSN_GPIO_PIN, OUTPUT, PID_UART1_RTSN, false);
    GPIO_ConfigurePin(UART1_CTSN_GPIO_PORT, UART1_CTSN_GPIO_PIN, INPUT,  PID_UART1_CTSN, false);
#endif
#ifdef CFG_WAKEUP_EXT_PROCESSOR		
    // external MCU wakeup GPIO
    GPIO_ConfigurePin(EXT_WAKEUP_PORT,      EXT_WAKEUP_PIN,      OUTPUT, PID_GPIO,       false); // initialized to low
#endif	
}

/**
 ****************************************************************************************
 * @brief Enable pad's and peripheral clocks assuming that peripherals' power domain is down.
 *
 * The Uart and SPi clocks are set. 
 ****************************************************************************************
 */
void periph_init(void)  // set i2c, spi, uart, uart2 serial clks
{
	// Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP)); 
    
    SetBits16(CLK_16M_REG,XTAL16_BIAS_SH_ENABLE, 1);
	
	// TODO: Application specific - Modify accordingly!
	// Example: Activate UART and SPI.
	
    // Initialize UART component
    SetBits16(CLK_PER_REG, UART1_ENABLE, 1);    // enable clock - always @16MHz
	
    // NOTE:
    // If a lower than 4800bps baudrate is required then uart_init() must be replaced
    // by a call to arch_uart_init_slow() (defined in arch_system.h).
    // mode=3-> no parity, 1 stop bit 8 data length
    uart_init(UART_BAUDRATE_115K2, 3);
	
	//rom patch
	patch_func();
	
	//Init pads
	set_pad_functions();

    // Enable the pads
	SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);
}
