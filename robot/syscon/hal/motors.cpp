#include "anki/cozmo/robot/hal.h"
#include "motors.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"

// 16 MHz timer with PWM running at 20kHz
#define TIMER_TICKS_END ((16000000 / 20000) - 1)

namespace
{
  const u32 IRQ_PRIORITY = 1;
  
  struct EncoderData
  {
    u32 lastTick;
    u32 delta;
  };
  
  struct MotorInfo
  {
    const u8 forwardUpPin;
    const u8 backwardDownPin;
    const u8 encoderPins[2];
    volatile u32* const pwm;  // This is a pointer to TIMERx->CC[0 or 1]
    
    s32 unitsPerTick;
    s32 position;
    EncoderData encoderData[2];
  };
  
  const u8 LEFT_WHEEL_FORWARD_PIN = 3;
  const u8 LEFT_WHEEL_BACKWARD_PIN = 7;
  const u8 RIGHT_WHEEL_FORWARD_PIN = 22;
  const u8 RIGHT_WHEEL_BACKWARD_PIN = 23;
  const u8 LIFT_UP_PIN = 25;
  const u8 LIFT_DOWN_PIN = 24;
  const u8 HEAD_UP_PIN = 9;
  const u8 HEAD_DOWN_PIN = 10;
  
  const u8 ENCODER_NONE = 0xFF;
  const u8 ENCODER_1_PIN = 4;
  const u8 ENCODER_2_PIN = 21;
  const u8 ENCODER_3A_PIN = 8;
  const u8 ENCODER_3B_PIN = 19;
  const u8 ENCODER_4A_PIN = 20;
  const u8 ENCODER_4B_PIN = 18;
  
  // TODO: Get real values and do 16.16 fixed point
  
  // Given a gear ratio of 91.7:1 and 125.67mm wheel circumference, we compute
  // the mm per tick as (using 4 magnets):
  const s32 MM_PER_TICK = 1; //125.67 / 182.0 / 4.0;
  
  // Given a gear ratio of 815.4:1 and 4 encoder ticks per revolution, we
  // compute the radians per tick on the lift as:
  const s32 RADIANS_PER_LIFT_TICK = 1; //(0.5 * M_PI) / 815.4;

  // If no encoder activity for 200ms, we may as well be stopped
  const u32 ENCODER_TIMEOUT_US = 200000;

  // Set a max speed and reject all noise above it
  const u32 MAX_SPEED_MM_S = 500;
  const u32 DEBOUNCE_US = (MM_PER_TICK * 1000) / MAX_SPEED_MM_S;
  
  // NOTE: Do NOT re-order the MotorID enum, because this depends on it
  MotorInfo m_motors[] =
  {
    {
      LEFT_WHEEL_FORWARD_PIN,
      LEFT_WHEEL_BACKWARD_PIN,
      ENCODER_1_PIN,
      ENCODER_NONE,
      &NRF_TIMER1->CC[0],
      MM_PER_TICK,
      0,
      0, 0, 0, 0
    },
    {
      RIGHT_WHEEL_FORWARD_PIN,
      RIGHT_WHEEL_BACKWARD_PIN,
      ENCODER_2_PIN,
      ENCODER_NONE,
      &NRF_TIMER1->CC[1],
      MM_PER_TICK,
      0,
      0, 0, 0, 0
    },
    {
      LIFT_UP_PIN,
      LIFT_DOWN_PIN,
      ENCODER_3A_PIN,
      ENCODER_3B_PIN,
      &NRF_TIMER2->CC[0],
      RADIANS_PER_LIFT_TICK,
      0,
      0, 0, 0, 0
    },
    {
      HEAD_UP_PIN,
      HEAD_DOWN_PIN,
      ENCODER_4A_PIN,
      ENCODER_4B_PIN,
      &NRF_TIMER2->CC[1],
      RADIANS_PER_LIFT_TICK,
      0,
      0, 0, 0, 0
    },
  };
  
  const u32 MOTOR_COUNT = sizeof(m_motors) / sizeof(MotorInfo);
  
  u32 m_pinsHigh = 0;
}

