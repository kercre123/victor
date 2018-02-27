#include <string.h>

#include "common.h"
#include "hardware.h"

#include "encoders.h"
#include "timer.h"

static const int8_t QUAD_DECODE[4][4] = {
  {  0,  1, -1,  0 },
  { -1,  0,  0,  1 },
  {  1,  0,  0, -1 },
  {  0, -1,  1,  0 }
};

static int page;
static uint32_t time[2][MOTOR_COUNT];
static int32_t delta[2][MOTOR_COUNT];

void Encoders::init(void) {
  static const uint32_t EVENT_MASK =
    LENCA::mask | LENCB::mask |
    HENCA::mask | HENCB::mask |
    RTENC::mask | LTENC::mask;

  // Enable power to encoder LEDs
  nVENC_EN::reset();
  nVENC_EN::mode(MODE_OUTPUT);

  // Setup gpio
  LENCA::mode(MODE_INPUT);
  LENCB::mode(MODE_INPUT);
  HENCA::mode(MODE_INPUT);
  HENCB::mode(MODE_INPUT);
  RTENC::mode(MODE_INPUT);
  LTENC::mode(MODE_INPUT);

  // When this pin changes, do some shizz
  SYSCFG->EXTICR[0] =
    SYSCFG_EXTICR1_EXTI0_PA |
    SYSCFG_EXTICR1_EXTI1_PA |
    SYSCFG_EXTICR1_EXTI2_PB |
    SYSCFG_EXTICR1_EXTI3_PB;

  SYSCFG->EXTICR[3] =
    SYSCFG_EXTICR4_EXTI14_PC |
    SYSCFG_EXTICR4_EXTI15_PC;

  EXTI->FTSR |= EVENT_MASK;
  EXTI->RTSR |= EVENT_MASK;
  EXTI->IMR  |= EVENT_MASK;

  NVIC_SetPriority(EXTI0_1_IRQn, PRIORITY_ENCODERS);
  NVIC_SetPriority(EXTI2_3_IRQn, PRIORITY_ENCODERS);
  NVIC_SetPriority(EXTI4_15_IRQn, PRIORITY_ENCODERS);
  NVIC_EnableIRQ(EXTI0_1_IRQn);
  NVIC_EnableIRQ(EXTI2_3_IRQn);
  NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void Encoders::stop() {
  NVIC_DisableIRQ(EXTI0_1_IRQn);
  NVIC_DisableIRQ(EXTI2_3_IRQn);
  NVIC_DisableIRQ(EXTI4_15_IRQn);

  nVENC_EN::set();
}

void Encoders::flip(uint32_t* &time_last, int32_t* &delta_last) {
  const int next_page = page ^ 1;

  time_last = time[page];
  delta_last = delta[page];
  memset(&time[next_page], 0, sizeof(time[0]));
  memset(&delta[next_page], 0, sizeof(delta[0]));
  page = next_page;
}

// Head encoder
extern "C" void EXTI0_1_IRQHandler(void) {
  static uint32_t prev;
  const uint32_t now = (HENCA::bank->IDR >> HENCA::pin) & 0x3;

  time[page][MOTOR_HEAD] = Timer::getTime();
  delta[page][MOTOR_HEAD] += QUAD_DECODE[prev][now];
  prev = now;

  // Clear our interrupt
  EXTI->PR = HENCA::mask | HENCB::mask;
}

// Lift encoder
extern "C" void EXTI2_3_IRQHandler(void) {
  static uint32_t prev;
  const uint32_t now = (LENCA::bank->IDR >> LENCA::pin) & 0x3;

  time[page][MOTOR_LIFT] = Timer::getTime();
  delta[page][MOTOR_LIFT] += QUAD_DECODE[prev][now];
  prev = now;

  // Clear our interrupt
  EXTI->PR = LENCA::mask | LENCB::mask;
}

// Tread encoders
extern "C" void EXTI4_15_IRQHandler(void) {
  const uint32_t now = Timer::getTime();
  const uint32_t pins = RTENC::bank->IDR & (RTENC::mask | LTENC::mask);

  if (EXTI->PR & RTENC::mask) {
    delta[page][MOTOR_RIGHT]++;
    time[page][MOTOR_RIGHT] = now;
  }

  if (EXTI->PR & LTENC::mask) {
    delta[page][MOTOR_LEFT]++;
    time[page][MOTOR_LEFT] = now;
  }

  // Clear our interrupt
  EXTI->PR = RTENC::mask | LTENC::mask;
}
