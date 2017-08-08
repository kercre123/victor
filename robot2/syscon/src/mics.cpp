#include <string.h>

#include "common.h"
#include "hardware.h"
#include "timer.h"

#include "mics.h"
#include "mic_tables.h"

using namespace Anki::Cozmo::Spine;

static const int WORDS_PER_SAMPLE = (AUDIO_DECIMATION * 2) / 8;
static const int SAMPLES_PER_IRQ = 20;
static const int IRQS_PER_FRAME = AUDIO_SAMPLES_PER_FRAME / SAMPLES_PER_IRQ;

static int16_t audio_data[2][AUDIO_SAMPLES_PER_FRAME * 4];
__align(2) static uint8_t pdm_data[2][2][WORDS_PER_SAMPLE * SAMPLES_PER_IRQ];
static int sample_index;

static uint16_t SPI_CR1 = 0
           | SPI_CR1_MSTR                 // Master
           | SPI_CR1_RXONLY               // Read only
           | SPI_CR1_SSM                  // Software slave
           | SPI_CR1_SSI                  // Slave selected
           | SPI_CR1_LSBFIRST             // LSB First
           | (SPI_CR1_BR_0 * AUDIO_SHIFT) // Prescale
           | SPI_CR1_SPE
           ;

static void configurePerf(SPI_TypeDef* spi, DMA_Channel_TypeDef* ch, int perf) {
  ch->CPAR = (uint32_t) &spi->DR;
  ch->CMAR = (uint32_t) &pdm_data[perf];
  ch->CNDTR = sizeof(pdm_data[0]) / sizeof(uint16_t);
  ch->CCR = 0
          | DMA_CCR_MINC      // Memory is incrementing
          | DMA_CCR_PSIZE_0   // 16-bit mode
          | DMA_CCR_MSIZE_0
          | DMA_CCR_CIRC      // Circular mode
          | DMA_CCR_PL        // Highest Priority
          | DMA_CCR_EN        // Enabled
          ;

  spi->CR2 = 0
           | SPI_CR2_DS            // 16-Bit word
           | SPI_CR2_RXDMAEN       // RX DMA Enable
           ;
}

void Mics::init(void) {
  sample_index = 0;

  // Set our MISO lines to SPI1 and SPI2
  MIC1MISO::alternate(0);
  MIC1MISO::speed(SPEED_HIGH);
  MIC1MISO::mode(MODE_ALTERNATE);

  MIC2MISO::alternate(0);
  MIC2MISO::speed(SPEED_HIGH);
  MIC2MISO::mode(MODE_ALTERNATE);

  // Setup our output clock to TIM15
  MIC1MOSI::alternate(1);
  MIC1MOSI::speed(SPEED_HIGH);
  MIC1MOSI::mode(MODE_ALTERNATE);

  /* TEMP CODE UNTIL REV 2 */
  POWER_EN::reset();
  POWER_EN::mode(MODE_OUTPUT);
  RTN2::alternate(0);
  RTN2::mode(MODE_ALTERNATE);
  /* TEMP CODE UNTIL REV 2 */

  // Start configuring out clock
  TIM15->PSC = 0;
  TIM15->CNT = 0;
  TIM15->ARR = AUDIO_PRESCALE * 2 - 1;
  TIM15->CCR2 = AUDIO_PRESCALE;
  TIM15->CCMR1 = TIM_CCMR1_OC2PE | TIM_CCMR1_OC2FE | (TIM_CCMR1_OC2M_0 * 6);
  TIM15->CCER = TIM_CCER_CC2E;
  TIM15->BDTR = TIM_BDTR_MOE;

  // Configure PERFs (but don't start them)
  configurePerf(SPI1, DMA1_Channel2, 0);
  configurePerf(SPI2, DMA1_Channel4, 1);

  // Enable IRQs on channel 2 only, it will do double duty
  DMA1_Channel2->CCR |= 0
                     | DMA_CCR_HTIE
                     | DMA_CCR_TCIE
                     ;

  NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
  NVIC_SetPriority(DMA1_Channel2_3_IRQn, PRIORITY_MICS);

  // NEED TO SLOP TIMING HERE
  TIM15->CR1 = TIM_CR1_CEN;
  SPI1->CR1 = SPI_CR1;
  SPI2->CR1 = SPI_CR1;
}

