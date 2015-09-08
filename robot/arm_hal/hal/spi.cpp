// XXX: "spi.cpp" was a bad name for "spineport.cpp"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"
#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/spineData.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // These are the DMA copies - they will have tearing and corruption    
      // m_DMAtoHead is double sized so it can carry the echoed body data (to check for xmit error?)
      // These are ONCHIP because CCM can't handle DMA
      //typedef struct { GlobalDataToBody body; GlobalDataToHead head; } DMABodyPlusHead;
      volatile static ONCHIP /*DMABodyPlusHead*/ GlobalDataToHead m_DMAtoHead;
      volatile static ONCHIP GlobalDataToBody m_DMAtoBody;
      
      // These are "live" copies - they won't tear or corrupt, but they will change underneath you
      volatile ONCHIP GlobalDataToHead g_dataToHead;
      volatile ONCHIP GlobalDataToBody g_dataToBody;
      
      // True when main exec should run, false when it is ready to run
      volatile u8 g_runMainExec = 0;
      
      #define BAUDRATE 350000

      #define RCC_GPIO        RCC_AHB1Periph_GPIOD
      #define RCC_DMA         RCC_AHB1Periph_DMA1
      #define RCC_UART        RCC_APB1Periph_USART2
      #define GPIO_AF         GPIO_AF_USART2
      #define UART            USART2

      #define DMA_STREAM_RX   DMA1_Stream5
      #define DMA_CHANNEL_RX  DMA_Channel_4
      #define DMA_FLAG_RX     DMA_FLAG_TCIF5    // Stream 5
      #define DMA_IRQ_RX      DMA1_Stream5_IRQn
      #define DMA_HANDLER_RX  DMA1_Stream5_IRQHandler

      #define DMA_STREAM_TX   DMA1_Stream6
      #define DMA_CHANNEL_TX  DMA_Channel_4
      #define DMA_FLAG_TX     DMA_FLAG_TCIF6    // Stream 6
      #define DMA_IRQ_TX      DMA1_Stream6_IRQn
      #define DMA_HANDLER_TX  DMA1_Stream6_IRQHandler
      
      GPIO_PIN_SOURCE(TRX, GPIOD, 5);

      void PrintCrap()
      {
        printf("\nTX: %d  RX: %d\n", DMA_STREAM_TX->NDTR, DMA_STREAM_RX->NDTR);
        for (int i = 0; i < sizeof(g_dataToHead); i++)
        {
          printf("%02x", ((u8*)(&g_dataToHead))[i]);
          if ((i & 63) == 63)
            printf("\n");
        }
      }

      void SPIInit()
      {
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_GPIO, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_DMA, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_UART, ENABLE);
        
        // Configure the pins for UART in AF mode
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = PIN_TRX;  
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;    // Open collector transmit/receive
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_TRX, &GPIO_InitStructure);
        
        GPIO_PinAFConfig(GPIO_TRX, SOURCE_TRX, GPIO_AF);
        
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
        //USART_HalfDuplexCmd(UART, ENABLE);
        USART_Cmd(UART, ENABLE);
        
        // Configure DMA for receiving
        DMA_DeInit(DMA_STREAM_RX);
        
        DMA_InitTypeDef DMA_InitStructure;
        DMA_InitStructure.DMA_Channel = DMA_CHANNEL_RX;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART->DR;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)&m_DMAtoHead;
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
        DMA_InitStructure.DMA_BufferSize = sizeof(m_DMAtoHead);
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
        DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_Init(DMA_STREAM_RX, &DMA_InitStructure);
        
        // Enable DMA
        USART_DMACmd(UART, USART_DMAReq_Rx, ENABLE);
        //DMA_Cmd(DMA_STREAM_RX, ENABLE);
                        
        // Enable interrupt on DMA transfer complete for RX.
        DMA_ITConfig(DMA_STREAM_RX, DMA_IT_TC, ENABLE);
        
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = DMA_IRQ_RX;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  // Don't want this to be a very high priority
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);   

        // Set up first "hello" message to body
        g_dataToBody.common.source = SPI_SOURCE_HEAD;
        g_dataToBody.common.SYNC[0] = 0xfa;
        g_dataToBody.common.SYNC[1] = 0xf3;
        g_dataToBody.common.SYNC[2] = 0x20;
        memcpy((void*)&m_DMAtoBody, (void*)&g_dataToBody, sizeof(m_DMAtoBody));
        
        // Configure DMA For transmitting
        DMA_DeInit(DMA_STREAM_TX);        
        DMA_InitStructure.DMA_Channel = DMA_CHANNEL_TX;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART->DR;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)&m_DMAtoBody;
        DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
        DMA_InitStructure.DMA_BufferSize = sizeof(m_DMAtoBody);
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
        
        // Enable interrupt on DMA transfer complete for RX.
        DMA_ITConfig(DMA_STREAM_TX, DMA_IT_TC, ENABLE);
        
        NVIC_InitStructure.NVIC_IRQChannel = DMA_IRQ_TX;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  // Don't want this to be a very high priority
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure); 
        
        // Enable first UART transmission - this will tell the body we are awake and ready                
        USART_DMACmd(UART, USART_DMAReq_Tx, ENABLE);
        DMA_Cmd(DMA_STREAM_TX, ENABLE);
      }                      
    }
  }
}

