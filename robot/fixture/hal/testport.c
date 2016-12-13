// Based on Drive Testfix, updated for Cozmo EP1 Testfix

// Basic operation of the testport:
//  The testport is a 500kbaud half-duplex connection using the charge contacts
//    The protocol is binary little-endian, with an 8-bit message ID, and a fixed-length per message
//  Transmit:
//    Receive and transmit use different UARTs (to workaround some electrical limitations)
//    High/idle signals use the TX GPIO to connect a 500mA 4.6V source - powerful enough to charge the robot
//    Low signals use the RX GPIO (10mA) to ground/discharge the contacts via 330 ohms 
//  Receive:
//    RX simply listens on the charge contact pin - with a ~50K pull-down
//    It is up to the robot to push the charge contacts high/low when transmitting
//    If the robot is not transmitting/not connected, RX will float low
#include "lib/stm32f2xx.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/portable.h"
#include "hal/uart.h"
#include "../app/fixture.h"
#include "hal/board.h"

#define TESTPORT       USART6  // Only works on Rev1+ fixtures
#define BAUD_RATE      100000

static __align(4) u8 m_globalBuffer[1024 * 16];

// #define DEBUG_TESTPORT

// Enable the test port
void InitTestPort(int baudRate)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  if (baudRate < 1)
    baudRate = BAUD_RATE;
  
  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
  
  // Don't enable by default
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin =  GPIOC_CHGTX;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, PINC_CHGTX, GPIO_AF_USART6);
  
  GPIO_InitStructure.GPIO_Pin =  GPIOC_CHGRX;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, PINC_CHGRX, GPIO_AF_USART6);
  
  // TESTPORT/RX config
  USART_Cmd(TESTPORT, DISABLE);
  USART_InitStructure.USART_BaudRate = baudRate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(TESTPORT, &USART_InitStructure);  
  USART_Cmd(TESTPORT, ENABLE);
}

// Send a character over the test port
void TestPutChar(u8 c)
{    
  TESTPORT->DR = c;
  // Wait for FIFO to empty and FIFO to run out
  while (!(TESTPORT->SR & USART_FLAG_TXE))
    ;
  while (!(TESTPORT->SR & USART_FLAG_TC))
    ;
}

// Receive a byte from the test port, blocking until it arrives or there is a timeout
int TestGetCharWait(int timeout)
{
  volatile u32 v;
  u32 status;
  u32 startTime;
  int value;
  
  // Check for overrun
  status = TESTPORT->SR;
  if (status & USART_SR_ORE)
  {
    v = TESTPORT->SR;
    v = TESTPORT->DR;
  }
  
  status = 0;
  value = -1;
  startTime = getMicroCounter();
  while (getMicroCounter() - startTime < timeout)
  {
    if (TESTPORT->SR & USART_SR_RXNE)
    {
      value = TESTPORT->DR & 0xFF;
      
#ifdef DEBUG_TESTPORT
    SlowPrintf(">%02x ", value);
#endif
      
      break;
    }
  }
  return value;
}

u8* GetGlobalBuffer(void)
{
  return m_globalBuffer;
}


