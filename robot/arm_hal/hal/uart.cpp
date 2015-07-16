#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "portable.h"

// Baudrate communicating to Espressif
#define BAUDRATE 5000000

#define RCC_GPIO        RCC_AHB1Periph_GPIOB
#define RCC_DMA         RCC_AHB1Periph_DMA2
#define RCC_UART        RCC_APB2Periph_USART1
#define GPIO_AF         GPIO_AF_USART1
#define UART            USART1

#define DMA_STREAM_RX   DMA2_Stream2
#define DMA_CHANNEL_RX  DMA_Channel_4

#define DMA_STREAM_TX   DMA2_Stream7
#define DMA_CHANNEL_TX  DMA_Channel_4
#define DMA_IRQ_TX      DMA2_Stream7_IRQn

// This enables "boardPrintf" - a blocking UART for HAL/board bring-up use only - when all else is broken
// Because it is very slow and blocking, it is not suitable for general use and tends to screw up timing
// It uses TP13 (PC10) on headboard 4.0
#define ENABLE_BOARD_UART

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      volatile s32 m_writeTail = 0;
      volatile s32 m_writeHead = 0;
      volatile s32 m_writeLength = 0;

      volatile s32 m_readTail = 0;
      volatile s32 m_readHead = 0;

      volatile bool m_isTransferring = false;

      static void UARTStartTransfer();
 
      // ONCHIP because both buffers are accessed by DMA
      ONCHIP u8 m_bufferWrite[1024 * 8];
      ONCHIP u8 m_bufferRead[1024 * 16];
      
      static void UARTStartTransfer()
      {
        int tail = m_writeTail;
        int length = m_writeHead - tail;
        if (length < 0)
          length = sizeof(m_bufferWrite) - tail;
        if (length > 65535)
          length = 65535;

        DMA_STREAM_TX->NDTR = length;                     // Buffer size
        DMA_STREAM_TX->M0AR = (u32)&m_bufferWrite[tail];  // Buffer address

        m_writeLength = length;
        m_isTransferring = true;

        DMA_STREAM_TX->CR |= DMA_SxCR_EN; // Enable DMA
      }

      int UARTGetFreeSpace()
      {
        int tail = m_writeTail;
        int head = m_writeHead;
        if (m_writeHead < tail)
          return tail - head;
        else
          return sizeof(m_bufferWrite) - (head - tail);
      }

      int UARTGetWriteBufferSize()
      {
        return sizeof(m_bufferWrite);
      }

      void UARTInit()
      {
        // Supporting 4.0
        GPIO_PIN_SOURCE(TX, GPIOB, 6);
        GPIO_PIN_SOURCE(RX, GPIOB, 7);

        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_GPIO, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_DMA, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_UART, ENABLE);

        // Configure the pins for UART in AF mode
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = PIN_TX;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_TX, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_InitStructure.GPIO_Pin = PIN_RX;
        GPIO_Init(GPIO_RX, &GPIO_InitStructure);

        GPIO_PinAFConfig(GPIO_TX, SOURCE_TX, GPIO_AF);
        GPIO_PinAFConfig(GPIO_RX, SOURCE_RX, GPIO_AF);

        // Configure the UART for the appropriate baudrate
        USART_InitTypeDef USART_InitStructure;
        USART_Cmd(UART, DISABLE);
        USART_OverSampling8Cmd(UART, ENABLE);
        USART_InitStructure.USART_BaudRate = BAUDRATE;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_2;
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
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;  // Don't want this to be a very high priority
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        m_writeHead = m_writeTail = m_readTail = 0;
        
#ifdef ENABLE_BOARD_UART
        #define BUART USART3        
        #define BBAUDRATE 3000000
        
        GPIO_PIN_SOURCE(BTX, GPIOC, 10);
        
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        
        // Configure the pins for UART in AF mode
        GPIO_InitStructure.GPIO_Pin = PIN_BTX;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  // XXX - why not OD to interface with 3V3 PropPlug?
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_BTX, &GPIO_InitStructure);
        GPIO_PinAFConfig(GPIO_BTX, SOURCE_BTX, GPIO_AF_USART3);
        
        // Configure the UART for the appropriate baudrate
        USART_Cmd(BUART, DISABLE);
        USART_OverSampling8Cmd(BUART, ENABLE);
        USART_InitStructure.USART_BaudRate = BBAUDRATE;
        USART_Init(BUART, &USART_InitStructure);
        USART_Cmd(BUART, ENABLE);
        
        BoardPrintf("Board debug enabled\n");
