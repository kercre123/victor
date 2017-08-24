#include <string.h>

#include "common.h"
#include "hardware.h"

#include "encoders.h"
#include "motors.h"
#include "messages.h"

static const int MOTOR_SERVICE_COUNTDOWN = 4;
static const int MAX_ENCODER_FRAMES = 25; // 0.1250s
static const int MAX_POWER = 0x8000;
static const uint16_t MOTOR_PERIOD = 20000; // 20khz
static const int16_t MOTOR_MAX_POWER = SYSTEM_CLOCK / MOTOR_PERIOD;

struct MotorConfig {
  // Pin BRSS
  volatile uint32_t* P_BSRR;
  uint32_t           P_Set;

  // N1 Pin
  volatile uint32_t* N1CC;
  GPIO_TypeDef*      N1_Bank;
  uint32_t           N1_ModeMask;
  uint32_t           N1_ModeAlt;
  uint32_t           N1_ModeOutput;

  // N2 Pin
  volatile uint32_t* N2CC;
  GPIO_TypeDef*      N2_Bank;
  uint32_t           N2_ModeMask;
  uint32_t           N2_ModeAlt;
  uint32_t           N2_ModeOutput;
};

// Current status of the motors
struct MotorStatus {
  uint32_t  position;
  uint32_t  last_time;
  int       power;
  bool      direction;
  bool      configured;
  uint8_t   serviceCountdown;
};

#define CONFIG_N(PIN) \
  (PIN::bank), ~(3 << (PIN::pin * 2)), (MODE_ALTERNATE << (PIN::pin * 2)), (MODE_OUTPUT << (PIN::pin * 2))

static const MotorConfig MOTOR_DEF[MOTOR_COUNT] = {
  {
    &LTP1::bank->BSRR, LTP1::mask,
    &TIM1->CCR2, CONFIG_N(LTN1),
    &TIM1->CCR2, CONFIG_N(LTN2)
  },
  {
    &RTP1::bank->BSRR, RTP1::mask,
    &TIM1->CCR1, CONFIG_N(RTN1),
    &TIM1->CCR1, CONFIG_N(RTN2)
  },
  {
    &LP1::bank->BSRR, LP1::mask,
    &TIM1->CCR3, CONFIG_N(LN1),
    &TIM1->CCR4, CONFIG_N(LN2)
  },
  {
    &HP1::bank->BSRR, HP1::mask,
    &TIM3->CCR2, CONFIG_N(HN1),
    &TIM3->CCR4, CONFIG_N(HN2)
  },
};

static MotorStatus motorStatus[MOTOR_COUNT];
static int16_t motorPower[MOTOR_COUNT];
static int moterServiced;

static void configure_pins() {
  // Reset our ports (This is portable, but ugly, can easily collapse this into 6 writes)
  LP1::reset();
  LN1::reset();
  LN2::reset();
  HP1::reset();
  HN1::reset();
  HN2::reset();
  RTP1::reset();
  RTN1::reset();
  RTN2::reset();
  LTP1::reset();
  LTN1::reset();
  LTN2::reset();

  LP1::mode(MODE_OUTPUT);
  LN1::mode(MODE_OUTPUT);
  LN2::mode(MODE_OUTPUT);

  HP1::mode(MODE_OUTPUT);
  HN1::mode(MODE_OUTPUT);
  HN2::mode(MODE_OUTPUT);

  RTP1::mode(MODE_OUTPUT);
  RTN1::mode(MODE_OUTPUT);
  RTN2::mode(MODE_OUTPUT);

  LTP1::mode(MODE_OUTPUT);
  LTN1::mode(MODE_OUTPUT);
  LTN2::mode(MODE_OUTPUT);

  LN1::alternate(2);
  LN2::alternate(2);
  HN1::alternate(1);
  HN2::alternate(1);
  LTN1::alternate(2);
  LTN2::alternate(2);
  RTN1::alternate(2);
  RTN2::alternate(2);
}

static void Motors::receive(HeadToBody *payload) {
  moterServiced = MOTOR_SERVICE_COUNTDOWN;
  memcpy(motorPower, payload->motorPower, sizeof(motorPower));
}

#include "contacts.h"

