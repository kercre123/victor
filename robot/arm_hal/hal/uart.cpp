#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "portable.h"

#define BAUDRATE 1000000

#define RCC_GPIO        RCC_AHB1Periph_GPIOC
#define RCC_DMA         RCC_AHB1Periph_DMA1
#define RCC_UART        RCC_APB1Periph_UART4
#define GPIO_AF         GPIO_AF_UART4
#define UART            UART4

#define DMA_STREAM_RX   DMA1_Stream2
#define DMA_CHANNEL_RX  DMA_Channel_4

#define DMA_STREAM_TX   DMA1_Stream4
#define DMA_CHANNEL_TX  DMA_Channel_4
#define DMA_IRQ_TX      DMA1_Stream4_IRQn

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      /////////////////////////////////////////////////////////////////////
      // UART
      //
      OFFCHIP u8 m_bufferWrite[1024 * 1024 * 1];
      OFFCHIP u8 m_bufferRead[1024];
      
      volatile s32 m_DMAWriteTail = 0;
      volatile s32 m_DMAWriteHead = 0;
      volatile s32 m_DMAWriteLength = 0;
      u32 m_DMAReadIndex = 0;
      
      volatile bool m_isDMARunning = false;
      
      static void FlushDMA()
      {
        int tail = m_DMAWriteTail;
        int length = m_DMAWriteHead - tail;
        if (length < 0)
          length = sizeof(m_bufferWrite) - tail;
        if (length > 65535)
          length = 65535;
        
        DMA_STREAM_TX->NDTR = length;                     // Buffer size
        DMA_STREAM_TX->M0AR = (u32)&m_bufferWrite[tail];  // Buffer address
        
        m_DMAWriteLength = length;
        m_isDMARunning = true;
        
        DMA_STREAM_TX->CR |= DMA_SxCR_EN; // Enable DMA
      }
      
      int UARTGetFreeSpace()
      {
        int tail = m_DMAWriteTail;
        if (m_DMAWriteHead < tail)
          return tail - m_DMAWriteHead;
        else
          return sizeof(m_bufferWrite) - (m_DMAWriteHead - tail);
      }
      
      void UARTInit()
      {
        GPIO_PIN_SOURCE(TX, GPIOC, 10);
        GPIO_PIN_SOURCE(RX, GPIOC, 11);
        
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_GPIO, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_DMA, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_UART, ENABLE);
        
        // Configure the pins for UART in AF mode
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = PIN_TX;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_TX, &GPIO_InitStructure);
        
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Pin = PIN_RX;
        GPIO_Init(GPIO_RX, &GPIO_InitStructure);
        
        GPIO_PinAFConfig(GPIO_TX, SOURCE_TX, GPIO_AF);
        GPIO_PinAFConfig(GPIO_RX, SOURCE_RX, GPIO_AF);
        
        // Configure the UART for the appropriate baudrate
        USART_InitTypeDef USART_InitStructure;
        USART_Cmd(UART, DISABLE);
        USART_InitStructure.USART_BaudRate = BAUDRATE;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        USART_Init(UART, &USART_InitStructure);
        USART_Cmd(UART, ENABLE);
        
        // Configure DMA for receiving
        DMA_DeInit(DMA_STREAM_RX);
        
        DMA_InitTypeDef DMA_InitStructure;
        DMA_InitStructure.DMA_Channel = DMA_CHANNEL_RX;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART->DR;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)m_bufferRead;
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
        DMA_InitStructure.DMA_BufferSize = sizeof(m_bufferRead);
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
        DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_Init(DMA_STREAM_RX, &DMA_InitStructure);
        
        // Enable DMA
        USART_DMACmd(UART, USART_DMAReq_Rx, ENABLE);
        DMA_Cmd(DMA_STREAM_RX, ENABLE);
        
        // Configure DMA For transmitting
        DMA_DeInit(DMA_STREAM_TX);
        
        DMA_InitStructure.DMA_Channel = DMA_CHANNEL_TX;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)m_bufferWrite;
        DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
        DMA_InitStructure.DMA_BufferSize = sizeof(m_bufferWrite);
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;  // Use in combination with FIFO to increase throughput? Needs to be divisible by 1, 1/4, 1/2 of FIFO size
        DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;  // See comment above
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_Init(DMA_STREAM_TX, &DMA_InitStructure);
        
        // Enable UART DMA, but don't start the actual DMA engine
        USART_DMACmd(UART, USART_DMAReq_Tx, ENABLE);
        
        // Note: DMA is not enabled for TX here, because the buffer is empty.
        // After main/long execution, DMA will be enabled for a specified
        // length.
        
        // Enable interrupt on DMA transfer complete for TX.
        // This is mainly for LongExecution.
        DMA_ITConfig(DMA_STREAM_TX, DMA_IT_TC, ENABLE);
        
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = DMA_IRQ_TX;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  // Don't want this to be a very high priority
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
        
        m_DMAWriteHead = m_DMAWriteTail = m_DMAReadIndex = 0;
      }

      int UARTPutChar(int c)
      {
        //UART->DR = c;
        //while (!(UART->SR & USART_FLAG_TXE))
        //  ;
        //return c;

        // Leave one guard byte in the buffer
        while (UARTGetFreeSpace() <= 2)
          ;
        
        __disable_irq();
        m_bufferWrite[m_DMAWriteHead] = c;
        m_DMAWriteHead++;
        if (m_DMAWriteHead >= sizeof(m_bufferWrite))
        {
          m_DMAWriteHead = 0;
        }
        
        // Enable DMA if it's not already running
        if (!m_isDMARunning)
        {
          FlushDMA();
        }
        
        __enable_irq();
        
        return c;
      }
      
      bool UARTPutBuffer(u8* buffer, u32 length)
      {
        bool result = false;
        
        __disable_irq();
        int bytesLeft = UARTGetFreeSpace();
        
        // Leave one guard byte
        if (bytesLeft > (length + 1))
        {
          result = true;
          
          bytesLeft = sizeof(m_bufferWrite) - m_DMAWriteHead;
          if (length <= bytesLeft)
          {
            memcpy(&m_bufferWrite[m_DMAWriteHead], buffer, length);
            m_DMAWriteHead += length;
            
            if (m_DMAWriteHead == sizeof(m_bufferWrite))
            {
              m_DMAWriteHead = 0;
            }
          } else {
            // Copy to the end of the buffer, then wrap around for the rest
            int lengthFirst = bytesLeft;
            memcpy(&m_bufferWrite[m_DMAWriteHead], buffer, lengthFirst);
            
            bytesLeft = length - lengthFirst;
            memcpy(m_bufferWrite, &buffer[lengthFirst], bytesLeft);
            
            m_DMAWriteHead = bytesLeft;
          }
          
          // Enable DMA if it's not already running
          if (!m_isDMARunning)
          {
            FlushDMA();
          }
        }
        __enable_irq();
        
        return result;
      }

      void UARTPutString(const char* s)
      {
        while (*s)
          UARTPutChar(*s++);
      }
      
      void UARTPutHex(u8 c)
      {
        static u8 hex[] = "0123456789ABCDEF";
        UARTPutChar(hex[c >> 4]);
        UARTPutChar(hex[c & 0xF]);
      }
      
      void UARTPutHex32(u32 value)
      {
        UARTPutHex(value >> 24);
        UARTPutHex(value >> 16);
        UARTPutHex(value >> 8);
        UARTPutHex(value);
      }

      s32 UARTGetChar(u32 timeout)
      {
        u32 startTime = GetMicroCounter();

        do
        {
          // Make sure there's data in the FIFO
          // NDTR counts down...
          if (DMA_STREAM_RX->NDTR != sizeof(m_bufferRead) - m_DMAReadIndex)
          {
            u8 value = m_bufferRead[m_DMAReadIndex];
            m_DMAReadIndex = (m_DMAReadIndex + 1) % sizeof(m_bufferRead);  
            return value;
          }
        }
        while ((GetMicroCounter() - startTime) < timeout);

        // No data, so return with an error
        return -1;
      }
      
      void USBSendBuffer(const u8* buffer, const u32 size)
      {
        for (u32 i=0; i<size; ++i) {
          UARTPutChar(buffer[i]);
        }
      }
			
      u32 USBRecvBuffer(u8* buffer, const u32 max_size)
      {
        u32 i;
        for (i=0; i<max_size; ++i) {
          s32 c = UARTGetChar(0);
          if (c<0) {
            return i;
          }
          buffer[i] = c;
        }
        return i;
      }
    }
  }
}

// Override fputc and fgetc for our own UART methods
int std::fputc(int c, FILE* f)
{
  return Anki::Cozmo::HAL::UARTPutChar(c);
}

int std::fgetc(FILE* f)
{
  return Anki::Cozmo::HAL::UARTGetChar();
}

extern "C"
void DMA1_Stream4_IRQHandler()
{
  using namespace Anki::Cozmo::HAL;
  
  // Clear DMA Transfer Complete flag
  DMA_ClearFlag(DMA1_Stream4, DMA_FLAG_TCIF4);
  
  m_DMAWriteTail += m_DMAWriteLength;
  if (m_DMAWriteTail >= sizeof(m_bufferWrite))
    m_DMAWriteTail = 0;
  
  // Check if there's more data to be transferred
  if (m_DMAWriteHead != m_DMAWriteTail)
  {
    FlushDMA();
  } else {
    m_isDMARunning = false;
  }
}