static void ConfigureTimer(NRF_TIMER_Type* timer, const u8 taskChannel, const u8 ppiChannel)
{
  // Configure the timer to be in 16-bit timer mode
  timer->MODE = TIMER_MODE_MODE_Timer;
  timer->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
  timer->PRESCALER = 0;
  
  // Clear the timer to 0
  timer->TASKS_CLEAR = 1;
  
  // XXX: 0 and 799 are invalid
  // for 0, use GPIO to disable / cut power to motor (don't waste current)
  // for 799, just clamp to 798
  timer->CC[0] = 0;  // PWM n + 0, where n = taskChannel
  timer->CC[1] = 0;  // PWM n + 1
  timer->CC[3] = TIMER_TICKS_END;    // Trigger on timer wrap-around at 800
  
  // Configure the timer to reset the count when it hits the period defined in compare[3]
  timer->SHORTS = 1 << TIMER_SHORTS_COMPARE3_CLEAR_Pos;
  
  // Configure PPI channels to toggle the output PWM pins on matching timer compare
  // Match compare[0] (PWM n + 0)
  NRF_PPI->CH[ppiChannel + 0].EEP = (u32)&timer->EVENTS_COMPARE[0];
  NRF_PPI->CH[ppiChannel + 0].TEP = (u32)&NRF_GPIOTE->TASKS_OUT[taskChannel + 0];
  
  // Match compare[3] (timer wrap-around)
  NRF_PPI->CH[ppiChannel + 1].EEP = (u32)&timer->EVENTS_COMPARE[3];
  NRF_PPI->CH[ppiChannel + 1].TEP = (u32)&NRF_GPIOTE->TASKS_OUT[taskChannel + 0];
  
  // Match compare[1] (PWM n + 1)
  NRF_PPI->CH[ppiChannel + 2].EEP = (u32)&timer->EVENTS_COMPARE[1];
  NRF_PPI->CH[ppiChannel + 2].TEP = (u32)&NRF_GPIOTE->TASKS_OUT[taskChannel + 1];
  
  // Match compare[3] (timer wrap-around)
  NRF_PPI->CH[ppiChannel + 3].EEP = (u32)&timer->EVENTS_COMPARE[3];
  NRF_PPI->CH[ppiChannel + 3].TEP = (u32)&NRF_GPIOTE->TASKS_OUT[taskChannel + 1];
}

static void ConfigurePinSense(u8 pin, u32 pinState)
{
  u32 mask = 1 << pin;
    
  // Configure initial pin sensing (used for inversion with whack-a-mole)
  if (pinState & mask)
  {
    nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);
    m_pinsHigh |= mask;
  } else {
    nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
  }
}

void MotorsInit()
{
  int i;
  
  // Configure TIMER1 and TIMER2 with the appropriate task and PPI channels
  ConfigureTimer(NRF_TIMER1, 0, 0);
  ConfigureTimer(NRF_TIMER2, 2, 4);
  
  // Enable PPI channels for timer reset
  NRF_PPI->CHEN = (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7);
  
  // Disable PPI channels for PWM
  NRF_PPI->CHENCLR = (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6);
  
  // Start the timers
  NRF_TIMER1->TASKS_START = 1;
  NRF_TIMER2->TASKS_START = 1;
  
  // Clear all GPIOTE interrupts
  NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;
  
  // Clear pending interrupts
  NVIC_ClearPendingIRQ(GPIOTE_IRQn);
  NVIC_SetPriority(GPIOTE_IRQn, IRQ_PRIORITY);
  NVIC_EnableIRQ(GPIOTE_IRQn);
  
  // Clear pending events
  NRF_GPIOTE->EVENTS_PORT = 0;
  
  // Get the current state of the input pins
  u32 state = NRF_GPIO->IN;
  
  // Enable interrupt on port event
  NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
  
  // Configure each motor pin as an output
  for (i = 0; i < MOTOR_COUNT; i++)
  {
    // Configure the pins for the motor bridge to be outputs and default high (stopped)
    MotorInfo* motorInfo = &m_motors[i];
    nrf_gpio_pin_set(motorInfo->forwardUpPin);
    nrf_gpio_pin_set(motorInfo->backwardDownPin);
    nrf_gpio_cfg_output(motorInfo->backwardDownPin);
    nrf_gpio_cfg_output(motorInfo->forwardUpPin);
  
    // Enable sensing for each encoder pin
    ConfigurePinSense(motorInfo->encoderPins[0], state);
    if (motorInfo->encoderPins[1] != ENCODER_NONE)
    {
      ConfigurePinSense(motorInfo->encoderPins[1], state);
    }
  }
}

