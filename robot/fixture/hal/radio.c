#include <string.h>
#include "hal/board.h"
#include "hal/portable.h"
#include "hal/uart.h"
#include "hal/testport.h"
#include "hal/display.h"
#include "hal/timers.h"
#include "../app/fixture.h"
#include "hal/cube.h"
#include "hal/flash.h"
#include "app/binaries.h"

bool UpdateNRF(bool forceupdate);

const int BAUDRATE = 115200;
#define NRF_UART USART3

// Initialize the UART link to the radio
static void InitRadio()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin =  GPIOC_NRF_TX;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, PINC_NRF_TX, GPIO_AF_USART3);
  
  GPIO_InitStructure.GPIO_Pin =  GPIOC_NRF_RX;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, PINC_NRF_RX, GPIO_AF_USART3);
  
  // TX/RX config
  USART_Cmd(NRF_UART, DISABLE);
  USART_InitStructure.USART_BaudRate = BAUDRATE;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(NRF_UART, &USART_InitStructure);  
  USART_Cmd(NRF_UART, ENABLE);
}

// Receive a byte from the radio if there is one, or return -1 if not
int GetChar()
{
  // Check for overrun
  if (NRF_UART->SR & USART_SR_ORE)
  {
    volatile int v;
    v = NRF_UART->SR;
    v = NRF_UART->DR;
  }
  
  if (NRF_UART->SR & USART_SR_RXNE)
    return NRF_UART->DR & 0xFF;
  return -1;
}

// Receive a byte from the radio, blocking until it arrives, or return -1 after timeout microseconds
int GetCharWait(int timeout)
{
  volatile u32 v;
  u32 status;
  u32 startTime;
  int value;
  
  // Check for overrun
  status = NRF_UART->SR;
  if (status & USART_SR_ORE)
  {
    v = NRF_UART->SR;
    v = NRF_UART->DR;
  }
  
  status = 0;
  value = -1;
  startTime = getMicroCounter();
  while (getMicroCounter() - startTime < timeout)
  {
    if (NRF_UART->SR & USART_SR_RXNE)
    {
      value = NRF_UART->DR & 0xFF;
      break;
    }
  }
  return value;
}

// Send a character to the radio
static void PutChar(u8 c)
{    
  NRF_UART->DR = c;
  while (!(NRF_UART->SR & USART_FLAG_TXE))
    ;
}

char g_mode = 'X';

// Process incoming bytes from the radio - must call at least 12,000 times/second
void RadioProcess()
{
  // Just continue if no character
  int c = GetChar();
  if (-1 == c)
    return;
  
  // Otherwise, process messages or grab arguments
  static int argbytes = 0, arg = 0, msg = 0;
  if (!argbytes)
  {
    // Process messages
    msg = c;
    if (msg == 'C')   // Print cube ID
    {
      // Cube message has 4 bytes
      argbytes = 4;
      arg = 0;
    }
    if (msg == '1')   // Watchdogged - restore mode
      PutChar(g_mode);
  } else {
    // Grab arguments
    argbytes--;
    arg |= (c << (8*argbytes)); // XXX: This byteswaps because I'm used to IDs being byteswapped
    
    // If we have a whole argument, print it out
    if (!argbytes)
      ConsolePrintf("cube,%c,%08x\r\n", msg, arg);
  }
}

// Put the radio into a specific test mode
void SetRadioMode(char mode)
{
  static bool isRadioOK = false;
  bool forceupdate = (mode == 'U');
  
  InitRadio();
  // Try 5 times, since a buggy ISR in the NRF clobbers the update attempt
  for (int i = 5; i >= 0; i--)
    try {
      GetChar();
      isRadioOK = UpdateNRF(forceupdate || i == 0);   // Always force-update on last attempt
      // Wait for sign-on message
      int c;
      do {
        c = GetCharWait(1000000);
        if (-1 == c)
          throw ERROR_RADIO_TIMEOUT;
    } while ('!' != c);
      break;
    } catch (int e) {
      if (i == 0)
        throw e;
    }

  g_mode = mode;
  PutChar(g_mode);
}
