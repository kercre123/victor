#include "motors.h"
#include "spiData.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include <limits.h>

#include "uart.h"

#define TO_FIXED(x) ((x) * 65535)
#define FIXED_MUL(x, y) ((s32)(((s64)(x) * (s64)(y)) >> 16))
#define FIXED_DIV(x, y) ((s32)(((s64)(x) << 16) / (y)))

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

#define ABS(x) ((x) < 0 ? -(x) : (x))

namespace
{
  enum MotorID
  {
    MOTOR_LEFT_WHEEL,
    MOTOR_RIGHT_WHEEL,
    MOTOR_LIFT,
    MOTOR_HEAD
  };
  
  struct MotorInfo
  {
    // These few are constant; will refactor into separate structs again soon.
    u8 forwardUpPin;
    u8 backwardDownPin;
    u8 encoderPins[2];
    
    Fixed unitsPerTick;
    Fixed position;
    Fixed lastPosition;
    u32 count;
    u32 lastCount;
    
    s16 nextPWM;
    s16 oldPWM;
  };
 
  const u32 IRQ_PRIORITY = 1;
  
  // 16 MHz timer with PWM running at 20kHz
  const s16 TIMER_TICKS_END = (16000000 / 20000) - 1;
  
  const s16 PWM_DIVISOR = SHRT_MAX / TIMER_TICKS_END;
  
  const u8 LEFT_WHEEL_FORWARD_PIN = 7;
  const u8 LEFT_WHEEL_BACKWARD_PIN = 3;
  const u8 RIGHT_WHEEL_FORWARD_PIN = 22;
  const u8 RIGHT_WHEEL_BACKWARD_PIN = 23;
  const u8 LIFT_UP_PIN = 24;
  const u8 LIFT_DOWN_PIN = 25;
  const u8 HEAD_UP_PIN = 10;
  const u8 HEAD_DOWN_PIN = 9;
  
  const u8 ENCODER_NONE = 0xFF;
  const u8 ENCODER_1_PIN = 4;
  const u8 ENCODER_2_PIN = 21;
  const u8 ENCODER_3A_PIN = 8;
  const u8 ENCODER_3B_PIN = 19;
  const u8 ENCODER_4A_PIN = 20;
  const u8 ENCODER_4B_PIN = 18;
  
  // Given a gear ratio of 283.5:1 and 88mm wheel circumference and 2 encoder
  // ticks per revolution, we compute the meters per tick as (using 2 edges):
  const Fixed METERS_PER_TICK = TO_FIXED(0.088 / (283.5 * 2.0));
  
  // Given a gear ratio of 729:1 and 4 encoder ticks per revolution, we
  // compute the radians per tick on the lift as:
  const Fixed RADIANS_PER_LIFT_TICK = TO_FIXED((0.5 * 3.14159265359) / 729.0);
  
  // Given a gear ratio of 67.5:1 and 4 encoder ticks per revolution, we
  // compute the radians per tick on the head as:
  const Fixed RADIANS_PER_HEAD_TICK = TO_FIXED((0.5 * 3.14159265359) / 67.5);

  // If no encoder activity for 200ms, we may as well be stopped
  const u32 ENCODER_TIMEOUT_COUNT = 200000 * US_PER_COUNT;

  // Set the debounce to reject all noise above it (100 us)
  const u32 DEBOUNCE_COUNT = 2 * US_PER_COUNT;
  
  // NOTE: Do NOT re-order the MotorID enum, because this depends on it
  MotorInfo m_motors[MOTOR_COUNT] =
  {
    {
      LEFT_WHEEL_FORWARD_PIN,
      LEFT_WHEEL_BACKWARD_PIN,
      ENCODER_1_PIN,
      ENCODER_NONE,
      METERS_PER_TICK,
      0, 0, 0, 0, 0, 0
    },
    {
      RIGHT_WHEEL_FORWARD_PIN,
      RIGHT_WHEEL_BACKWARD_PIN,
      ENCODER_2_PIN,
      ENCODER_NONE,
      METERS_PER_TICK,
      0, 0, 0, 0, 0, 0
    },
    {
      LIFT_UP_PIN,
      LIFT_DOWN_PIN,
      ENCODER_3A_PIN,
      ENCODER_3B_PIN,
      RADIANS_PER_LIFT_TICK,
      0, 0, 0, 0, 0, 0
    },
    {
      HEAD_UP_PIN,
      HEAD_DOWN_PIN,
      ENCODER_4A_PIN,
      ENCODER_4B_PIN,
      RADIANS_PER_HEAD_TICK,
      0, 0, 0, 0, 0, 0
    },
  };
  
  //const u32 MOTOR_COUNT = sizeof(m_motors) / sizeof(MotorInfo);
  
  u32 m_lastState = 0;
}

