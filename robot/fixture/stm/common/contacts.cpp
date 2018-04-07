#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "board.h"
#include "common.h"
#include "contacts.h"
#include "timer.h"

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

#define CONTACTS_DEBUG  0
  
#if CONTACTS_DEBUG && TARGET_FIXTURE
#include "console.h"
#else //syscon doesn't have a debug output
#undef CONTACTS_DEBUG
#define CONTACTS_DEBUG  0
#endif

namespace Contacts
{
  enum CONTACT_MODES {
    MODE_UNINITIALIZED = 0,
    MODE_IDLE, //intermediate state. pins/peripherals disabled.
    MODE_POWER,
    MODE_TX,
    MODE_RX,
  };
  
  static const uint32_t CONTACT_BAUD = 115200;
  static int mode = MODE_UNINITIALIZED;
  static bool m_console_echo = 0;
  
  //recieve buffer
  static const int rx_fifo_size = 0x40;
  static struct {
    char buf[rx_fifo_size]; 
    volatile int len; 
    int w, r;
  } rx;
  
  static volatile int framing_err_cnt = 0, overflow_err_cnt = 0, drop_char_cnt = 0;
  
  static inline void delay_bit_time_(const uint16_t n) {
    Timer::wait(1 + ( ((uint32_t)n*1000*1000) / CONTACT_BAUD) );
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
      
      if( CONTACT_BAUD > 57600 ) //bandwidth throttle for 115.2k
        delay_bit_time_(10);
    #endif
  }
}

//------------------------------------------------------------------------------------------------------
//                                        (de)Init
//------------------------------------------------------------------------------------------------------

void Contacts::init(void)
{
  if( mode != MODE_UNINITIALIZED )
    return;
  
  //Enable peripheral clocks
  GPIO_RCC_EN(1);
  USART_RCC_EN(1);
  
  Contacts::setModeIdle();
  
  //configure uart pin(s)
  #if TARGET_SYSCON
    VEXT_SENSE::init(MODE_INPUT, PULL_NONE, TYPE_PUSHPULL);
    VEXT_SENSE::alternate(GPIO_AF_1); //USART2_TX
  #elif TARGET_FIXTURE
    if( (int)Board::revision >= BOARD_REV_2_0 ) {
      CHGPWR::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL); //always drive, to override CHGTX_4V control
    }
    CHGRX::init(MODE_INPUT, PULL_NONE, TYPE_PUSHPULL);
    CHGTX::init(MODE_INPUT, PULL_NONE, TYPE_PUSHPULL);
    CHGTX::alternate(GPIO_AF_USART6);
    CHGRX::alternate(GPIO_AF_USART6);
  #endif
  
  //Configure UART
  USART_InitTypeDef USART_InitStruct;
  USART_InitStruct.USART_BaudRate   = CONTACT_BAUD;
  USART_InitStruct.USART_WordLength = USART_WordLength_8b;
  USART_InitStruct.USART_StopBits   = USART_StopBits_1;
  USART_InitStruct.USART_Parity     = USART_Parity_No ;
  USART_InitStruct.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx;
  USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_Init(USART, &USART_InitStruct);
  USART_ITConfig(USART, USART_IT_RXNE, ENABLE); //rx int enable
  NVIC_SetPriority(USART_IRQn, 2);
  
  #if CONTACTS_DEBUG
    ConsolePrintf("DBG Contacts::init()\n");
    
    //what the @%*# is our clock??
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);
    ConsolePrintf("  HCLK=%u SYSCLK=%u PCLK1=%u PCLK2=%u\n", RCC_ClocksStatus.HCLK_Frequency, RCC_ClocksStatus.SYSCLK_Frequency, RCC_ClocksStatus.PCLK1_Frequency, RCC_ClocksStatus.PCLK2_Frequency );
    ConsolePrintf("  USART using PCLK%u\n", (USART==USART1 || USART==USART6 ? 2 : 1) );
    
    //let's see what the usart driver calcualted for register values...
    ConsolePrintf("  CR1=0x%04x CR2=0x%04x CR3=0x%04x BRR=0x%04x\n", USART->CR1, USART->CR2, USART->CR3, USART->BRR);
    ConsolePrintf("  UE=%u TE=%u RE=%u HDSEL=%u\n", (USART->CR1 & USART_CR1_UE) > 0, (USART->CR1 & USART_CR1_TE) > 0, (USART->CR1 & USART_CR1_RE) > 0, (USART->CR3 & USART_CR3_HDSEL) > 0 );
  #endif
}

