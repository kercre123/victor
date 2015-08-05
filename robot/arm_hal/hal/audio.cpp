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

static int16_t MuLawDecompressTable[] =
{
       0,     16,     32,     48,     64,     80,     96,    112,
     128,    144,    160,    176,    192,    208,    224,    240,
     256,    272,    288,    304,    320,    336,    352,    368,
     384,    400,    416,    432,    448,    464,    480,    496,
     512,    544,    576,    608,    640,    672,    704,    736,
     768,    800,    832,    864,    896,    928,    960,    992,
    1024,   1088,   1152,   1216,   1280,   1344,   1408,   1472,
    1536,   1600,   1664,   1728,   1792,   1856,   1920,   1984,
    2048,   2176,   2304,   2432,   2560,   2688,   2816,   2944,
    3072,   3200,   3328,   3456,   3584,   3712,   3840,   3968,
    4096,   4352,   4608,   4864,   5120,   5376,   5632,   5888,
    6144,   6400,   6656,   6912,   7168,   7424,   7680,   7936,
    8192,   8704,   9216,   9728,  10240,  10752,  11264,  11776,
   12288,  12800,  13312,  13824,  14336,  14848,  15360,  15872,
   16384,  17408,  18432,  19456,  20480,  21504,  22528,  23552,
   24576,  25600,  26624,  27648,  28672,  29696,  30720,  31744,
      -1,    -17,    -33,    -49,    -65,    -81,    -97,   -113,
    -129,   -145,   -161,   -177,   -193,   -209,   -225,   -241,
    -257,   -273,   -289,   -305,   -321,   -337,   -353,   -369,
    -385,   -401,   -417,   -433,   -449,   -465,   -481,   -497,
    -513,   -545,   -577,   -609,   -641,   -673,   -705,   -737,
    -769,   -801,   -833,   -865,   -897,   -929,   -961,   -993,
   -1025,  -1089,  -1153,  -1217,  -1281,  -1345,  -1409,  -1473,
   -1537,  -1601,  -1665,  -1729,  -1793,  -1857,  -1921,  -1985,
   -2049,  -2177,  -2305,  -2433,  -2561,  -2689,  -2817,  -2945,
   -3073,  -3201,  -3329,  -3457,  -3585,  -3713,  -3841,  -3969,
   -4097,  -4353,  -4609,  -4865,  -5121,  -5377,  -5633,  -5889,
   -6145,  -6401,  -6657,  -6913,  -7169,  -7425,  -7681,  -7937,
   -8193,  -8705,  -9217,  -9729, -10241, -10753, -11265, -11777,
  -12289, -12801, -13313, -13825, -14337, -14849, -15361, -15873,
  -16385, -17409, -18433, -19457, -20481, -21505, -22529, -23553,
  -24577, -25601, -26625, -27649, -28673, -29697, -30721, -31745,
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

      typedef int16_t AudioSample;

      const int InputFreq       = 24000;
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
        int16_t sample = 0;
        int error = 0;
        
        // Previous samples for gaussian filtering
        for (int rem = BufferLength; rem > 0; rem--) {
          error += InputFreq;

          if (error >= SampleRate) {
            sample = MuLawDecompressTable[*(input++)];
            error -= SampleRate;
          }

          *(output++) = sample;
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
