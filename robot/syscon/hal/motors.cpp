#include "motors.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"

#define PRIORITY    1

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
  
  // TODO: Fill with real indices
  const u8 LEFT_WHEEL_FORWARD_PIN = 10;
  const u8 LEFT_WHEEL_BACKWARD_PIN = 11;
  const u8 RIGHT_WHEEL_FORWARD_PIN = 12;
  const u8 RIGHT_WHEEL_BACKWARD_PIN = 13;
  const u8 LIFT_UP_PIN = 14;
  const u8 LIFT_DOWN_PIN = 15;
  const u8 HEAD_UP_PIN = 16;
  const u8 HEAD_DOWN_PIN = 17;
  
  static const MotorInfo m_motors[] =
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
  static const f32 RADIANS_PER_LIFT_TICK = (0.5 * M_PI) / 815.4;

  // If no encoder activity for 200ms, we may as well be stopped
  static const u32 ENCODER_TIMEOUT_US = 200000;

  // Set a max speed and reject all noise above it
  static const u32 MAX_SPEED_MM_S = 500;
  static const u32 DEBOUNCE_US = (MM_PER_TICK * 1000) / MAX_SPEED_MM_S;
  
  // Setup the GPIO indices
  const u8 ENCODER_COUNT = 4;
  const u8 ENCODER_1_PIN = 1;  // TODO: Fill with real indices
  const u8 ENCODER_2_PIN = 2;
  const u8 ENCODER_3_PIN = 3;
  const u8 ENCODER_4_PIN = 4;
  
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

void MotorInit()
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
