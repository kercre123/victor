#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "common.h"
#include "contacts.h"
#include "hardware.h"
#include "ucount.h"

//target selections
#if defined(SYSCON) && defined(STM32F030xC)
  #define TARGET_SYSCON     1
  #define TARGET_FIXTURE    0
  #define USART             USART2
  #define USART_IRQn        USART2_IRQn
  #define USART_IRQHandler  USART2_IRQHandler
  #define USART_RCC_EN(x)   RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ((x)?ENABLE:DISABLE) )
  #define GPIO_RCC_EN(x)    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ((x)?ENABLE:DISABLE) )
#elif defined(FIXTURE) && defined(STM32F2XX)
  #define TARGET_SYSCON     0
  #define TARGET_FIXTURE    1
  #define USART             USART6
  #define USART_IRQn        USART6_IRQn
  #define USART_IRQHandler  USART6_IRQHandler
  #define USART_RCC_EN(x)   RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ((x)?ENABLE:DISABLE) );
  #define GPIO_RCC_EN(x)    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ((x)?ENABLE:DISABLE) )
#else
  #error No compatible target for Contacts interface
#endif

namespace Contacts
{
  enum CONTACT_MODES {
    MODE_UNINITIALIZED = 0,
    MODE_INITIALIZED,
  };
  
  static const uint32_t CONTACT_BAUD = 115200;
  static int mode = MODE_UNINITIALIZED;
  
  //recieve buffer
  static const int rx_fifo_size = 0x40;
  static struct {
    char buf[rx_fifo_size]; 
    volatile int len; 
    int w, r;
  } rx;
  
  static inline void delay_bit_time_(const uint16_t n) {
    uCount::waitUs(1 + ( ((uint32_t)n*1000*1000) / CONTACT_BAUD) );
  }
  
  static inline void rx_enable_(void)
  {
    #if TARGET_SYSCON
      USART->CR1 |= USART_CR1_RE; //enable RX
      __NOP();__NOP();__NOP();__NOP(); //XXX: optimize this
    #endif
  }
  
  static inline void rx_disable_(void)
  {
    #if TARGET_SYSCON
      USART->CR1 &= ~USART_CR1_RE; //disable RX
      __NOP();__NOP();__NOP();__NOP(); //XXX: optimize this
    #endif
  }
  
  static inline void putchar_(char c)
  {
    #if TARGET_SYSCON
      USART->TDR = c;
      while(!(USART->ISR & USART_ISR_TXE)); //dat moved to shift register (ok to write a new data)
      while(!(USART->ISR & USART_ISR_TC )); //wait for transmit complete
    #elif TARGET_FIXTURE
      volatile int junk = USART->SR; //TC bit is cleared by 'read from the USART_SR register followed by a write to the USART_DR register'
      USART->DR = c;
      while( !(USART->SR & USART_SR_TXE) ); //dat moved to shift register (ok to write a new data)
      while( !(USART->SR & USART_SR_TC)  ); //wait for transmit complete
    #endif
  }
}

void Contacts::init(void)
{
  if( mode != MODE_UNINITIALIZED )
    return;
  
  memset(&rx, 0, sizeof(rx));
  
  //Enable peripheral clocks
  GPIO_RCC_EN(1);
  USART_RCC_EN(1);
  
  //Config uart pins
  #if TARGET_SYSCON
    VEXT_SENSE::init(MODE_ALTERNATE, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);
    //VEXT_SENSE::init(MODE_ALTERNATE, PULL_UP, TYPE_PUSHPULL, SPEED_HIGH);
    VEXT_SENSE::alternate(GPIO_AF_1); //USART2_TX
  #elif TARGET_FIXTURE
    CHGTX::init(MODE_ALTERNATE, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);
    CHGTX::alternate(GPIO_AF_USART6);
    CHGRX::init(MODE_ALTERNATE, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);  
    CHGRX::alternate(GPIO_AF_USART6);
  #endif
  
  //Configure UART
  USART->CR1 = 0; //disable usart while we config
  /*
  USART->BRR = SYSTEM_CLOCK / CONTACT_BAUD;
  USART->CR2 = 0;
  USART->CR3 = TARGET_SYSCON ? USART_CR3_HDSEL : 0; //syscon=half-duplex. duplexing is external in fixture hw
  USART->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
  */
  USART_InitTypeDef USART_InitStruct;
  USART_InitStruct.USART_BaudRate = CONTACT_BAUD;
  USART_InitStruct.USART_WordLength = USART_WordLength_8b;
  USART_InitStruct.USART_StopBits = USART_StopBits_1;
  USART_InitStruct.USART_Parity = USART_Parity_No ;
  USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
  USART_Init(USART, &USART_InitStruct);
  #if TARGET_SYSCON
    USART_HalfDuplexCmd(USART, ENABLE);
  #endif
  
  USART_Cmd(USART, ENABLE);
  delay_bit_time_(20); //2 bytes on the wire
  
  NVIC_SetPriority(USART_IRQn, 2);
  NVIC_EnableIRQ(USART_IRQn);
  USART->CR1 |= USART_CR1_RXNEIE; //peripheral int enable
  
  mode = MODE_INITIALIZED;
  Contacts::flush();
}