static void Motors::transmit(BodyToHead *payload) {
  uint32_t* time_last;
  int32_t* delta_last;

  Encoders::flip(time_last, delta_last);

  // Radio silence = power down motors
  if (moterServiced <= 0) {
    memset(motorPower, 0, sizeof(motorPower));
  } else {
    moterServiced--;
  }

  for (int i = 0; i < MOTOR_COUNT; i++) {
    MotorStatus* state = &motorStatus[i];

    // Calculate motor power
    motorStatus[i].power = (MOTOR_MAX_POWER * (int)motorPower[i]) / MAX_POWER;

    // Clear our delta when we've been working too hard
    if (++state->serviceCountdown > MAX_ENCODER_FRAMES) {
      payload->motor[i].delta = 0;
    }

    // Calculate the encoder values
    if (delta_last[i] != 0) {
      // Update robot positions
      switch (i) {
        case MOTOR_LEFT:
        case MOTOR_RIGHT:
          state->position += state->direction ? delta_last[i] : -delta_last[i];
          break ;
        default:
          state->position += delta_last[i];
          break ;
      }

      payload->motor[i].position = state->position;

      // Copy over tick values
      payload->motor[i].delta = state->direction ? delta_last[i] : -delta_last[i];
      payload->motor[i].time = state->last_time - time_last[i];

      // We will survives
      state->last_time = time_last[i];
      state->serviceCountdown = 0;
    }
  }
}

static void configure_timer(TIM_TypeDef* timer) {
  // PWM mode 2, edge aligned with fast output enabled (prebuffered)
  // Complementary outputs are inverted
  timer->CCMR2 =
  timer->CCMR1 =
    TIM_CCMR1_OC1PE | TIM_CCMR1_OC1FE | (TIM_CCMR1_OC1M_0 * 6) |
    TIM_CCMR1_OC2PE | TIM_CCMR1_OC2FE | (TIM_CCMR1_OC2M_0 * 6);
  timer->CCER =
    TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E |
    TIM_CCER_CC1NE | TIM_CCER_CC2NE | TIM_CCER_CC3NE |
    TIM_CCER_CC1NP | TIM_CCER_CC2NP | TIM_CCER_CC3NP;

  timer->BDTR = TIM_BDTR_MOE;

  // Clear our comparison registers
  timer->CCR1 =
  timer->CCR2 =
  timer->CCR3 =
  timer->CCR4 = 0;

  // Configure (PWM edge-aligned, count up, 20khz)
  timer->PSC = 0;
  timer->ARR = MOTOR_MAX_POWER - 1;
  timer->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}

void Motors::init() {
  // Setup motor power
  memset(&motorStatus, 0, sizeof(motorStatus));

  configure_timer(TIM1);
  configure_timer(TIM3);
  configure_pins();
}

// This treats 0 power as a 'transitional' state
// This can be optimized so that if in configured and direction has not changed, to reconfigure the pins, otherwise set power

void Motors::tick() {
  for(int i = 0; i < MOTOR_COUNT; i++) {
    MotorConfig* config =  (MotorConfig*) &MOTOR_DEF[i];
    MotorStatus* state = &motorStatus[i];

    bool direction = state->power >= 0;
    bool transition = state->power == 0 || state->direction != direction;

    // This is set when the bus is idle
    if (!state->configured) {
      // Configure our pins
      if (direction) {
        *config->P_BSRR = config->P_Set;
        config->N1_Bank->MODER = (config->N1_Bank->MODER & config->N1_ModeMask) | config->N1_ModeAlt;
        config->N2_Bank->MODER = (config->N2_Bank->MODER & config->N2_ModeMask) | config->N2_ModeOutput;
      } else {
        *config->P_BSRR = config->P_Set << 16;
        config->N2_Bank->MODER = (config->N2_Bank->MODER & config->N2_ModeMask) | config->N2_ModeAlt;
        config->N1_Bank->MODER = (config->N1_Bank->MODER & config->N1_ModeMask) | config->N1_ModeOutput;
      }

      state->direction = direction;
      state->configured = true;
      return ;
    } else if (transition) {
      // Power down the motor for a cycle
      *config->N1CC = 0;
      *config->N2CC = 0;

      state->configured = false;
    } else {
      // Drive our power forward
      if (direction) {
        *config->N1CC = state->power;
      } else {
        *config->N2CC = -state->power;
      }
    }
  }
}
