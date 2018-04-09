/**
 ****************************************************************************************
 *
 * @file periph_setup.c
 *
 * @brief Peripherals initialization functions
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
#include "global_io.h"
#include "uart.h"
#include "gpio.h"
#include <core_cm0.h>
#include "user_periph_setup.h"

 /**
 ****************************************************************************************
 * @brief Enable pad's and peripheral clocks assuming that peripherals' power domain is down.
 *        The Uart and SPi clocks are set. 
 * 
 ****************************************************************************************
 */
void periph_init(void)
{
    // Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, PERIPH_SLEEP_ENABLE);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));

    // Init pads
    GPIO_ConfigurePin(UART2_GPIO_PORT, UART2_TX_PIN, OUTPUT, PID_UART2_TX, false);
    GPIO_ConfigurePin(UART2_GPIO_PORT, UART2_RX_PIN, INPUT, PID_UART2_RX, false);
    SetBits16(CLK_PER_REG, UART2_ENABLE, 1);

    // Initialize UART component
    uart2_init(UART2_BAUDRATE, UART2_DATALENGTH);
}