int Contacts::deinit(void)
{
  mode = MODE_UNINITIALIZED;
  
  NVIC_DisableIRQ(USART_IRQn);
  __NOP(); __NOP();
  
  USART->CR1 = 0; //disable usart
  
  //unconfig uart pins
  #if TARGET_SYSCON
    VEXT_SENSE::init(MODE_INPUT, PULL_NONE);
    VEXT_SENSE::alternate(0);
  #elif TARGET_FIXTURE
    CHGTX::init(MODE_INPUT, PULL_NONE);
    CHGTX::alternate(0);
    CHGRX::init(MODE_INPUT, PULL_NONE);
    CHGRX::alternate(0);
  #endif
  
  //Disable peripheral clocks
  //GPIO_RCC_EN(0); //<--can't be sure someone else isn't using this
  USART_RCC_EN(0);
  
  int n = rx.len;
  rx.len = 0;
  return n;
}

int Contacts::printf(const char *format, ... )
{
  if( mode == MODE_UNINITIALIZED )
    return 0;
  
  char dest[128];
  va_list argptr;
  
  va_start(argptr, format);
  int len = vsnprintf(dest, sizeof(dest), format, argptr);
  va_end(argptr);
  
  Contacts::write(dest);
  return len > sizeof(dest) ? sizeof(dest) : len;
}

void Contacts::write(const char* s)
{
  if( mode == MODE_UNINITIALIZED )
    return;
  
  rx_disable_();
  while(*s)
    putchar_(*s++);
  rx_enable_();
}

void Contacts::put(char c)
{
  if( mode == MODE_UNINITIALIZED )
    return;
  
  rx_disable_();
  putchar_(c);
  rx_enable_();
}

int Contacts::get(void)
{
  int c;
  if( mode == MODE_UNINITIALIZED )
    return -1;
  
  NVIC_DisableIRQ(USART_IRQn);
  __NOP(); __NOP();
  
  if( rx.len > 0 ) {
    c = rx.buf[rx.r];
    if(++rx.r >= rx_fifo_size)
      rx.r = 0;
    rx.len--;
  } 
  else {
    c = -1; //no data
  }
  
  NVIC_EnableIRQ(USART_IRQn);
  
  return c;
}

int Contacts::flush(void)
{
  int n;
  if( mode == MODE_UNINITIALIZED )
    return 0;
  
  NVIC_DisableIRQ(USART_IRQn);
  __NOP(); __NOP();
  
  n = rx.len;
  rx.len = 0;
  rx.r = 0;
  rx.w = 0;
  
  NVIC_EnableIRQ(USART_IRQn);
  
  return n;
}

namespace Contacts {
  static inline void uart_isr_handler_(char c)
  {
    //if( mode == MODE_COMMS_RX ) {
      if( rx.len < rx_fifo_size ) { //drop chars if fifo full
        rx.buf[rx.w] = c;
        if(++rx.w >= rx_fifo_size)
          rx.w = 0;
        rx.len++;
      }
    //}
  }
}

extern "C" void USART_IRQHandler(void)
{
  volatile int junk;
  
  #if TARGET_SYSCON
    if( USART->ISR & (USART_ISR_ORE | USART_ISR_FE) ) { //framing and/or overrun error
      junk = USART->RDR; //flush the rdr & shift register
      junk = USART->RDR;
      USART->ICR = USART_ICR_ORECF | USART_ICR_FECF; //clear flags
    } else if (USART->ISR & USART_ISR_RXNE) {
      Contacts::uart_isr_handler_(USART->RDR); //reading RDR clears RXNE flag
    }
    
  #elif TARGET_FIXTURE
    if( USART->SR & (USART_SR_ORE | USART_SR_FE) ) { //framing and/or overrun error
      junk = USART->DR; //flush the dr & shift register
      junk = USART->DR; //reading DR clears error flags
    } else if (USART->SR & USART_SR_RXNE) {
      Contacts::uart_isr_handler_(USART->DR); //reading DR clears RXNE flag
    }
    
  #endif
}