void Contacts::deinit(void)
{
  if( mode == MODE_UNINITIALIZED )
    return;
  
  Contacts::setModeIdle();
  
  //deconfigure pin(s)
  #if TARGET_SYSCON
    //XXX: not implemented
  #elif TARGET_FIXTURE
    if( (int)Board::revision >= BOARD_REV_2_0 ) {
      CHGPWR::init(MODE_INPUT, PULL_NONE); //release power ctrl
    }
  #endif
  
  //Disable peripheral clocks
  //GPIO_RCC_EN(0);
  //USART_RCC_EN(0);
  
  mode = MODE_UNINITIALIZED;
}

void Contacts::echo(bool on)
{
  #if TARGET_SYSCON
    m_console_echo = on;
  #elif TARGET_FIXTURE
    m_console_echo = 0; //not supported on fixture
  #endif
}

void Contacts::setModeIdle(void)
{
  if( mode == MODE_IDLE )
    return;
  
  //disconnect pins from peripheral (default state set during init)
  #if TARGET_SYSCON
    VEXT_SENSE::mode(MODE_INPUT);
  #elif TARGET_FIXTURE
    if( (int)Board::revision >= BOARD_REV_2_0 ) {
      CHGTX_EN::init(MODE_INPUT, PULL_NONE); //4V TX driver disabled
      CHGPWR::reset(); //5V charge power disabled
    }
    CHGRX::mode(MODE_INPUT);
    CHGTX::mode(MODE_INPUT);
  #endif
  
  NVIC_DisableIRQ(USART_IRQn);
  USART_Cmd(USART, DISABLE);
  mode = MODE_IDLE;
}

//------------------------------------------------------------------------------------------------------
//                                        Power Mode
//------------------------------------------------------------------------------------------------------

void Contacts::setModePower(void)
{
  if( mode == MODE_POWER )
    return;
  if( mode == MODE_UNINITIALIZED )
    Contacts::init();

  Contacts::setModeIdle();
  
  #if TARGET_SYSCON
    //doesn't supply power. effectively in idle mode.
  #elif TARGET_FIXTURE
    //if( (int)Board::revision >= BOARD_REV_2_0 ) {
    //  CHGPWR::reset(); //5V charge power disabled
    //  CHGPWR::init(MODE_OUTPUT, PULL_NONE);
    //}
    CHGTX::reset(); //VEXT floating(off)
    CHGTX::mode(MODE_OUTPUT);
    CHGRX::reset(); //prime output register for force discharge
  #endif
  
  mode = MODE_POWER;
}

void Contacts::powerOn(int turn_on_delay_ms)
{
  Contacts::setModePower();
  
  #if TARGET_SYSCON
    //nothing to do
  #elif TARGET_FIXTURE
    if( !powerIsOn() ) {
      CHGTX::set(); //VEXT drive 5V
      if( (int)Board::revision >= BOARD_REV_2_0 )
        CHGPWR::set(); //DUT_VEXT drive 5V
      Timer::delayMs(turn_on_delay_ms);
    }
  #endif
}

void Contacts::powerOff(int turn_off_delay_ms, bool force)
{
  Contacts::setModePower();
  
  #if TARGET_SYSCON
    //nothing to do
  #elif TARGET_FIXTURE
    if( powerIsOn() )
    {
      CHGTX::reset(); //VEXT float/off
      if( (int)Board::revision >= BOARD_REV_2_0 )
        CHGPWR::reset(); //DUT_VEXT disconnect from 5V
      
      if( force ) //discharge VEXT through 330R
        CHGRX::mode(MODE_OUTPUT);
      
      Timer::delayMs(turn_off_delay_ms);
      CHGRX::mode(MODE_INPUT); //release drain
    }
  #endif
}

bool Contacts::powerIsOn(void)
{
  #if TARGET_SYSCON
    return false;
  #elif TARGET_FIXTURE
    return mode == MODE_POWER && CHGTX::read();
  #endif
}

