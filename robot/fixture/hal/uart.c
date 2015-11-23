// Based on Drive Testfix, updated for Cozmo EP1 Testfix
#include "lib/stm32f2xx.h"
#include "hal/uart.h"
#include "hal/timers.h"
#include <stdarg.h>
#include <stdio.h>

char* const g_hex = "0123456789abcdef";

#define BAUD_RATE    3000000

#ifdef DEBUG_UART_ENABLED

void SlowPrintf(const char* format, ...)
{
  char dest[64];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(dest, 64, format, argptr);
  va_end(argptr);  
  SlowPutString(dest);
}

void InitUART(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  // Configure USART1_TX for Push/Pull output (until gpio.c turns it into an LED)
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   // You laugh, but there's a ton of capacitance on those LEDs!
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);
    
  // USART1 config
  USART_Cmd(USART1, DISABLE);
  USART_InitStructure.USART_BaudRate = BAUD_RATE;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx;
  USART_OverSampling8Cmd(USART1, ENABLE);
  USART_Init(USART1, &USART_InitStructure);
  USART_Cmd(USART1, ENABLE);
}

void SlowPutChar(char c)
{
  USART_SendData(USART1, c);
  while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void SlowPutString(const char *s)
{
  while (*s) {
    if (*s == '\n')
      SlowPutChar('\r');
    SlowPutChar(*s++);
  }
}

void SlowPutInt(int i)
{
  SlowPutChar(' ');
  SlowPutChar('0' + i / 100);
  SlowPutChar('0' + (i / 10) % 10);
  SlowPutChar('0' + i % 10);
}

void SlowPutLong(int i)
{
  SlowPutChar((i < 0) ? '-' : ' ');
  if (i < 0)
    i = -i;
  SlowPutChar('0' + i / 10000);
  SlowPutChar('0' + (i / 1000) % 10);
  SlowPutChar('0' + (i / 100) % 10);
  SlowPutChar('0' + (i / 10) % 10);
  SlowPutChar('0' + i % 10);
}

void SlowPutHex(int i)
{
  SlowPutChar(g_hex[(i >> 4) & 15]);
  SlowPutChar(g_hex[(i) & 15]);
}

void SlowPutLongHex(int i)
{
  int nibble;
  for (nibble = 7; nibble >= 0; nibble--)
  {
    SlowPutChar(g_hex[(i >> (4 * nibble)) & 0x0F]);
  }
}
#endif
