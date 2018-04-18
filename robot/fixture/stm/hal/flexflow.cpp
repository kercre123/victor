#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "board.h"
#include "flexflow.h"
#include "timer.h"

#define FLEXFLOW_DEBUG  0

#if FLEXFLOW_DEBUG > 0
#warning FLEXFLOW debug mode
#include "console.h"
#endif

#define FTX_PIN   TX /*ICSP TX @ PB.6*/

//-----------------------------------------------------------------------------
//                  FLEXFLOW
//-----------------------------------------------------------------------------

static inline void delay_bit_time_(const uint16_t nbits, int baud) {
  Timer::wait(1 + ( ((uint32_t)nbits*1000*1000) / baud) );
}

namespace FLEXFLOW {
  static int current_baud = 0;
}

void FLEXFLOW::init(int baud)
{
  if( baud == current_baud ) //already initialized!
    return;
  
  FLEXFLOW::deinit();
  if( baud > 0 )
  {
    FTX_PIN::alternate(GPIO_AF_USART1);
    FTX_PIN::mode(MODE_ALTERNATE);
    
    //Configure UART
    USART_InitTypeDef USART_InitStruct;
    USART_InitStruct.USART_BaudRate   = baud;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits   = USART_StopBits_1;
    USART_InitStruct.USART_Parity     = USART_Parity_No ;
    USART_InitStruct.USART_Mode       = USART_Mode_Tx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_OverSampling8Cmd(USART1, ENABLE);
    USART_Init(USART1, &USART_InitStruct);
    
    //Enable UART
    USART_Cmd(USART1, ENABLE);
    
    delay_bit_time_(2,baud); //wait at least 1 bit-time for uart peripheral to spin up
    delay_bit_time_(20,baud); //2 byte times on the wire for receiver to sync line change
    current_baud = baud;
  }
}

void FLEXFLOW::deinit(void)
{
  if( current_baud > 0 ) {
    while( !(USART1->SR & USART_SR_TXE) ); //dat moved to shift register (ok to write a new data)
    while( !(USART1->SR & USART_SR_TC)  ); //wait for transmit complete
  }
  
  //Enable peripheral clocks
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  
  //init pins
  FTX_PIN::init(MODE_INPUT, PULL_NONE, TYPE_PUSHPULL);
  FTX_PIN::alternate(0);
  
  // Reset the UARTs since they tend to clog with framing errors
  RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART1, ENABLE);
  RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART1, DISABLE);
  USART_Cmd(USART1, DISABLE);
  
  current_baud = 0;
}

int FLEXFLOW::printf(const char *format, ... )
{
  char dest[128];
  va_list argptr;
  
  va_start(argptr, format);
  int len = vsnprintf(dest, sizeof(dest), format, argptr);
  va_end(argptr);
  
  FLEXFLOW::write(dest);
  return len > sizeof(dest) ? sizeof(dest) : len;
}

void FLEXFLOW::write(const char* s)
{
  while(*s)
    FLEXFLOW::putchar(*s++);
}

void FLEXFLOW::putchar(char c)
{
  if( current_baud > 0 ) //initialized
  {
    while( !(USART1->SR & USART_SR_TXE) ); //dat moved to shift register (ok to write a new data)
    
    volatile int junk = USART1->SR; //TC bit is cleared by 'read from the USART_SR register followed by a write to the USART_DR register'
    USART1->DR = c;
    //while( !(USART1->SR & USART_SR_TXE) ); //dat moved to shift register (ok to write a new data)
    //while( !(USART1->SR & USART_SR_TC)  ); //wait for transmit complete
  }
}