//------------------------------------------------------------------------------------------------------
//                                        Uart TX Mode
//------------------------------------------------------------------------------------------------------

void Contacts::setModeTx(void)
{
  if( mode == MODE_TX )
    return;
  if( mode == MODE_UNINITIALIZED )
    Contacts::init();

  Contacts::setModeIdle();
  
  #if TARGET_SYSCON
    VEXT_SENSE::mode(MODE_ALTERNATE); //connect pin to uart
    USART_HalfDuplexCmd(USART, DISABLE); //disconnect uart RX
    USART_Cmd(USART, ENABLE);
    delay_bit_time_(2); //wait at least 1 bit-time for uart peripheral to spin up
    
  #elif TARGET_FIXTURE
    //v1.0 - TX pin controls a high-current FET. 1->VEXT=5V 0->VEXT=Z
    //v2.0 - TX pin drives a 4V push-pull buffer
    CHGTX::mode(MODE_ALTERNATE); //hand over to uart control
    USART_Cmd(USART, ENABLE);
    delay_bit_time_(2); //wait at least 1 bit-time for uart peripheral to spin up
    
    if( (int)Board::revision >= BOARD_REV_2_0 ) {
      CHGTX_EN::reset();  //enable 4V TX driver
      CHGTX_EN::init(MODE_OUTPUT);
    } else {
      //Drive CHGRX low for the other half of push-pull (pulls VEXT down through 330R)
      CHGRX::reset();
      CHGRX::mode(MODE_OUTPUT);
    }
    
  #endif
  
  delay_bit_time_(20); //2 byte times on the wire for receiver to sync line change
  mode = MODE_TX;
}

int Contacts::printf(const char *format, ... )
{
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
  Contacts::setModeTx();
  
  rx_disable_(); //protect half-duplex uart (syscon) from echo
  while(*s)
    putchar_(*s++);
  rx_enable_();
}

void Contacts::putchar(char c)
{
  Contacts::setModeTx();
  
  rx_disable_(); //protect half-duplex uart (syscon) from echo
  putchar_(c);
  rx_enable_();
}

//------------------------------------------------------------------------------------------------------
//                                        Uart RX Mode
//------------------------------------------------------------------------------------------------------

namespace Contacts {
  //getline() local buffer
  #if TARGET_SYSCON
  const  int  line_maxlen = 31;
  #else //TARGET_FIXTURE
  const  int  line_maxlen = 127;
  #endif
  static char m_line[line_maxlen+1];
  static int  line_len = 0;
}

void Contacts::setModeRx(void)
{
  if( mode == MODE_RX )
    return;
  if( mode == MODE_UNINITIALIZED )
    Contacts::init();
  
  Contacts::setModeIdle();
  
  #if TARGET_SYSCON
    VEXT_SENSE::mode(MODE_ALTERNATE); //connect pin to uart
    USART_HalfDuplexCmd(USART, ENABLE); //connect uart RX to TX pin
    USART_Cmd(USART, ENABLE);
    delay_bit_time_(2); //wait at least 1 bit-time for uart peripheral to spin up
    
  #elif TARGET_FIXTURE
    //float VEXT and turn on the receiver
    CHGRX::mode(MODE_ALTERNATE);
    USART_Cmd(USART, ENABLE);
    delay_bit_time_(2); //wait at least 1 bit-time for uart peripheral to spin up
    
  #endif
  
  //clear rx buffer
  line_len = 0;
  rx.len = 0;
  rx.r = 0;
  rx.w = 0;
  
  //framing_err_cnt = 0;
  //overflow_err_cnt = 0;
  //drop_char_cnt = 0;
  
  NVIC_EnableIRQ(USART_IRQn);
  mode = MODE_RX;
}

int Contacts::getchar(void)
{
  Contacts::setModeRx();
  
  NVIC_DisableIRQ(USART_IRQn);
  __NOP();
  __NOP();
  
  int c;
  if( rx.len > 0 ) {
    c = rx.buf[rx.r];
    if(++rx.r >= rx_fifo_size)
      rx.r = 0;
    rx.len--;
  } else
    c = -1; //no data
  
  NVIC_EnableIRQ(USART_IRQn);
  
  return c;
}