static void ConfigureTimer(NRF_TIMER_Type* timer, const u8 taskChannel, const u8 ppiChannel)
{
  // TODO: There's a PAN about calling TASKS_STOP and TASKS_START in the same
  // clock period.
  
  // Ensure the timer is stopped
  timer->TASKS_STOP = 1;
  
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
  timer->CC[2] = TIMER_TICKS_END;    // Trigger on timer wrap-around at 800
  timer->CC[3] = TIMER_TICKS_END;    // Trigger on timer wrap-around at 800
  
  // Configure the timer to reset the count when it hits the period defined in compare[3]
  timer->SHORTS = 
    (1 << TIMER_SHORTS_COMPARE3_CLEAR_Pos) |
    (1 << TIMER_SHORTS_COMPARE2_CLEAR_Pos);
  
  // TODO: Verify PAN 33: TIMER: One CC register is not able to generate an 
  // event for the second of two subsequent counter/ timer values.
  
  // Configure PPI channels to toggle the output PWM pins on matching timer compare
  // and toggle the GPIO for the selected duty cycle.
  
  // Match compare[0] (PWM n + 0)
  NRF_PPI->CH[ppiChannel + 0].EEP = (u32)&timer->EVENTS_COMPARE[0];
  NRF_PPI->CH[ppiChannel + 0].TEP = (u32)&NRF_GPIOTE->TASKS_OUT[taskChannel + 0];
  
  // Match compare[2] (timer wrap-around)
  NRF_PPI->CH[ppiChannel + 1].EEP = (u32)&timer->EVENTS_COMPARE[2];
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
    m_lastState |= mask;
  } else {
    nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
  }
}