void MotorsSetPower(Anki::Cozmo::HAL::MotorID motorID, s16 power)
{
  // PPI channel[index * 2] contains the PWM timer capture-compare
  u32 channelMask = 1 << ((u32)motorID << 1);
  
  MotorInfo* motorInfo = &m_motors[motorID];
  
  if (power == 0)
  {
    // Clear the PPI and GPIOTE channels from running and set the lines high (stopped)
    NRF_PPI->CHENCLR = channelMask;
    nrf_gpiote_unconfig(motorID);
    
    nrf_gpio_pin_set(motorInfo->backwardDownPin);
    nrf_gpio_pin_set(motorInfo->forwardUpPin);
  } else {
    // Clamp the PWM power
    u32 value = power < 0 ? -power : power;
    if (value >= TIMER_TICKS_END)
      value = TIMER_TICKS_END - 1;
    
    *motorInfo->pwm = value;
    
    // Check the sign bit for the current direction (designated by unitsPerTick) and the new power
    bool isDifferentDirection = ((s32)motorInfo->unitsPerTick >> 31) != ((s32)power >> 31);
    
    // Make sure the PPI and GPIOTE channels for this PWM are enabled
    if (!(NRF_PPI->CHEN & channelMask) || isDifferentDirection)
    {
      // Unconfigure the current GPIOTE setting and change it below.
      // This also prevents glitching the PWM duty cycle via a race condition.
      nrf_gpiote_unconfig(motorID);
      
      if (power < 0)
      {
        if (motorInfo->unitsPerTick > 0)
          motorInfo->unitsPerTick = -motorInfo->unitsPerTick;
        
        // Switch the task to the correct pin and disable the other direction
        nrf_gpiote_task_config(motorID, motorInfo->backwardDownPin,
          NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
        nrf_gpio_pin_set(motorInfo->forwardUpPin);
      } else {
        if (motorInfo->unitsPerTick < 0)
          motorInfo->unitsPerTick = -motorInfo->unitsPerTick;
        
        nrf_gpiote_task_config(motorID, motorInfo->forwardUpPin,
          NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
        nrf_gpio_pin_set(motorInfo->backwardDownPin);
      }
      
      // Enable the PPI channel
      NRF_PPI->CHENSET = channelMask;
    }
  }
}

static void HandlePinTransition(MotorInfo* motorInfo, u8 encoderIndex, u32 pinState)
{
  u32 pin = motorInfo->encoderPins[encoderIndex];
  u32 mask = 1 << pin;
  EncoderData* encoderData = &motorInfo->encoderData[encoderIndex];
  
  u32 transition = (pinState ^ m_pinsHigh) & mask;
  
  // Toggle the sense level (for whack-a-mole)
  if (transition)
  {
    // Check for high to low transition and invert the sensing
    if (!(pinState & mask))
    {
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
      m_pinsHigh &= ~mask;
    } else {
      // Handle low to high transition
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);
      m_pinsHigh |= mask;
      
      u32 ticks = Anki::Cozmo::HAL::GetMicroCounter();
      if ((ticks - encoderData->lastTick) > DEBOUNCE_US)
      {
        encoderData->delta = ticks - encoderData->lastTick;
        encoderData->lastTick = ticks;
        motorInfo->position += motorInfo->unitsPerTick;
      }
    }
  }
}

extern "C"
void GPIOTE_IRQHandler()
{
  // Grab the current state of the input pins
  u32 state = NRF_GPIO->IN;
  
  // Clear the event/interrupt
  NRF_GPIOTE->EVENTS_PORT = 0;
  
  for (int i = 0; i < MOTOR_COUNT; i++)
  {
    MotorInfo* motorInfo = &m_motors[i];
    
    HandlePinTransition(motorInfo, 0, state);
    if (motorInfo->encoderPins[1] != ENCODER_NONE)
    {
      HandlePinTransition(motorInfo, 1, state);
    }
  }
}
