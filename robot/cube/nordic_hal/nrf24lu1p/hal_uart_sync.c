/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic 
 * Semiconductor ASA.Terms and conditions of usage are described in detail 
 * in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT. 
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRENTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *              
 * $LastChangedRevision: 133 $
 */
 
/** @file
* @brief Implementation of the UART HAL module for nRF24LU1+ without data buffering
*/
 
#include "nrf24lu1p.h"
#include <stdint.h>

#include "hal_uart.h"


#define BAUD_57K6   1015
#define BAUD_38K4   1011
#define BAUD_19K2    998
#define BAUD_9K6     972

void hal_uart_init(hal_uart_baudrate_t baud)
{
  uint16_t temp;

  ES0 = 0;                      // Disable UART0 interrupt while initializing
  REN0 = 1;                     // Enable receiver
  SM0 = 0;                      // Mode 1..
  SM1 = 1;                      // ..8 bit variable baud rate
  PCON |= 0x80;                 // SMOD = 1
  WDCON |= 0x80;                // Select internal baud rate generator

  switch(baud)
  {
    case UART_BAUD_57K6:
      temp = BAUD_57K6;
      break;
    case UART_BAUD_38K4:
      temp = BAUD_38K4;
      break;
    case UART_BAUD_9K6:
      temp = BAUD_9K6;
      break;
    case UART_BAUD_19K2:
    default:
      temp = BAUD_19K2;
      break;
  }

  S0RELL = (uint8_t)temp;
  S0RELH = (uint8_t)(temp >> 8);
  P0ALT |= 0x06;                // Select alternate functions on P01 and P02
  P0EXP &= 0xf0;                // Select RxD on P01 and TxD on P02
  P0DIR |= 0x02;                // P01 (RxD) is input
  TI0 = 1;
}

void hal_uart_putchar(uint8_t ch)
{
  while(TI0 == 0)
    ;
  TI0 = 0;
  S0BUF = ch;
}

uint8_t hal_uart_getchar(void)
{
  while(RI0 == 0)
    ;
  RI0 = 0;
  return S0BUF;
}

static void hal_uart_putstring(char *s)
{
  while(*s != 0)
  hal_uart_putchar(*s++);
}