void Mics::transmit(int16_t* payload) {
  memcpy(payload, audio_data[sample_index < IRQS_PER_FRAME ? 1 : 0], sizeof(audio_data[0]));
}

#define TABLE(i, sa, sb, ta, tb) \
  values = DECIMATION_TABLE[0][*(i++)]; \
  sa = ta + values[0]; \
  ta = values[1]; \
  sb = tb + values[2]; \
  tb = values[3]; \
  values = DECIMATION_TABLE[1][*(i++)]; \
  sa += values[0]; \
  ta += values[1]; \
  sb += values[2]; \
  tb += values[3]; \
  values = DECIMATION_TABLE[2][*(i++)]; \
  sa = (sa + values[0]) >> 16; \
  ta += values[1]; \
  sb = (sb + values[2]) >> 16; \
  tb += values[3]
#define Q1(i, t1, t2, d1, d2) \
  TABLE(i, sample12[0], sample12[1], dec12[d1], dec12[d2]); \
  sample8[0] = DECIMATION_TAP_##t1 * sample12[0] + dec8[d1]; dec8[d1] = DECIMATION_TAP_##t2 * sample12[0]; \
  sample8[1] = DECIMATION_TAP_##t1 * sample12[1] + dec8[d2]; dec8[d2] = DECIMATION_TAP_##t2 * sample12[1]
#define Q2(i, t1, t2, d1, d2) \
  TABLE(i, sample12[0], sample12[1], dec12[d1], dec12[d2]); \
  sample8[0] += DECIMATION_TAP_##t1 * sample12[0]; dec8[d1] += DECIMATION_TAP_##t2 * sample12[0]; \
  sample8[1] += DECIMATION_TAP_##t1 * sample12[1]; dec8[d2] += DECIMATION_TAP_##t2 * sample12[0]
#define SAMP(i, d1, d2) \
  Q1(i, 7, 0, d1, d2); \
  Q2(i, 6, 1, d1, d2); \
  Q2(i, 5, 2, d1, d2); \
  Q2(i, 4, 3, d1, d2); \
  Q2(i, 3, 4, d1, d2); \
  Q2(i, 2, 5, d1, d2); \
  Q2(i, 1, 6, d1, d2); \
  Q2(i, 0, 7, d1, d2); \
  *(output++) = sample8[0] >> 16; \
  *(output++) = sample8[1] >> 16

static inline void decimate(uint8_t* input1, uint8_t* input2, int16_t*& output) {
  static int32_t dec8[4] = { 0, 0, 0, 0 };
  static int32_t dec12[4] = { 0, 0, 0, 0 };

  int32_t sample8[2];
  int32_t sample12[2];
  const int32_t* values;

  for (int i = 0; i < SAMPLES_PER_IRQ; i++) {
    SAMP(input1, 0, 1);
    SAMP(input2, 2, 3);
  }
}

extern "C" void DMA1_Channel2_3_IRQHandler(void) {
  static int16_t *index = audio_data[0];

  if (DMA1->ISR & DMA_ISR_HTIF2) {
    decimate(pdm_data[0][0], pdm_data[1][0], index);
  }

  else if (DMA1->ISR & DMA_ISR_TCIF2) {
    decimate(pdm_data[0][1], pdm_data[1][1], index);
  }

  // Circular buffer increment
  if (++sample_index >= IRQS_PER_FRAME * 2) {
    index = audio_data[0];
    sample_index = 0;
  }

  // Clear DMA1 interrupts
  DMA1->IFCR = 0x00F0;
}
