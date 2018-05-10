#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "board.h"
#include "dut_uart.h"
#include "timer.h"

#define DUT_UART_DEBUG  0

#if DUT_UART_DEBUG > 0
#warning DUT_UART debug mode
#include "console.h"
#endif

//-----------------------------------------------------------------------------
//                  DUT UART
//-----------------------------------------------------------------------------

static inline void dut_uart_delay_bit_time_(const uint16_t nbits, int baud) {
  Timer::wait(1 + ( ((uint32_t)nbits*1000*1000) / baud) );
}

namespace DUT_UART {
  static int current_baud = 0;
  
  //recieve buffer
  static const int rx_fifo_size = 0x400;
  static struct {
    char buf[rx_fifo_size]; 
    volatile int len; 
    int w, r, enabled;
  } rx;
  
  static volatile int framing_err_cnt = 0, overflow_err_cnt = 0, drop_char_cnt = 0;
}

void DUT_UART::init(int baud)
{
  if( baud == current_baud ) //already initialized!
    return;
  
  DUT_UART::deinit();
  if( baud > 0 )
  {
    DUT_TX::alternate(GPIO_AF_USART2);
    DUT_RX::alternate(GPIO_AF_USART2);
    DUT_TX::mode(MODE_ALTERNATE);
    DUT_RX::mode(MODE_ALTERNATE);
    
    //Configure UART
    USART_InitTypeDef USART_InitStruct;
    USART_InitStruct.USART_BaudRate   = baud;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits   = USART_StopBits_1;
    USART_InitStruct.USART_Parity     = USART_Parity_No ;
    USART_InitStruct.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_OverSampling8Cmd(USART2, ENABLE);
    USART_Init(USART2, &USART_InitStruct);
    
    //set up rx ints
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); //rx int enable
    NVIC_SetPriority(USART2_IRQn, 2);
    NVIC_EnableIRQ(USART2_IRQn);
    
    //Enable UART
    USART_Cmd(USART2, ENABLE);
    
    #if DUT_UART_DEBUG
      #define USART USART2
      ConsolePrintf("DUT_UART:init(%i)\n", baud);
      
      //what the @%*# is our clock??
      RCC_ClocksTypeDef RCC_ClocksStatus;
      RCC_GetClocksFreq(&RCC_ClocksStatus);
      ConsolePrintf("  HCLK=%u SYSCLK=%u PCLK1=%u PCLK2=%u\n", RCC_ClocksStatus.HCLK_Frequency, RCC_ClocksStatus.SYSCLK_Frequency, RCC_ClocksStatus.PCLK1_Frequency, RCC_ClocksStatus.PCLK2_Frequency );
      ConsolePrintf("  USART using PCLK%u\n", (USART==USART1 || USART==USART6 ? 2 : 1) );
      
      //let's see what the usart driver calcualted for register values...
      ConsolePrintf("  CR1=0x%04x CR2=0x%04x CR3=0x%04x BRR=0x%04x\n", USART->CR1, USART->CR2, USART->CR3, USART->BRR);
      ConsolePrintf("  UE=%u TE=%u RE=%u HDSEL=%u\n", (USART->CR1 & USART_CR1_UE) > 0, (USART->CR1 & USART_CR1_TE) > 0, (USART->CR1 & USART_CR1_RE) > 0, (USART->CR3 & USART_CR3_HDSEL) > 0 );
    #endif
    
    dut_uart_delay_bit_time_(2,baud); //wait at least 1 bit-time for uart peripheral to spin up
    dut_uart_delay_bit_time_(20,baud); //2 byte times on the wire for receiver to sync line change
    current_baud = baud;
    rx.enabled = 1;
  }
}

void DUT_UART::deinit(void)
{
  NVIC_DisableIRQ(USART2_IRQn);
  
  //Enable peripheral clocks
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  
  //init pins
  DUT_TX::init(MODE_INPUT, PULL_NONE, TYPE_PUSHPULL);
  DUT_RX::init(MODE_INPUT, PULL_NONE, TYPE_PUSHPULL);
  DUT_TX::alternate(0);
  DUT_RX::alternate(0);
  
  // Reset the UARTs since they tend to clog with framing errors
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART2, ENABLE);
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART2, DISABLE);
  USART_Cmd(USART2, DISABLE);
  
  current_baud = 0;
  
  //clear rx buffer
  rx.enabled = 0;
  rx.len = 0;
  rx.r = 0;
  rx.w = 0;
  
  framing_err_cnt = 0;
  overflow_err_cnt = 0;
  drop_char_cnt = 0;
}

