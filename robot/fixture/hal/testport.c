// Basic operation of the testport:
//  The testport is a 500kbaud half-duplex connection using the charge contacts
//    The protocol is binary little-endian, with an 8-bit message ID, and a fixed-length per message
//  Transmit:
//    Receive and transmit use different UARTs (to workaround some electrical limitations)
//    High/idle signals use the TX GPIO to connect a 500mA 4.6V source - powerful enough to charge the car
//    Low signals use the RX GPIO (10mA) to ground/discharge the contacts via 330 ohms 
//  Receive:
//    RX simply listens on the charge contact pin - with a ~50K pull-down
//    It is up to the car to push the charge contacts high/low when transmitting
//    If the car is not transmitting/not connected, RX will float low
#include "lib/stm32f2xx.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/portable.h"
#include "../app/fixture.h"

#define TESTPORT_TX       UART5
#define TESTPORT_RX       USART3
#define BAUD_RATE         500000

#define PINC_TX           12
#define GPIOC_TX          (1 << PINC_TX)
#define PINC_RX           10
#define GPIOC_RX          (1 << PINC_RX)

#define TIMEOUT 800000  // 800 ms, some commands take a while...

//#define DEBUG_RECEIVE
//#define DEBUG_TRANSMIT

static __align(4) u8 m_globalBuffer[1024 * 5];

#define BUFFER_LENGTH (1024 + 8)
static u8 m_buffer[BUFFER_LENGTH];
static u32 m_bufferIndex = 0;
static BOOL m_isInTxMode = TRUE;

extern void SlowPrintf(const char* format, ...);

// Enable the test port
void InitTestPort(int baudRate)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  if (baudRate < 1)
    baudRate = BAUD_RATE;
  
  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
  
  USART_HalfDuplexCmd(TESTPORT_RX, ENABLE);  // Enable the pin for transmitting and receiving
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin =  GPIOC_TX;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, PINC_TX, GPIO_AF_UART5);
  
  GPIO_InitStructure.GPIO_Pin =  GPIOC_RX;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, PINC_RX, GPIO_AF_USART3);
  
  // TESTPORT_TX config
  USART_Cmd(TESTPORT_TX, DISABLE);
  USART_InitStructure.USART_BaudRate = baudRate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx;
  USART_Init(TESTPORT_TX, &USART_InitStructure);  
  USART_Cmd(TESTPORT_TX, ENABLE);
  
  // TESTPORT_RX config
  USART_Cmd(TESTPORT_RX, DISABLE);
  
  USART_InitStructure.USART_Mode = USART_Mode_Rx;
  USART_Init(TESTPORT_RX, &USART_InitStructure);  
  USART_Cmd(TESTPORT_RX, ENABLE);
  
  // Pull PC10 low
  GPIO_ResetBits(GPIOC, GPIO_Pin_10);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  // Pull PC12 low
  GPIO_ResetBits(GPIOC, GPIO_Pin_12);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  TestEnableRx();
}

void TestEnable(void)
{
  USART_Cmd(TESTPORT_TX, ENABLE);
  USART_Cmd(TESTPORT_RX, ENABLE);
}

void TestDisable(void)
{
  USART_Cmd(TESTPORT_TX, DISABLE);
  USART_Cmd(TESTPORT_RX, DISABLE);
}

void TestClear(void)
{
  volatile u32 v;
  while (!(TESTPORT_TX->SR & USART_FLAG_TXE))
    v = TESTPORT_TX->DR;
}

// Send a character over the test port
void TestPutChar(u8 c)
{    
#ifdef DEBUG_TRANSMIT
  SlowPutChar('<');
  SlowPutHex(c);
#endif
  
  TESTPORT_TX->DR = c;
  while (!(TESTPORT_TX->SR & USART_FLAG_TXE))
    ;
}

// Send a string over the test port
void TestPutString(char *s)
{
  while (*s)
    TestPutChar(*s++);
}

// Receive a byte from the test port, blocking until it arrives or there is a timeout
int TestGetCharWait(int timeout)
{
  volatile u32 v;
  u32 status;
  u32 startTime;
  int value;
  
  // Check for overrun
  status = TESTPORT_RX->SR;
  if (status & USART_SR_ORE)
  {
    v = TESTPORT_RX->SR;
    v = TESTPORT_RX->DR;
  }
  
  status = 0;
  value = -1;
  startTime = getMicroCounter();
  while (getMicroCounter() - startTime < timeout)
  {
    if (TESTPORT_RX->SR & USART_SR_RXNE)
    {
      value = TESTPORT_RX->DR & 0xFF;
      
#ifdef DEBUG_RECEIVE
      SlowPutChar('>');
      SlowPutHex(value);
#endif
      
      break;
    }
  }
  return value;
}

void TestEnableTx(void)
{
  // Wait until tranfer is complete
  while (!(TESTPORT_TX->SR & USART_FLAG_TC))
    ;
    
  if (!m_isInTxMode)
  {
    GPIO_RESET(GPIOC, PINC_RX);
    PIN_OUT(GPIOC, PINC_RX);
    PIN_AF(GPIOC, PINC_TX);
    volatile u32 v = TESTPORT_RX->SR;
    USART_Cmd(TESTPORT_RX, DISABLE);
    MicroWait(40);
    m_isInTxMode = 1;
  }
}

