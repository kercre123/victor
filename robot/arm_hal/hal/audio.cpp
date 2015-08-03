#include <string.h>
#include <math.h>

#include "lib/stm32f4xx.h"
#include "lib/stm32f4xx_gpio.h"
#include "lib/stm32f4xx_rcc.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "hal/portable.h"

//#include "gauss.h"

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

typedef struct {
  uint8_t index;
  int16_t predictor;
  uint8_t samples[400];
} AudioChunk;

namespace Anki
{
  namespace Cozmo
  {          
    namespace HAL
    {
      GPIO_PIN_SOURCE(AUDIO_WS, GPIOB, 12);
      GPIO_PIN_SOURCE(AUDIO_CK, GPIOB, 13);
      GPIO_PIN_SOURCE(AUDIO_SD, GPIOB, 15);

      typedef int16_t AudioSample;

      const int ADPCMFreq       = 24000;
      const int SampleRate      = 48000;
      const int BufferLength    = 1600;       // 33.3ms at 24khz
      const float PLL_Dilate    = 1.555456f;  // No clue why this is nessessary

      static ONCHIP int32_t m_audioBuffer[2][BufferLength*2];
      static ONCHIP AudioSample m_audioWorking[BufferLength];

      bool m_AudioRendered;              // Have we pre-rendered an audio sample for copy
      volatile bool m_AudioClear;        // Current AudioBuffer is clear
      volatile bool m_AudioSilent;       // DMA has move to silence
      volatile int m_AudioBackBuffer;    // Currently available buffer
      
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
        I2S_InitStructure.I2S_AudioFreq = (uint32_t)(SampleRate*PLL_Dilate);
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
        DMA_InitStructure.DMA_BufferSize = BufferLength * 4;
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
        DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
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
        m_AudioClear = true;
        m_AudioSilent = true;
        m_AudioRendered = false;
        m_AudioBackBuffer = 1;

        // Enable the SPI
        I2S_Cmd(AUDIO_SPI, ENABLE);
      }

      void AudioPlaySilence() {
        // We are not ready to render another audio frame yet
        if (m_AudioRendered) { return ; }

        // Clear audio buffer
        memset(m_audioWorking, 0, sizeof(m_audioWorking));
        m_AudioRendered = true;
      }

      void AudioPlayFrame(Messages::AnimKeyFrame_AudioSample *msg) {
        // We are not ready to render another audio frame yet
        if (m_AudioRendered) { return ; }

        AudioSample *output = m_audioWorking;
        const u8 *input = msg->sample;
        bool toggle = false;
        int error = 0;

        // Previous samples for gaussian filtering
        int valpred = msg->predictor;
        int index = msg->index;

        for (int rem = BufferLength; rem > 0; rem--) {
          error += ADPCMFreq;

          if (error >= SampleRate) {
            int delta = toggle ? (*(input++) >> 4) : (*input & 0xF);
            toggle = !toggle;

            error -= SampleRate;

            int step = ima_step_table[index];
            index += ima_index_table[delta];

            if (index < 0) index = 0;
            else if (index > 88) index = 88;

            bool sign = ((delta & 8) == 8);
            delta = delta & 7;

            int vpdiff = step >> 3;
            if (delta & 4) vpdiff += step;
            if (delta & 2) vpdiff += step >> 1;
            if (delta & 1) vpdiff += step >> 2;

            if (sign) valpred -= vpdiff;
            else valpred += vpdiff;

            if (valpred > 32767) valpred = 32767;
            else if (valpred < -32768) valpred = -32768;
          }

          *(output++) = valpred;
        }

        m_AudioRendered = true;
      }
      
      bool AudioReady() {
        return !m_AudioRendered;
      }
      
      void AudioFill(void) {
        // Audio buffers are not empty yet
        if (!m_AudioClear || !m_AudioRendered) { 
          return ;
        }

        // Audio system was not streaming audio, so we should ramp up volume
        if (m_AudioSilent) {
          const int BufferMidpoint  = BufferLength / 2;
          
          for (int i = 0; i < BufferMidpoint; i++) {
            m_audioWorking[i] = m_audioWorking[i] * i / BufferMidpoint;
          }
        }

        // We triple buffer to reduce the total time spent in IRQ disable
        __disable_irq();
        int32_t *out = &m_audioBuffer[m_AudioBackBuffer][0];
        int16_t *in  = &m_audioWorking[0];
        
        for (int i = 0; i < BufferLength; i++, out+=2, in++) {
          *out = *in;
        }

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
  int audioBuffer = DMA_GetCurrentMemoryTarget(AUDIO_DMA_STREAM);
  
  if (DMA_GetFlagStatus(AUDIO_DMA_STREAM, DMA_FLAG_TCIF4)) {
    // Buffer completed, zero out buffer and mark system as potentially underflowing
    m_AudioBackBuffer = 1 - m_AudioBackBuffer;
    memset(&m_audioBuffer[m_AudioBackBuffer], 0, sizeof(m_audioBuffer[0]));   
    m_AudioClear = true;
  } else if (DMA_GetFlagStatus(AUDIO_DMA_STREAM, DMA_FLAG_HTIF4)) {
    // Are we going to underflow?  Ramp down to silence
    if (m_AudioClear) {
      const int BufferMidpoint  = BufferLength;
      
      int32_t* buffer = &m_audioBuffer[1-m_AudioBackBuffer][BufferMidpoint];
      for (int i = BufferMidpoint; i > 0; i--, buffer++) {
        *buffer = *buffer * i / BufferMidpoint;
      }

      m_AudioSilent = true;
    }
  }

  DMA_ClearFlag(AUDIO_DMA_STREAM, DMA_FLAG_TCIF4);
  DMA_ClearFlag(AUDIO_DMA_STREAM, DMA_FLAG_HTIF4);
}