// Reset Nordic tasks to allow less glitchy changes.
// Without a reset, the polarity will become permanently inverted.
static void ConfigureTask(u8 motorID)
{
  MotorInfo* motorInfo = &m_motors[motorID];
  u8 pinPWM;
  u8 pinHigh;
  if (motorInfo->nextPWM > 0)
  {
    if (motorInfo->unitsPerTick < 0)
          motorInfo->unitsPerTick = -motorInfo->unitsPerTick;
    
    pinPWM = motorInfo->forwardUpPin;
    pinHigh = motorInfo->backwardDownPin;
    
    nrf_gpio_pin_set(pinHigh);
    nrf_gpiote_task_config(motorID, pinPWM,
      NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
  } else if (motorInfo->nextPWM < 0) {
    if (motorInfo->unitsPerTick > 0)
          motorInfo->unitsPerTick = -motorInfo->unitsPerTick;
    
    pinPWM = motorInfo->backwardDownPin;
    pinHigh = motorInfo->forwardUpPin;
    
    nrf_gpio_pin_set(pinHigh);
    nrf_gpiote_task_config(motorID, pinPWM,
      NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
  } else {
    nrf_gpiote_unconfig(motorID);
    __NOP();
    __NOP();
    __NOP();
    nrf_gpio_pin_set(motorInfo->forwardUpPin);
    nrf_gpio_pin_set(motorInfo->backwardDownPin);
  }
}

void MotorsInit()
{
  int i;
  
  // Configure TIMER1 and TIMER2 with the appropriate task and PPI channels
  ConfigureTimer(NRF_TIMER1, 0, 0);
  ConfigureTimer(NRF_TIMER2, 2, 4);
  
  // Enable PPI channels for timer PWM and reset
  NRF_PPI->CHEN = 0xFF;
  
  // Start the timers
  NRF_TIMER1->TASKS_START = 1;
  NRF_TIMER2->TASKS_START = 1;
  
  // Clear all GPIOTE interrupts
  NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;
  
  // Clear pending interrupts and enable the GPIOTE interrupt
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
    
    // Enable sensing for each encoder pin (only one per quadrature encoder)
    ConfigurePinSense(motorInfo->encoderPins[0], state);
    if (motorInfo->encoderPins[1] != ENCODER_NONE)
    {
      nrf_gpio_cfg_input(motorInfo->encoderPins[1], NRF_GPIO_PIN_NOPULL);
    }
  }
}

void MotorsSetPower(u8 motorID, s16 power)
{
  // Scale from [0, SHRT_MAX] to [0, TIMER_TICKS_END-1]
  power /= PWM_DIVISOR;
  
  // Clamp the PWM power
  if (power >= TIMER_TICKS_END)
    power = TIMER_TICKS_END - 1;
  else if (power <= -TIMER_TICKS_END)
    power = -TIMER_TICKS_END + 1;

  // Store the PWM value for the MotorsUpdate() and keep the sign
  MotorInfo* motorInfo = &m_motors[motorID];
  motorInfo->oldPWM = motorInfo->nextPWM;
  motorInfo->nextPWM = power;
}

Fixed MotorsGetSpeed(u8 motorID)
{
  MotorInfo* motorInfo = &m_motors[motorID];
  
  // If the motor hasn't moved in a while, consider it stopped
  if ((GetCounter() - motorInfo->count) > ENCODER_TIMEOUT_COUNT)
  {
    motorInfo->lastCount = motorInfo->count;
    motorInfo->lastPosition = motorInfo->position;
  }
  
  // Convert deltaCount to fixed-point seconds by div 128
  Fixed deltaSeconds = (motorInfo->count - motorInfo->lastCount) >> 7;
  Fixed deltaPosition = motorInfo->position - motorInfo->lastPosition;
  
  if (!deltaSeconds)
  {
    return 0;
  }
  
  // Update count and position for the next iteration if motor has moved far enough
  if (ABS(deltaPosition) > ABS(motorInfo->unitsPerTick))
  {
    motorInfo->lastCount = motorInfo->count;
    motorInfo->lastPosition = motorInfo->position;
  }
  
  return FIXED_DIV(deltaPosition, deltaSeconds);
}

void MotorsUpdate()
{
  // Stop the timer task and clear it, along with GPIO for the motors
  if ((m_motors[0].nextPWM != m_motors[0].oldPWM) ||
      (m_motors[1].nextPWM != m_motors[1].oldPWM))
  {
    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;
    
    // Reconfigure the GPIOTE for the motors
    ConfigureTask(MOTOR_LEFT_WHEEL);
    ConfigureTask(MOTOR_RIGHT_WHEEL);
    
    // Update the PWM values
    NRF_TIMER1->CC[0] = m_motors[0].nextPWM < 0 ? -m_motors[0].nextPWM : m_motors[0].nextPWM;
    NRF_TIMER1->CC[1] = m_motors[1].nextPWM < 0 ? -m_motors[1].nextPWM : m_motors[1].nextPWM;
    
    // Restart the timer
    NRF_TIMER1->TASKS_START = 1;
  }
  
  if ((m_motors[2].nextPWM != m_motors[2].oldPWM) ||
      (m_motors[3].nextPWM != m_motors[3].oldPWM))
  {
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;
    
    ConfigureTask(MOTOR_LIFT);
    ConfigureTask(MOTOR_HEAD);
    
    NRF_TIMER2->CC[0] = m_motors[2].nextPWM < 0 ? -m_motors[2].nextPWM : m_motors[2].nextPWM;
    NRF_TIMER2->CC[1] = m_motors[3].nextPWM < 0 ? -m_motors[3].nextPWM : m_motors[3].nextPWM;
    
    NRF_TIMER2->TASKS_START = 1;
  }
  
  // Update the SPI data structure to send data back to the head
  g_dataToHead.speeds[0] = MotorsGetSpeed(0);
  g_dataToHead.speeds[1] = MotorsGetSpeed(1);
  g_dataToHead.speeds[2] = MotorsGetSpeed(2);
  g_dataToHead.speeds[3] = MotorsGetSpeed(3);
  
  g_dataToHead.positions[0] = m_motors[0].position;
  g_dataToHead.positions[1] = m_motors[1].position;
  g_dataToHead.positions[2] = m_motors[2].position;
  g_dataToHead.positions[3] = m_motors[3].position;
}

void MotorsPrintEncodersRaw()
{
  UARTPutChar('0' + nrf_gpio_pin_read(ENCODER_1_PIN));
  UARTPutChar(' ');
  UARTPutChar('0' + nrf_gpio_pin_read(ENCODER_2_PIN));
  UARTPutChar(' ');
  UARTPutChar('0' + nrf_gpio_pin_read(ENCODER_3A_PIN));
  UARTPutChar(' ');
  UARTPutChar('0' + nrf_gpio_pin_read(ENCODER_3B_PIN));
  UARTPutChar(' ');
  UARTPutChar('0' + nrf_gpio_pin_read(ENCODER_4A_PIN));
  UARTPutChar(' ');
  UARTPutChar('0' + nrf_gpio_pin_read(ENCODER_4B_PIN));
  UARTPutChar('\n');
}

// TODO: This should be optimized
static void HandlePinTransition(MotorInfo* motorInfo, u32 pinState, u32 count)
{
  u32 pin = motorInfo->encoderPins[0];
  u32 mask = 1 << pin;
  
  u32 transition = (pinState ^ m_lastState) & mask;
  
  // Toggle the sense level (for whack-a-mole)
  if (transition)
  {
    // Check for high to low transition and invert the sensing
    if (!(pinState & mask))
    {
      // NOTE: Using this edge because of the orientation of the encoder.
      // If the encoder was rotated 180 degrees, then the other edge should be used.
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
      
      if ((count - motorInfo->count) > DEBOUNCE_COUNT)
      {
        motorInfo->count = count;
        u32 pin2 = motorInfo->encoderPins[1];
        
        if (pin2 != ENCODER_NONE)
        {
          // Check quadrature encoder state for forward vs backward
          if (pinState & (1 << pin2))
            motorInfo->position += ABS(motorInfo->unitsPerTick);
          else
            motorInfo->position -= ABS(motorInfo->unitsPerTick);
        } else {
          motorInfo->position += motorInfo->unitsPerTick;
        }
      }
      
    } else {
      // Handle low to high transition
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);
    }
  }
}

extern "C"
void GPIOTE_IRQHandler()
{
  // Clear the event/interrupt
  NRF_GPIOTE->EVENTS_PORT = 0;
  // Note:  IN could have already changed - but if it did, it would be a glitch anyway
  // Grab the current state of the input pins
  u32 state = NRF_GPIO->IN;

  u32 count = GetCounter();
  
  for (int i = 0; i < MOTOR_COUNT; i++)
  {
    MotorInfo* motorInfo = &m_motors[i];    
    HandlePinTransition(motorInfo, state, count);
  }
  
  m_lastState = state;
}