int DUT_UART::printf(const char *format, ... )
{
  char dest[128];
  va_list argptr;
  
  va_start(argptr, format);
  int len = vsnprintf(dest, sizeof(dest), format, argptr);
  va_end(argptr);
  
  DUT_UART::write(dest);
  return len > sizeof(dest) ? sizeof(dest) : len;
}

void DUT_UART::write(const char* s)
{
  while(*s)
    DUT_UART::putchar(*s++);
}

void DUT_UART::putchar(char c)
{
  if( current_baud > 0 ) //initialized
  {
    while( !(USART2->SR & USART_SR_TXE) ); //dat moved to shift register (ok to write a new data)
    
    //volatile int junk = USART2->SR; //TC bit is cleared by 'read from the USART_SR register followed by a write to the USART_DR register'
    USART2->DR = c;
    //while( !(USART2->SR & USART_SR_TXE) ); //dat moved to shift register (ok to write a new data)
    //while( !(USART2->SR & USART_SR_TC)  ); //wait for transmit complete
  }
}

static int dut_uart_getchar_(void)
{
  /*
  volatile int junk;
  
  if( DUT_UART::current_baud > 0 ) //initialized
  {
    if( USART2->SR & (USART_SR_ORE | USART_SR_FE) ) { //framing and/or overrun error
      junk = USART2->DR; //flush the dr & shift register
      junk = USART2->DR; //reading DR clears error flags
      //USART2->SR = 0;
    } else if (USART2->SR & USART_SR_RXNE) {
      return USART2->DR; //reading DR clears RXNE flag
    }
  }
  
  return -1;
  */
  
  NVIC_DisableIRQ(USART2_IRQn);
  __NOP();
  __NOP();
  
  int c;
  if( DUT_UART::rx.len > 0 ) {
    c = DUT_UART::rx.buf[DUT_UART::rx.r];
    if(++DUT_UART::rx.r >= DUT_UART::rx_fifo_size)
      DUT_UART::rx.r = 0;
    DUT_UART::rx.len--;
  } else
    c = -1; //no data
  
  NVIC_EnableIRQ(USART2_IRQn);
  return c;
}

int DUT_UART::getchar(int timeout_us)
{
  int c;
  uint32_t start = Timer::get();
  
  do {
    c = dut_uart_getchar_();
  } while( c < 0 && Timer::elapsedUs(start) < timeout_us );
  
  return c;
}

char* DUT_UART::getline(char* buf, int bufsize, int timeout_us, int *out_len)
{
  //assert(buf != NULL && bufsize > 0);
  int c, len=0, eol=0, maxlen=bufsize-1;
  uint32_t start = Timer::get();
  do //read 1 char (even if timeout==0)
  {
    c = dut_uart_getchar_();
    if( c > 0 && c < 0x80 ) { //ignore null and non-ascii; messes with ascii parser
      if( c == '\r' || c == '\n' )
        eol = 1;
      else if( len < maxlen )
        buf[len++] = c;
      //else, whoops! drop data we don't have room for
    }
  }
  while( !eol && Timer::elapsedUs(start) < timeout_us );
  
  buf[len] = '\0'; //always null terminate
  if( out_len )
    *out_len = len;
  
  return eol ? buf : NULL;
}

namespace DUT_UART
{
  static inline void uart_isr_handler_(char c)
  {
    if( rx.enabled ) {
      if( rx.len < rx_fifo_size ) { //drop chars if fifo full
        rx.buf[rx.w] = c;
        if(++rx.w >= rx_fifo_size)
          rx.w = 0;
        rx.len++;
      }
      else {
        drop_char_cnt++;
      }
    }
  }
  
  int getRxDroppedChars() {
    int temp = drop_char_cnt;
    drop_char_cnt = 0;
    return temp;
  }
  
  int getRxOverflowErrors() {
    int temp = overflow_err_cnt;
    overflow_err_cnt = 0;
    return temp;
  }
  
  int getRxFramingErrors() {
    int temp = framing_err_cnt;
    framing_err_cnt = 0;
    return temp;
  }
}

extern "C" void USART2_IRQHandler(void)
{
  volatile int junk;
  
  uint32_t status = USART2->SR;
  if( status & (USART_SR_ORE | USART_SR_FE) ) //framing and/or overrun error
  {
    if( status & USART_SR_ORE )
      DUT_UART::overflow_err_cnt++;
    if( status & USART_SR_FE )
      DUT_UART::framing_err_cnt++;
    
    junk = USART2->DR; //flush the dr & shift register
    junk = USART2->DR; //reading DR clears error flags
  } else if (USART2->SR & USART_SR_RXNE) {
    DUT_UART::uart_isr_handler_(USART2->DR); //reading DR clears RXNE flag
  }
}
