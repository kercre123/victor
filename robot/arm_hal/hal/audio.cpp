#include <string.h>
#include <math.h>

#include "lib/stm32f4xx.h"
#include "lib/stm32f4xx_gpio.h"
#include "lib/stm32f4xx_rcc.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "hal/portable.h"

#define AUDIO_SPI         SPI2
#define AUDIO_DMA_CHANNEL DMA_Channel_0
#define AUDIO_DMA_STREAM  DMA1_Stream4
#define AUDIO_DMA_IRQ     DMA1_Stream4_IRQn

static const int8_t ima_index_table[] = {
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
}; 
static const uint16_t ima_step_table[] = {
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 
  19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 
  50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 
  130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
  337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
  876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 
  2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
  5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

namespace Anki
{
  namespace Cozmo
  {          
    namespace HAL
    {
      GPIO_PIN_SOURCE(AUDIO_WS, GPIOB, 12);
      GPIO_PIN_SOURCE(AUDIO_CK, GPIOB, 13);
      GPIO_PIN_SOURCE(AUDIO_SD, GPIOB, 15);

      const int SampleFreq    = 24000;
      const int AudioFrameMS  = 33;
      const int AudioSampleSize = 800;
      const int BufferLength  = SampleFreq * AudioFrameMS / 1000;
      const int BufferMidpoint = BufferLength / 2;
      typedef int32_t AudioSample;

      static ONCHIP AudioSample m_audioBuffer[2][BufferLength];
      static ONCHIP AudioSample m_audioWorking[BufferLength];
      bool m_AudioRendered;              // Have we pre-rendered an audio sample for copy
      volatile bool m_AudioClear;        // Current AudioBuffer is clear
      volatile bool m_AudioSilent;       // DMA has move to silence
      volatile int m_AudioBackBuffer;    // Currently inactive buffer

      void AudioInit(void) {
        // Enable peripheral clock
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

        // Enable GPIO clocks
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);  
        
        // Peripherals alternate function
        GPIO_PinAFConfig(GPIO_AUDIO_CK, SOURCE_AUDIO_CK, GPIO_AF_SPI2);
        GPIO_PinAFConfig(GPIO_AUDIO_WS, SOURCE_AUDIO_WS, GPIO_AF_SPI2);
        GPIO_PinAFConfig(GPIO_AUDIO_SD, SOURCE_AUDIO_SD, GPIO_AF_SPI2);

        // Set SPI alternate function pins       
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = PIN_AUDIO_WS | PIN_AUDIO_CK | PIN_AUDIO_SD;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_AUDIO_CK, &GPIO_InitStructure);  // GPIOB

        // Configure I2S interface
        I2S_InitTypeDef I2S_InitStructure;       
        I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
        I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;
        I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_32b;
        I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
        I2S_InitStructure.I2S_AudioFreq = ((uint32_t)SampleFreq);
        I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
        I2S_Init(AUDIO_SPI, &I2S_InitStructure);
        
        RCC_PLLI2SCmd(DISABLE);
        RCC_I2SCLKConfig(RCC_I2S2CLKSource_PLLI2S);
        RCC_PLLI2SCmd(ENABLE);
        RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY);

        // Setup Audio DMA
        DMA_Cmd(AUDIO_DMA_STREAM, DISABLE);
        DMA_DeInit(AUDIO_DMA_STREAM);
    
        DMA_InitTypeDef DMA_InitStructure;       
        DMA_InitStructure.DMA_Channel = AUDIO_DMA_CHANNEL;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&AUDIO_SPI->DR;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)m_audioBuffer;
        DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
        DMA_InitStructure.DMA_BufferSize = sizeof(m_audioBuffer) / 2;
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
        DMA_InitStructure.DMA_Priority = DMA_Priority_High; // Who knows if this is useful
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_Init(AUDIO_DMA_STREAM, &DMA_InitStructure);

        DMA_DoubleBufferModeConfig(AUDIO_DMA_STREAM, (u32)m_audioBuffer[1], DMA_Memory_0);
        DMA_DoubleBufferModeCmd(AUDIO_DMA_STREAM, ENABLE);

        DMA_Cmd(AUDIO_DMA_STREAM, ENABLE);
        SPI_I2S_DMACmd(AUDIO_SPI, SPI_I2S_DMAReq_Tx, ENABLE);

        // Configure interrupts
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = AUDIO_DMA_IRQ;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        // Enable half-transfer and transfer complete (HT is for gradual backdown)
        DMA_ITConfig(AUDIO_DMA_STREAM, DMA_IT_TC | DMA_IT_HT, ENABLE);
        NVIC_EnableIRQ(AUDIO_DMA_IRQ);

        // Clear our buffers to zero, and flag the system as empty and silent
        memset(m_audioBuffer, 0, sizeof(m_audioBuffer));
        m_AudioBackBuffer = 1;
        m_AudioClear = true;
        m_AudioSilent = true;
        m_AudioRendered = false;

        // Enable the SPI
        I2S_Cmd(AUDIO_SPI, ENABLE);
      }
      
      void AudioPlayFrame(s16 predictor, u8* frame) {
        // We are not ready to render another audio frame yet
        if (m_AudioRendered) { return ; }

        static int blocks = 100;
        
        // Silence frame
        if (!frame) {
          memset(m_audioWorking, 0, sizeof(m_audioWorking));
          m_AudioRendered = true;
          return ;
        }

        static float osc = 0;
        
        for (int idx = 0; idx < sizeof(m_audioWorking) / sizeof(AudioSample); idx++) {
          m_audioWorking[idx] = sinf(osc) * 0x80;
          osc += 2 * PI * 440.0f / SampleFreq;
          while (osc > 2 * PI) osc -= 2 * PI;
        }

        /*
        memset(m_audioWorking, 0, sizeof(m_audioWorking));

        int16_t *output = m_audioWorking;
        int step_index = 0;
        
        *(output++) = predictor;
        
        step_index = step_index + ima_index_table[nibble];
        if (step_index < 0) { step_index = 0; }
        if (step_index >= sizeof(ima_index_table)) { step_index = sizeof(ima_index_table) - 1; }

        predictor = predictor + (2 * nibble + 1) * ima_step_table[step_index] / 8;

        // Sloppily play audio out the head
        for (int i = 0; i < AudioSampleSize; i++) {
          m_audioWorking[i*BufferLength/AudioSampleSize] = *(frame++);
        }
        */

        m_AudioRendered = true;
      }
      
      bool AudioReady() {
        return !m_AudioRendered;
      }
      
      void AudioFill(void) {
        AudioPlayFrame(0, (u8*) -1);
        
        // Audio buffers are not empty yet
        if (!m_AudioClear || !m_AudioRendered) { 
          return ;
        }
       
        // Audio system was not streaming audio, so we should ramp up volume
        if (m_AudioSilent) {
          for (int i = 0; i < BufferMidpoint; i++) {
            m_audioWorking[i] = m_audioWorking[i] * i / BufferMidpoint;
          }
        }

        // We triple buffer to reduce the total time spent in IRQ disable
        __disable_irq();
        memcpy(m_audioBuffer[m_AudioBackBuffer], m_audioWorking, sizeof(m_audioWorking));
        m_AudioClear = false;
        m_AudioSilent = false;
        m_AudioRendered = false;
        __enable_irq();
      }
    }
  }
}

extern "C"
void DMA1_Stream4_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;
 
  if (DMA_GetFlagStatus(AUDIO_DMA_STREAM, DMA_FLAG_TCIF4)) {
    // Buffer completed, zero out buffer and mark system as potentially underflowing
    m_AudioBackBuffer = 1-DMA_GetCurrentMemoryTarget(AUDIO_DMA_STREAM);
    memset(&m_audioBuffer[m_AudioBackBuffer], 0, BufferLength * sizeof(AudioSample));

    m_AudioClear = true;
  } else if (DMA_GetFlagStatus(AUDIO_DMA_STREAM, DMA_FLAG_HTIF4)) {
    // Are we going to underflow?  Ramp down to silence
    if (m_AudioClear) {
      AudioSample* buffer = &m_audioBuffer[m_AudioBackBuffer][BufferMidpoint];
      for (int i = BufferMidpoint; i > 0; i--, buffer++) {
        *buffer = *buffer * i / BufferMidpoint;
      }

      m_AudioSilent = true;
    }
  }

  DMA_ClearFlag(AUDIO_DMA_STREAM, DMA_FLAG_TCIF4);
  DMA_ClearFlag(AUDIO_DMA_STREAM, DMA_FLAG_HTIF4);
}