void SendTestChar(int val)
{
  int c = val;
  u32 start = getMicroCounter();
  
  // Listen for pulse
  PIN_PULL_NONE(GPIOC, PINC_TRX);
  PIN_IN(GPIOC, PINC_TRX);
  PIN_RESET(GPIOC, PINC_CHGTX);     // Floating power
  PIN_OUT(GPIOC, PINC_CHGTX);
  PIN_PULL_UP(GPIOC, PINC_CHGRX);
  PIN_IN(GPIOC, PINC_CHGRX);

  // Wait for RX to go low/be low
  while (GPIO_READ(GPIOC) & GPIOC_CHGRX)
    if (getMicroCounter()-start > 500000)
      throw ERROR_NO_PULSE;
    
  // Now wait for it to go high
  MicroWait(1000);    // Avoid false detects due to slow discharge
  while (!(GPIO_READ(GPIOC) & GPIOC_CHGRX))
    if (getMicroCounter()-start > 500000)
      throw ERROR_NO_PULSE;
  
  // If nothing to send, just return
  if (c < 0)
    return;
    
  // Before we can send, we must drive the signal up via TX, and pull the signal down via RX
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_AF(GPIOC, PINC_CHGTX);
  
  PIN_PULL_NONE(GPIOC, PINC_CHGRX);
  PIN_RESET(GPIOC, PINC_CHGRX);
  PIN_OUT(GPIOC, PINC_CHGRX);
  
  // XXX: This is a fairly critical parameter - has to be timed to the RTOS delays in factory firmware
  // Too short is real bad, since you get a mangled byte - corrupting the command
  // Too long will miss end-of-byte - which requires a retransmit (not quite as bad)
  // There are no electrical consequences of running it too long, since it keeps the robot (externally) powered
  MicroWait(70);  // Enough time for robot to turn around - usually 30uS does it, worst I saw was 55uS
  
  TestPutChar(c);
  MicroWait(10);  // Some extra stop bit time
  
  // Back to listening for confirmation pulse
  start = getMicroCounter();
  PIN_RESET(GPIOC, PINC_CHGTX);     // Floating power
  PIN_OUT(GPIOC, PINC_CHGTX);
  PIN_PULL_NONE(GPIOC, PINC_CHGRX);
  PIN_IN(GPIOC, PINC_CHGRX);

  // Wait for acknowledge (RX to go low/pulse high/go low)
  while (GPIO_READ(GPIOC) & GPIOC_CHGRX)
    if (getMicroCounter()-start > 200)
      throw ERROR_NO_PULSE_ACK;
  while (!(GPIO_READ(GPIOC) & GPIOC_CHGRX))
    if (getMicroCounter()-start > 200)
      throw ERROR_NO_PULSE_ACK;
  while (GPIO_READ(GPIOC) & GPIOC_CHGRX)
    if (getMicroCounter()-start > 200)
      throw ERROR_NO_PULSE_ACK;
    
#ifdef DEBUG_TESTPORT
SlowPrintf("<%02x ", val);
#endif  
}

int SendCommand(u8 test, s8 param, u8 buflen, u8* buf)
{
  const int CHAR_TIMEOUT = 1000;
 
  // This is super unstable, so put a band-aid on it
  int retries = 3;
  while (retries)
    try {      
      // Clear trash from the UART buffer
      PIN_IN(GPIOC, PINC_CHGRX);
      while (-1 != TestGetCharWait(1))
        ;
      
      // Send command
      SendTestChar('W');
      SendTestChar('t');
      SendTestChar(param & 0xff);
      SendTestChar(test & 0xff);
      
      // Now read reply
      PIN_AF(GPIOC, PINC_CHGRX);
      int c;
      // Scan for header
      while ((0xBE^0xFF) != (c = TestGetCharWait(CHAR_TIMEOUT)))
        if (c == -1)
          throw ERROR_TESTPORT_TIMEOUT;
      u8 len = TestGetCharWait(CHAR_TIMEOUT);
      if (len > buflen)
        throw ERROR_TESTPORT_TMI;
      for (int i = 0; i < len; i++)
      {
        int c = TestGetCharWait(CHAR_TIMEOUT);
        if (c < 0)
          throw ERROR_TESTPORT_TIMEOUT;
        *buf++ = c;
        if (0 != TestGetCharWait(CHAR_TIMEOUT))
          throw ERROR_TESTPORT_PADDING;
      }
      if ((0xEF^0xFF) != TestGetCharWait(CHAR_TIMEOUT))
        throw ERROR_TESTPORT_TIMEOUT;
      PIN_IN(GPIOC, PINC_CHGRX);
          
#ifdef DEBUG_TESTPORT
    SlowPrintf(" - OK!\r\n");
#endif
      
      return len;
    } catch (int err) {
      retries--;
      if (!retries)
        throw err;
    }
  throw ERROR_UNKNOWN_MODE;
}
