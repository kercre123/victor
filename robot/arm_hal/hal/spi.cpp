#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"
#include "lib/stm32f4xx.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      volatile GlobalData m_dataBodyToHead;
      volatile GlobalData m_dataHeadToBody;
      
      static void ConfigurePins()
      {
        GPIO_PIN_SOURCE(SPI_SCK, GPIOG, 13);
        GPIO_PIN_SOURCE(SPI_MISO, GPIOG, 12);
        GPIO_PIN_SOURCE(SPI_MOSI, GPIOG, 14);
        
        // Configure the pins for SPI in AF mode
        GPIO_PinAFConfig(GPIO_SPI_SCK, SOURCE_SPI_SCK, GPIO_AF_SPI6);
        GPIO_PinAFConfig(GPIO_SPI_MISO, SOURCE_SPI_MISO, GPIO_AF_SPI6);
        GPIO_PinAFConfig(GPIO_SPI_MOSI, SOURCE_SPI_MOSI, GPIO_AF_SPI6);
        
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = PIN_SPI_SCK | PIN_SPI_MISO | PIN_SPI_MOSI;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
        GPIO_Init(GPIO_SPI_SCK, &GPIO_InitStructure);  // GPIOG
        
        // Initialize SPI in slave mode
        SPI_I2S_DeInit(SPI6);
        SPI_InitTypeDef SPI_InitStructure;
        SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
        SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
        SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
        SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
        SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
        SPI_InitStructure.SPI_CRCPolynomial = 7;
        SPI_Init(SPI6, &SPI_InitStructure);
        
        for (int i = 0; i < 64; i++)
          m_dataHeadToBody.padding[i] = i;
        
        m_dataHeadToBody.padding[0] = 'H';
      }
      
      static void ConfigureDMA()
      {
        // Initialize DMA for SPI
        // SPI6_TX is DMA2_Stream5 Channel 1
        // SPI6_RX is DMA2_Stream6 Channel 1
        DMA_DeInit(DMA2_Stream5);
        DMA_DeInit(DMA2_Stream6);
        
        DMA_InitTypeDef DMA_InitStructure;
        DMA_InitStructure.DMA_BufferSize = sizeof(m_dataBodyToHead);
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(SPI6->DR);
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_Priority = DMA_Priority_High;
        // Configure TX DMA
        DMA_InitStructure.DMA_Channel = DMA_Channel_1;
        DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)&m_dataHeadToBody;
        DMA_Init(DMA2_Stream5, &DMA_InitStructure);
        // Configure RX DMA
        DMA_InitStructure.DMA_Channel = DMA_Channel_1;
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)&m_dataBodyToHead;
        DMA_Init(DMA2_Stream6, &DMA_InitStructure);
        
        // Enable DMA
        DMA_Cmd(DMA2_Stream5, ENABLE);
        DMA_Cmd(DMA2_Stream6, ENABLE);
        
        // Enable DMA SPI requests
        SPI_I2S_DMACmd(SPI6, SPI_I2S_DMAReq_Tx, ENABLE);
        SPI_I2S_DMACmd(SPI6, SPI_I2S_DMAReq_Rx, ENABLE);
      }
      
      static void ConfigureIRQ()
      {
        // Enable interrupts on SPI RX complete
        DMA_ITConfig(DMA2_Stream6, DMA_IT_TC, ENABLE);
        
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream6_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
      }
      
      void SPIInit()
      {
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI6, ENABLE);
        
        ConfigurePins();
        ConfigureDMA();
        ConfigureIRQ();
        
        SPI_Cmd(SPI6, ENABLE);
      }
    }
  }
}

// DMA Completed IRQ Handler
// This is where we synchronize MainExecution with the body board.
extern "C"
void DMA2_Stream6_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Clear DMA Transfer Complete flags
  DMA_ClearFlag(DMA2_Stream5, DMA_FLAG_TCIF5);
  DMA_ClearFlag(DMA2_Stream6, DMA_FLAG_TCIF6);
  
  // Verify the magic identifier byte from the body board
  if (m_dataBodyToHead.padding[0] != 'B')
  {
    NVIC_SystemReset();
  }
  
  // Run MainExecution
  //Anki::Cozmo::Robot::step_MainExecution();
}