void TestEnableRx(void)
{
  volatile u32 v;
  if (m_isInTxMode)
  {
    // Wait until tranfer is complete
    while (!(TESTPORT_TX->SR & USART_FLAG_TC))
      ;

    GPIO_RESET(GPIOC, PINC_TX);    
    PIN_OUT(GPIOC, PINC_TX);
    PIN_AF(GPIOC, PINC_RX);
    MicroWait(20);
    USART_Cmd(TESTPORT_RX, ENABLE);
    v = TESTPORT_RX->SR;
    v = TESTPORT_RX->DR;
    m_isInTxMode = 0;
  }
}

void TestWaitForTransmissionComplete(void)
{
  while (!(TESTPORT_TX->SR & USART_FLAG_TC))
    ;
}

error_t SendCommand(u8* receiveBuffer, u32 receiveLength)
{
  u32 i;
  int value;
  error_t ret = ERROR_OK;
  
  if (m_bufferIndex == 0)
  {
    throw ERROR_EMPTY_COMMAND;
  }
  
  // We disable IRQ's so the USB interrupts don't make us lose UART data...
  // It's a long time to disable interrupts, but USB and flow-control should
  // be just fine, despite disabling for many milliseconds at a time.
  __disable_irq();
  
  // Send the command byte and check for acknowledgement
  TestEnableTx();
  TestPutChar(m_buffer[0]);
  
  TestEnableRx();
  value = TestGetCharWait(TIMEOUT);
  if (value != DMC_ACK)
  {
    SlowPrintf("FAULT: received %02X ACK1, %02X\r\n", value, m_buffer[0]);
    ret = ERROR_ACK1;
    goto cleanup;
  }
  
  // Send the remaining data in the buffer
  if (m_bufferIndex > 1)
  {
    TestEnableTx();
    for (i = 1; i < m_bufferIndex; i++)
    {
      TestPutChar(m_buffer[i]);
    }
  }
  
  // Receive data from the vehicle
  if (receiveBuffer && receiveLength)
  {
    TestEnableRx();
    for (i = 0; i < receiveLength; i++)
    {
      value = TestGetCharWait(TIMEOUT);
      if (value < 0)
      {
        SlowPrintf("TIMEOUT on byte: %i\r\n", i);
        ret = ERROR_RECEIVE;
        goto cleanup;
      }
      
      *receiveBuffer++ = (u8)value;
    }
  }
  
  // Get the final acknowledgement.
  TestEnableRx();
  value = TestGetCharWait(TIMEOUT);
  if (value != DMC_ACK)
  {
    SlowPrintf("FAULT: received %02X ACK2, %02X\r\n", value, m_buffer[0]);
    ret = ERROR_ACK2;
    goto cleanup;
  }

cleanup:
  TestEnableRx();
  m_bufferIndex = 0;
  __enable_irq();
  
  if (ret != ERROR_OK)
    throw ret;
  
  return ret;
}

error_t SendBuffer(u8 command, u8* buffer, u32 length)
{
  u32 i;
  int value;
  error_t ret = ERROR_OK;
  
  // We disable IRQ's so the USB interrupts don't make us lose UART data...
  // It's a long time to disable interrupts, but USB and flow-control should
  // be just fine, despite disabling for many milliseconds at a time.
  __disable_irq();
  
  // Send the command byte and check for acknowledgement
  TestEnableTx();
  TestPutChar(command);
  
  TestEnableRx();
  value = TestGetCharWait(TIMEOUT);
  if (value != DMC_ACK)
  {
    SlowPrintf("FAULT: received %02X ACK1, %02X\r\n", value, command);
    ret = ERROR_ACK1;
    goto cleanup;
  }
  
  // Send the remaining data in the buffer
  if (length > 0)
  {
    TestEnableTx();
    for (i = 0; i < length; i++)
    {
      TestPutChar(buffer[i]);
    }
  }
  
  // Get the final acknowledgement.
  TestEnableRx();
  value = TestGetCharWait(TIMEOUT);
  if (value != DMC_ACK)
  {
    SlowPrintf("FAULT: received %02X ACK2, %02X\r\n", value, command);
    ret = ERROR_ACK2;
    goto cleanup;
  }

cleanup:
  __enable_irq();
  
  if (ret != ERROR_OK)
    throw ret;
  
  return ret;
}

void Put8(u8 value)
{
  if (m_bufferIndex < BUFFER_LENGTH)
  {
    m_buffer[m_bufferIndex] = value;
    m_bufferIndex++;
  }
}

void Put16(u16 value)
{
  Put8(value);
  Put8(value >> 8);
}

void Put32(u32 value)
{
  Put16(value);
  Put16(value >> 16);
}
  
u8 Receive8(u8* value)
{
  int v = TestGetCharWait(TIMEOUT);
  if (v == -1)
    return 1;
  
  *value = v;
  return 0;
}

u8 Receive16(u16* value)
{
  u8 b0, b1;
  if (Receive8(&b0))
  {
    return 1;
  }
  
  if (Receive8(&b1))
  {
    return 1;
  }
  
  *value = (b1 << 8) | b0;
  return 0;
}

u8 Receive32(u32* value)
{
  u16 b0, b1;
  
  if (Receive16(&b0))
  {
    return 1;
  }
  
  if (Receive16(&b1))
  {
    return 1;
  }
  
  *value = (b1 << 16) | b0;
  return 0;
}

u16 Construct16(u8* data)
{
  return data[0] | (data[1] << 8);
}

u32 Construct32(u8* data)
{
  return Construct16(data) | (Construct16(&data[2]) << 16);
}

u8* GetGlobalBuffer(void)
{
  return m_globalBuffer;
}