// DMA transmit complete IRQ handler
// This is where we turn around the channel to receive more data
extern "C"
void DMA_HANDLER_TX(void)
{
  using namespace Anki::Cozmo::HAL;
    
  // Clear DMA Transfer Complete flag
  DMA_ClearFlag(DMA_STREAM_TX, DMA_FLAG_TX);
  USART_HalfDuplexCmd(UART, ENABLE);
  //UARTPutChar('T');
  
  // Wait 40uS before listening again
  MicroWait(200);
  
  // Clear anything in the USART
  while (UART->SR & USART_FLAG_RXNE)
    volatile int x = UART->DR;
  
  // Turn around the connection and receive reply
  DMA_STREAM_RX->NDTR = sizeof(m_DMAtoHead);// Buffer size
  DMA_STREAM_RX->M0AR = (u32)&m_DMAtoHead;  // Buffer address
  DMA_STREAM_RX->CR |= DMA_SxCR_EN; // Enable DMA
}

void * _memcpy ( void * destination, const void * source, size_t num ) {
  return memcpy(destination, source, num);
}

// DMA receive Completed IRQ Handler
// This is where we synchronize MainExecution with the body board.
extern "C"
void DMA_HANDLER_RX(void)
{
  using namespace Anki::Cozmo::HAL;
    
  // Clear DMA Transfer Complete flag
  DMA_ClearFlag(DMA_STREAM_RX, DMA_FLAG_RX);
  USART_HalfDuplexCmd(UART, DISABLE);
  //PrintCrap();
  //UARTPutChar('R');
  
  // If main execution isn't already running, copy live buffers to/from the DMA copies
  if (!g_runMainExec)
  {
    // XXX: There is a timing error causing an off-by-one, but I have to get into Kevin's hands
    for (int i = 0; i < 2; i++) {
      GlobalDataToHead* dataToHead = (GlobalDataToHead*)(((u8*)&m_DMAtoHead)+i);
      if (dataToHead->common.common == SPI_SOURCE_BODY) {
        //memcpy((void*)&g_dataToHead, dataToHead, sizeof(g_dataToHead) - i);
        _memcpy((void*)&g_dataToHead, ((u8*)&m_DMAtoHead) + i, sizeof(g_dataToHead) - i);
        break ;
      }
    }
       
    memcpy((void*)&m_DMAtoBody, (void*)&g_dataToBody, sizeof(m_DMAtoBody));
  }
    
  // Wait 80uS before replying
  MicroWait(80);
  
  // Turn around the connection and transmit reply
  DMA_STREAM_TX->NDTR = sizeof(m_DMAtoBody);// Buffer size
  DMA_STREAM_TX->M0AR = (u32)&m_DMAtoBody;  // Buffer address
  DMA_STREAM_TX->CR |= DMA_SxCR_EN; // Enable DMA
  
#if 0
  volatile u8* d = (volatile u8*)&g_dataToHead;
  for (int i = 0; i < 0x10; i++)
  {
    UARTPutHex(d[i]);
    UARTPutChar(' ');
  }
  UARTPutChar('\n');
#endif
  
  const u8 MAX_FAILED_TRANSFER_COUNT = 10;
  static u8 s_failedTransferCount = 0;
  // Verify the source is the body
  if (g_dataToHead.common.source != SPI_SOURCE_BODY)
  {
    if (++s_failedTransferCount > MAX_FAILED_TRANSFER_COUNT)
    {
      //NVIC_SystemReset();
    }
  }
  
  s_failedTransferCount = 0;  // XXX: We need to do something about sync loss here
  
	// Hack to allow timing events longer than about 50ms
	GetMicroCounter();
	
  // Flag start of MainExecution
  g_runMainExec = 1;
}
