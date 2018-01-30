/**
 ****************************************************************************************
 *
 * @file DA14580_examples.h
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

#ifndef __PROGRAMMER_INCLUDED__
#define __PROGRAMMER_INCLUDED__

void print_menu(void);
void print_input(void);
void endtest_bridge(short int *idx); 
void exit_test(void);
uint32_t crc32(uint32_t crc, const void *buf, size_t size);
#endif
