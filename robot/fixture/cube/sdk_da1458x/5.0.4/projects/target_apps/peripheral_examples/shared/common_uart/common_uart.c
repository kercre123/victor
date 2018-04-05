/**
 ****************************************************************************************
 *
 * @file common_uart.c
 *
 * @brief (Do not use for your designs) - (legacy) uart initialization & print functions
 *        Please, refer to the Peripheral Drivers documentation for the current uart.c driver
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
#include "gpio.h"
#include <core_cm0.h>
#include "common_uart.h"



/**
 ****************************************************************************************
 * @brief print a Byte in hex format
 * @param ch  character to print
 * 
 ****************************************************************************************
 */
void printf_byte(char ch)
{
    // print a Byte in hex format
    char b;
    b = ((0xF0 & ch) >> 4);
    b += (b < 10) ? 48 : 55;

    uart2_write((uint8_t *)&b,1, NULL);
    uart2_finish_transfers();
    b = (0xF & ch);
    b += (b < 10) ? 48 : 55;
    uart2_write((uint8_t *)&b,1, NULL);
    uart2_finish_transfers();
}

/**
 ****************************************************************************************
 * @brief Uart print string fucntion
 *
 * 
 ****************************************************************************************
 */
void printf_string(char * str)
{
    uart2_write((uint8_t *)str, strlen(str), NULL); // send next string character
    uart2_finish_transfers();
}


 /**
 ****************************************************************************************
 * @brief prints a (16-bit) half-word in hex format using printf_byte
 * @param aHalfWord The 16-bit half-word to print
  * 
 ****************************************************************************************
 */

void print_hword(uint16_t aHalfWord)
{
    printf_byte((aHalfWord >> 8) & 0xFF);
    printf_byte((aHalfWord) & 0xFF);
}


 /**
 ****************************************************************************************
 * @brief prints a (32-bit) word in hex format using printf_byte
 * @param aHalfWord The 32-bit word to print
  * 
 ****************************************************************************************
 */
void print_word(uint32_t aWord)
{
    printf_byte((aWord >> 24) & 0xFF);
    printf_byte((aWord >> 16) & 0xFF);
    printf_byte((aWord >> 8) & 0xFF);
    printf_byte((aWord) & 0xFF);
}

 /* reverse:  reverse string s in place */
 void reverse(char s[])
 {
    int i, j;
    char c;
 
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
 }

/* itoa:  convert n to characters in s */
 void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}
 
void printf_byte_dec(int a){		  // print a Byte in decimal format
#ifndef UART_ENABLED
	return ;
#else
    char temp_buf[4];

	if ( a>255 )
    return;

    itoa( a, 	temp_buf );	
	printf_string(temp_buf);

#endif
}
