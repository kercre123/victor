#include <string.h>

#include "common.h"
#include "hardware.h"
#include "timer.h"

#include "mics.h"
#include "mic_tables.h"

#include "messages.h"

extern "C" void start_mic_spi(int16_t a, int16_t b, void* tim);

static const int WORDS_PER_SAMPLE = (AUDIO_DECIMATION * 2) / 8;
static const int SAMPLES_PER_IRQ = 20;
static const int IRQS_PER_FRAME = AUDIO_SAMPLES_PER_FRAME / SAMPLES_PER_IRQ;

static int16_t audio_data[2][AUDIO_SAMPLES_PER_FRAME * 4];
__align(2) static uint8_t pdm_data[2][2][WORDS_PER_SAMPLE * SAMPLES_PER_IRQ];
static int sample_index;

static int16_t MIC_SPI_CR1 = 0
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
  MIC1_MISO::alternate(0);
  MIC1_MISO::speed(SPEED_HIGH);
  MIC1_MISO::mode(MODE_ALTERNATE);

  MIC2_MISO::alternate(0);
  MIC2_MISO::speed(SPEED_HIGH);
  MIC2_MISO::mode(MODE_ALTERNATE);

  // Setup our output clock to TIM15
  MIC_LR::alternate(1);
  MIC_LR::speed(SPEED_HIGH);
  MIC_LR::mode(MODE_ALTERNATE);

  // Set and output clock for the SPI perf so reads work (not connected)
  MIC1_SCK::alternate(0);
  MIC1_SCK::speed(SPEED_HIGH);
  MIC1_SCK::mode(MODE_ALTERNATE);

  MIC2_SCK::alternate(0);
  MIC2_SCK::speed(SPEED_HIGH);
  MIC2_SCK::mode(MODE_ALTERNATE);

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

  start_mic_spi(TIM_CR1_CEN, MIC_SPI_CR1, (void*)&TIM15->CR1);
}

void Mics::errorCode(uint16_t* data) {
  data[0] = *(uint16_t*)&pdm_data[0][0];
  data[1] = *(uint16_t*)&pdm_data[1][0];
}

void Mics::transmit(int16_t* payload) {
  memcpy(payload, audio_data[sample_index < IRQS_PER_FRAME ? 1 : 0], sizeof(audio_data[0]));
}

#define STAGE2(t_a) \
  ptr = &DECIMATION_TABLE[t_a][*(samples++)]; \
  accumulator[0] += *ptr; \
  ptr += 0xC00; accumulator[1] += *ptr; \
  ptr = &DECIMATION_TABLE[t_a][*(samples++)]; \
  accumulator[2] += *ptr; \
  ptr += 0xC00; accumulator[3] += *ptr

#define STAGE3(t_a) \
  ptr = &DECIMATION_TABLE[t_a][*(samples++)]; \
  acc_l += *ptr; \
  ptr += 0xC00; accumulator[0] += *ptr;  \
  ptr += 0xC00; accumulator[1] += *ptr; \
  ptr = &DECIMATION_TABLE[t_a][*(samples++)]; \
  acc_r += *ptr; \
  ptr += 0xC00; accumulator[2] += *ptr; \
  ptr += 0xC00; accumulator[3] += *ptr

#define STAGE3_A() \
  ptr = &DECIMATION_TABLE[31][*(samples++)]; \
  output[0] = (int16_t)((accumulator[1] + *ptr) >> 16); \
  ptr -= 0xC00; accumulator[1] = accumulator[0] + *ptr; \
  ptr -= 0xC00; accumulator[0] = acc_l + *ptr; \
  ptr = &DECIMATION_TABLE[31][*(samples++)]; \
  output[1] = (int16_t)((accumulator[3] + *ptr) >> 16); \
  ptr -= 0xC00; accumulator[3] = accumulator[2] + *ptr; \
  ptr -= 0xC00; accumulator[2] = acc_r + *ptr;

static void decimate(const uint8_t* input, int32_t* accumulator,  int16_t* output) {
  static uint8_t deinter[sizeof(uint16_t) * 12 * SAMPLES_PER_IRQ];
  uint16_t* target = (uint16_t*)&deinter;

  // Deinterlace the channel data
  for (int i = 0; i < 12 * SAMPLES_PER_IRQ; i++) {
    uint8_t b_a = *(input++);
    uint8_t b_b = *(input++);

    *(target++) = DEINTERLACE_TABLE[0][b_a] | DEINTERLACE_TABLE[1][b_b];
  }

  uint8_t* samples = deinter;
  for (int i = 0; i < SAMPLES_PER_IRQ; i++) {
    const int32_t *ptr;
    int32_t acc_l = 0;
    int32_t acc_r = 0;

    STAGE2( 8);
    STAGE2( 9);
    STAGE2(10);
    STAGE2(11);
    STAGE3( 0);
    STAGE3( 1);
    STAGE3( 2);
    STAGE3( 3);
    STAGE3( 4);
    STAGE3( 5);
    STAGE3( 6);
    STAGE3_A();

    output += 4;
  }
}

extern "C" void DMA1_Channel2_3_IRQHandler(void) {
  static int16_t *index = audio_data[0];
  uint32_t isr = DMA1->ISR;
  DMA1->IFCR = DMA_ISR_GIF2;

  static int32_t accumulator[2][4]; // 2 data lines, 2 channels, 2 accumulators

  // Note: if this falls behind, it will drop a bunch of samples
  if (isr & DMA_ISR_HTIF2) {
    decimate(pdm_data[0][0], accumulator[0], &index[0]);
    decimate(pdm_data[1][0], accumulator[1], &index[2]);
    index += SAMPLES_PER_IRQ * 4;
    sample_index++;
  } else {
    decimate(pdm_data[0][1], accumulator[0], &index[0]);
    decimate(pdm_data[1][1], accumulator[1], &index[2]);
    index += SAMPLES_PER_IRQ * 4;
    sample_index++;
  }

  // Circular buffer increment
  if (sample_index >= IRQS_PER_FRAME * 2) {
    index = audio_data[0];
    sample_index = 0;
  }
}
