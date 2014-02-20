#include "motors.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"

#define PRIORITY    1

// 16 MHz timer with PWM running at 20kHz
#define TIMER_TICKS_END ((16000000 / 20000) - 1)

namespace
{  
  struct MotorInfo
  {
    u8 forwardUpPin;
    u8 backwardDownPin;
  };
  
  struct MotorPosition
  {
    s32 position;
    u32 lastTick;
    u32 delta;
    s32 unitsPerTick;
    u8 pin;
  };
  
  const u8 LEFT_WHEEL_FORWARD_PIN = 7;
  const u8 LEFT_WHEEL_BACKWARD_PIN = 3;
  const u8 RIGHT_WHEEL_FORWARD_PIN = 9;
  const u8 RIGHT_WHEEL_BACKWARD_PIN = 10;
  const u8 LIFT_UP_PIN = 25;
  const u8 LIFT_DOWN_PIN = 24;
  const u8 HEAD_UP_PIN = 23;
  const u8 HEAD_DOWN_PIN = 22;
  
  const MotorInfo m_motors[] =
  {
    {
      LEFT_WHEEL_FORWARD_PIN,
      LEFT_WHEEL_BACKWARD_PIN
    },
    {
      RIGHT_WHEEL_FORWARD_PIN,
      RIGHT_WHEEL_BACKWARD_PIN
    },
    {
      LIFT_UP_PIN,
      LIFT_DOWN_PIN
    },
    {
      HEAD_UP_PIN,
      HEAD_DOWN_PIN
    },
  };
  
  // Given a gear ratio of 91.7:1 and 125.67mm wheel circumference, we compute
  // the mm per tick as (using 4 magnets):
  const f32 MM_PER_TICK = 125.67 / 182.0 / 4.0;
  
  // Given a gear ratio of 815.4:1 and 4 encoder ticks per revolution, we
  // compute the radians per tick on the lift as:
  const f32 RADIANS_PER_LIFT_TICK = (0.5 * M_PI) / 815.4;

  // If no encoder activity for 200ms, we may as well be stopped
  const u32 ENCODER_TIMEOUT_US = 200000;

  // Set a max speed and reject all noise above it
  const u32 MAX_SPEED_MM_S = 500;
  const u32 DEBOUNCE_US = (MM_PER_TICK * 1000) / MAX_SPEED_MM_S;
  
  // Setup the GPIO indices
  const u8 ENCODER_COUNT = 4;
  const u8 ENCODER_1_PIN = 4;
  const u8 ENCODER_2_PIN = 21;
  const u8 ENCODER_3_PIN = 8;
  // 3B
  const u8 ENCODER_4_PIN = 20;
  // 4B
  
  // NOTE: Do NOT re-order the MotorID enum, because this depends on it
  MotorPosition m_motorPositions[Anki::Cozmo::HAL::MOTOR_COUNT] =
  {
    {0, 0, 0, MM_PER_TICK, ENCODER_1_PIN},  // MOTOR_LEFT_WHEEL
    {0, 0, 0, MM_PER_TICK, ENCODER_2_PIN},  // MOTOR_RIGHT_WHEEL
    {0, 0, 0, RADIANS_PER_LIFT_TICK, ENCODER_3_PIN},  // MOTOR_LIFT
    {0, 0, 0, RADIANS_PER_LIFT_TICK, ENCODER_4_PIN},  // MOTOR_HEAD
    //{0}  // Zero out the rest
  };
  
  u32 m_pinsHigh = 0;
}