namespace Contacts {
  
  void putecho(char c) {
    if( m_console_echo ) {
      int len = line_len;
      Contacts::setModeTx(); //delay_bit_time_(20); //bus turnaround
      Contacts::putchar(c);
      Contacts::setModeRx();
      line_len = len; //must restore line len, cleared on mode switch
    }
  }
  
  char* line_process_char_(char c, int* out_len) //return ptr to full line, when available
  {
    switch(c)
    {
      case '\r': //Return, EOL
      case '\n':
        Contacts::putecho('\n');
        if(line_len)
        {
          m_line[line_len] = '\0';
          if(out_len)
            *out_len = line_len;
          line_len = 0;
          return m_line;
        }
        break;
      
      case 0x08: //Backspace
        if(line_len)
        {
          line_len--;
          Contacts::putecho(0x08);
          Contacts::putecho(' '); //overwrite terminal char with a space
          Contacts::putecho(0x08);
        }
        break;
      
      default:
        if( c >= ' ' && c <= '~' ) //printable char set
        {
          if( line_len < line_maxlen ) {
            m_line[line_len++] = c;
            Contacts::putecho(c);
          }
        }
        //else, ignore all other chars
        break;
    }
    return NULL;
  }
}

char* Contacts::getline(int timeout_us, int *out_len)
{
  int c;
  char *line = NULL;
  
  uint32_t start = Timer::get();
  do { //read 1 char (even if timeout==0)
    if( (c = Contacts::getchar()) > 0 ) //ignore null; messes with ascii parser
      line = line_process_char_(c, out_len);
  } while( !line && Timer::elapsedUs(start) < timeout_us );
  
  return line;
}

char* Contacts::getlinebuffer(int *out_len) {
  if(out_len)
    *out_len = line_len; //report length
  return m_line;
}

int Contacts::flushRx(void)
{
  if( mode != MODE_RX )
    return 0;
  
  NVIC_DisableIRQ(USART_IRQn);
  __NOP();
  __NOP();
  
  int n = rx.len + line_len;
  line_len = 0;
  rx.len = 0;
  rx.r = 0;
  rx.w = 0;
  
  framing_err_cnt = 0;
  overflow_err_cnt = 0;
  drop_char_cnt = 0;
  
  NVIC_EnableIRQ(USART_IRQn);
  
  return n;
}

namespace Contacts
{
  static inline void uart_isr_handler_(char c)
  {
    if( mode == MODE_RX ) {
      if( rx.len < rx_fifo_size ) { //drop chars if fifo full
        rx.buf[rx.w] = c;
        if(++rx.w >= rx_fifo_size)
          rx.w = 0;
        rx.len++;
      } else {
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

extern "C" void USART_IRQHandler(void)
{
  volatile int junk;
  
  #if TARGET_SYSCON
    uint32_t status = USART->ISR;
    if( status & (USART_ISR_ORE | USART_ISR_FE) ) //framing and/or overrun error
    {
      if( status & USART_ISR_ORE )
        Contacts::overflow_err_cnt++;
      if( status & USART_ISR_FE )
        Contacts::framing_err_cnt++;
      
      junk = USART->RDR; //flush the rdr & shift register
      junk = USART->RDR;
      USART->ICR = USART_ICR_ORECF | USART_ICR_FECF; //clear flags
    } else if (USART->ISR & USART_ISR_RXNE) {
      Contacts::uart_isr_handler_(USART->RDR); //reading RDR clears RXNE flag
    }
    
  #elif TARGET_FIXTURE
    uint32_t status = USART->SR;
    if( status & (USART_SR_ORE | USART_SR_FE) ) //framing and/or overrun error
    {
      if( status & USART_SR_ORE )
        Contacts::overflow_err_cnt++;
      if( status & USART_SR_FE )
        Contacts::framing_err_cnt++;
      
      junk = USART->DR; //flush the dr & shift register
      junk = USART->DR; //reading DR clears error flags
    } else if (USART->SR & USART_SR_RXNE) {
      Contacts::uart_isr_handler_(USART->DR); //reading DR clears RXNE flag
    }
    
  #endif
}

