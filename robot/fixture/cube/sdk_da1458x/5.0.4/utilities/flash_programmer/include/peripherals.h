/**
 ****************************************************************************************
 *
 * @file peripherals.h
 *
 * @brief Peripherals initialization fucntions headers
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
#include <stdint.h>
#include "gpio.h"

#ifndef PERIPHERAL_H_INCLUDED
#define PERIPHERAL_H_INCLUDED

void update_uart_pads(GPIO_PORT port, GPIO_PIN tx_pin, GPIO_PIN rx_pin);
void set_pad_uart(void);
void set_pad_spi(void);     
void set_pad_eeprom(void);  

void periph_init(void);
 
#endif
