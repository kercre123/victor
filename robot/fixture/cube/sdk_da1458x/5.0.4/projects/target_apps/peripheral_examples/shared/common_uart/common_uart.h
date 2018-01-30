/**
 ****************************************************************************************
 *
 * @file common_uart.h
 *
 * @brief uart initialization and print functions header file.
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

#ifndef UART_H_INCLUDED
#define UART_H_INCLUDED
#include "uart.h"
#include "user_periph_setup.h" 

/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */
 

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
  
void printf_byte(char ch);
/**
 ****************************************************************************************
 * @brief Uart print string fucntion
 *
 * 
 ****************************************************************************************
 */
void printf_string(char * str);
 
  /**
 ****************************************************************************************
 * @brief prints a (16-bit) half-word in hex format using printf_byte
 * @param aHalfWord The 16-bit half-word to print
  * 
 ****************************************************************************************
 */
void print_hword(uint16_t aHalfWord);

 /**
 ****************************************************************************************
 * @brief prints a (32-bit) word in hex format using printf_byte
 * @param aWord The 32-bit word to print
  * 
 ****************************************************************************************
 */
void print_word(uint32_t aWord);


 /**
 ****************************************************************************************
 * @brief prints a byte in dec format 
 * @param a The byte to print
  * 
 ****************************************************************************************
 */
void printf_byte_dec(int a); 

#endif