static void ConfigureTimer(NRF_TIMER_Type* timer, const u8 taskChannel, const u8 ppiChannel)
{
  // Configure the timer to be in 16-bit timer mode
  timer->MODE = TIMER_MODE_MODE_Timer;
  timer->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
  timer->PRESCALER = 0;
  
  // Clears the timer to 0
  timer->TASKS_CLEAR = 1;
  
  // XXX: 0 and 799 are invalid
  // for 0, use GPIO to disable / cut power to motor (don't waste current)
  // for 799, just clamp to 798
  timer->CC[0] = 300;  // PWM n + 0
  timer->CC[1] = 500;  // PWM n + 1
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

void MotorsInit()
{
  int i;
  // Clear all GPIOTE interrupts
  NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;
  
  // Clear pending interrupts
  NVIC_ClearPendingIRQ(GPIOTE_IRQn);
  NVIC_SetPriority(GPIOTE_IRQn, PRIORITY);
  NVIC_EnableIRQ(GPIOTE_IRQn);
  
  // Clear pending events
  NRF_GPIOTE->EVENTS_PORT = 0;
  
  // Configure TIMER1 and TIMER2 with the appropriate task and PPI channels
  ConfigureTimer(NRF_TIMER1, 0, 0);
  ConfigureTimer(NRF_TIMER2, 2, 4);
  
  // Enable PPI channels
  NRF_PPI->CHEN = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) |
                  (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
  
  // Star the timers
  NRF_TIMER1->TASKS_START = 1;
  NRF_TIMER2->TASKS_START = 1;
  
  // Configure each motor pin as an output
  for (i = 0; i < sizeof(m_motors) / sizeof(MotorInfo); i++)
  {
    const MotorInfo* motorInfo = &m_motors[i];
    nrf_gpio_cfg_output(motorInfo->backwardDownPin);
    nrf_gpio_cfg_output(motorInfo->forwardUpPin);
    
    nrf_gpio_pin_clear(motorInfo->forwardUpPin);
    
    nrf_gpio_pin_clear(motorInfo->backwardDownPin);
    
    // Configure the GPIOTE channels to toggle for PWM
    //nrf_gpiote_task_config(i, motorInfo->backwardDownPin, 
    //  NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
    
    //nrf_gpiote_task_config(i, motorInfo->forwardUpPin, 
    //  NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
  }
  
  nrf_gpio_pin_set(m_motors[0].forwardUpPin);
  nrf_gpio_pin_clear(m_motors[0].backwardDownPin);
  
  // Get the current state of the input pins
  u32 state = NRF_GPIO->IN;
  
  // Enable interrupt on port event
  NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
  
  // Enable sensing for each pin
  for (i = 0; i < ENCODER_COUNT; i++)
  {
    u32 pin = m_motorPositions[i].pin;
    u32 mask = 1 << pin;
    
    // Configure initial pin sensing (used for inversion with whack-a-mole)
    if (state & mask)
    {
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_LOW);
    } else {
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
      m_pinsHigh |= mask;
    }
  }
}

void MotorSetPower(Anki::Cozmo::HAL::MotorID motorID, u16 power)
{
}

extern "C"
void GPIOTE_IRQHandler()
{
  int i;
  // Grab the current state of the input pins
  u32 state = NRF_GPIO->IN;
  
  // Clear the event
  NRF_GPIOTE->EVENTS_PORT = 0;
  
  for (i = 0; i < ENCODER_COUNT; i++)
  {
    MotorPosition* motorPosition = &m_motorPositions[i];
    u32 pin = motorPosition->pin;
    u32 mask = 1 << pin;
    
    u32 transition = (state ^ ~m_pinsHigh) & mask;
    
    // Toggle the sense level (for whack-a-mole)
    if (transition)
    {
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
      m_pinsHigh |= mask;
      
      // Handle low to high transition
      u32 ticks = Anki::Cozmo::HAL::GetMicroCounter();
      if ((ticks - motorPosition->lastTick) > DEBOUNCE_US)
      {
        motorPosition->delta = ticks - motorPosition->lastTick;
        motorPosition->lastTick = ticks;
        motorPosition->position += motorPosition->unitsPerTick;
      }
    } else {
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_LOW);
      m_pinsHigh &= ~mask;
    }
  }
}