#endif
      }

      // Add one char to the buffer, wrapping around
      static void BufPutChar(u8 c)
      {
        m_bufferWrite[m_writeHead] = c;
        m_writeHead++;
        if (m_writeHead >= sizeof(m_bufferWrite))
        {
          m_writeHead = 0;
        }
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
        BufPutChar(c);

        // Enable DMA if it's not already running
        if (!m_isTransferring)
        {
          UARTStartTransfer();
        }

        __enable_irq();

        return c;
      }

      bool UARTPutPacket(const u8* buffer, const u32 length, const u8 socket)
      {
        bool result = false;
        __disable_irq();
        int bytesLeft = UARTGetFreeSpace();

        // Leave one guard byte + header
        if (bytesLeft > (length + 1 + 8))
        {
          result = true;

          // Write header first
          BufPutChar(0xBE);
          BufPutChar(0xEF);
          BufPutChar(length >>  0);
          BufPutChar(length >>  8);
          BufPutChar(0);
          BufPutChar(socket);

          bytesLeft = sizeof(m_bufferWrite) - m_writeHead;
          if (length <= bytesLeft)
          {
            memcpy(&m_bufferWrite[m_writeHead], buffer, length);
            m_writeHead += length;

            if (m_writeHead == sizeof(m_bufferWrite))
            {
              m_writeHead = 0;
            }
          } else {
            // Copy to the end of the buffer, then wrap around for the rest
            int lengthFirst = bytesLeft;
            memcpy(&m_bufferWrite[m_writeHead], buffer, lengthFirst);

            bytesLeft = length - lengthFirst;
            memcpy(m_bufferWrite, &buffer[lengthFirst], bytesLeft);

            m_writeHead = bytesLeft;
          }

          // Enable DMA if it's not already running
          if (!m_isTransferring)
          {
            UARTStartTransfer();
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

      s32 UARTGetChar()
      {
        // Make sure there's data in the FIFO
        // NDTR counts down...
        if (DMA_STREAM_RX->NDTR != sizeof(m_bufferRead) - m_readTail)
        {
          u8 value = m_bufferRead[m_readTail];
          m_readTail = (m_readTail + 1) % sizeof(m_bufferRead);
          return value;
        }
        else
        {
          // No data, so return with an error
          return -1;
        }
      }

      void BoardPrintf(const char *format, ...)
      {
#ifdef ENABLE_BOARD_UART     
        // Build the printf into a local buffer and zero-terminate it
        char buffer[256];
        va_list argptr;
        va_start(argptr, format);
        vsnprintf(buffer, sizeof(buffer)-1, format, argptr);
        va_end(argptr);
        buffer[sizeof(buffer)-1] = '\0';
        
        // Send it out the UART, blocking as needed
        char* p = buffer;
        while (*p)
        {
          BUART->DR = *p++;
          while (!(BUART->SR & USART_FLAG_TXE))
            ;
        }
#endif   
      }        
    }
  }
}

extern "C" {
  void board_printf(const char *format, ...)
  {
#ifdef ENABLE_BOARD_UART  
    using namespace Anki::Cozmo::HAL;    
    static int stamping = 0;
    
    if (!stamping)
    {
      stamping = 1;
      board_printf(" %3dms: ", (GetMicroCounter() / 1000) % 1000);
      stamping = 0;
    }
    
    // Build the printf into a local buffer and zero-terminate it
    char buffer[256];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(buffer, sizeof(buffer)-1, format, argptr);
    va_end(argptr);
    buffer[sizeof(buffer)-1] = '\0';
    
    // Send it out the UART, blocking as needed
    char* p = buffer;
    while (*p)
    {
      BUART->DR = *p++;
      while (!(BUART->SR & USART_FLAG_TXE))
        ;
    }
#endif
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
{
  // Used for UART transfer-complete
  void DMA2_Stream7_IRQHandler()
  {
    using namespace Anki::Cozmo::HAL;

    // Clear DMA Transfer Complete flag
    DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TCIF7);

    m_writeTail += m_writeLength;
    if (m_writeTail >= sizeof(m_bufferWrite))
      m_writeTail = 0;

    // Check if there's more data to be transferred
    if (m_writeHead != m_writeTail)
    {
      UARTStartTransfer();
    } else {
      m_isTransferring = false;
    }
  }
}
