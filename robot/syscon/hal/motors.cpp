#include "motors.h"
#include "anki/cozmo/robot/spineData.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "hardware.h"
#include <limits.h>

#include "debug.h"

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;




#define ABS(x) ((x) < 0 ? -(x) : (x))

#define ROBOT4 // robot 4.0

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
  u8 n1Pin;
  u8 n2Pin;
  u8 pPin;
  u8 isBackward;   // True if wired backward
  u8 lastP;        // Last value of pPin
  u8 encoderPins[2];
  
  Fixed unitsPerTick;
  Fixed64 position;
  Fixed64 lastPosition;
  u32 count;
  u32 lastCount;
  
  s16 nextPWM;
  s16 oldPWM;
};

const u32 IRQ_PRIORITY = 1;

// 16 MHz timer with PWM running at 20kHz
const s16 TIMER_TICKS_END = (16000000 / 20000) - 1;

const s16 PWM_DIVISOR = SHRT_MAX / TIMER_TICKS_END;

// Encoder scaling reworked for Cozmo 4.0

// Given a gear ratio of 161.5:1 and 94mm wheel circumference and 2 ticks * 4 teeth
// for 8 encoder ticks per revolution, we compute the meters per tick as:
// Applying a slip factor correction of 94.8%
const Fixed_8_24 METERS_PER_TICK = TO_FIXED_8_24((0.948 * 0.125 * 0.0292 * 3.14159265359) / 173.43); // 1052

// Given a gear ratio of 172.68:1 and 4 encoder ticks per revolution, we
// compute the radians per tick on the lift as:
const Fixed RADIANS_PER_LIFT_TICK = TO_FIXED((0.25 * 3.14159265359) / 172.68);

// Given a gear ratio of 348.77:1 and 8 encoder ticks per revolution, we
// compute the radians per tick on the head as:
#ifdef ROBOT4
const Fixed RADIANS_PER_HEAD_TICK = TO_FIXED((0.125 * 3.14159265359) / 348.77);
#else 
const Fixed RADIANS_PER_HEAD_TICK = TO_FIXED((0.125 * 3.14159265359) / 324.0);
#endif  
// If no encoder activity for 200ms, we may as well be stopped
const u32 ENCODER_TIMEOUT_COUNT = 200 * COUNT_PER_MS;
const u32 ENCODER_NONE = 0xFF;

// Set the debounce to reject all noise above it
// const u8 DEBOUNCE_COUNT = 511;   // About 60uS?

// NOTE: Do NOT re-order the MotorID enum, because this depends on it
MotorInfo m_motors[MOTOR_COUNT] =
{
  {
    PIN_LEFT_N1,
    PIN_LEFT_N2,
    PIN_LEFT_P,
    false,
    0,
    PIN_ENCODER_LEFT,
    ENCODER_NONE,
    1, // units per tick = 1 tick
    0, 0, 0, 0, 0, 0
  },
  {
    PIN_RIGHT_N1,
    PIN_RIGHT_N2,
    PIN_RIGHT_P,
    false,
    0,
    PIN_ENCODER_RIGHT,
    ENCODER_NONE,
    1, // units per tick = 1 tick
    0, 0, 0, 0, 0, 0
  },
  {
    PIN_LIFT_N1,
    PIN_LIFT_N2,
    PIN_LIFT_P,
#ifdef ROBOT4
    true,
#else
    false,
#endif
    0,
    PIN_ENCODER_LIFTA,
    PIN_ENCODER_LIFTB,
    RADIANS_PER_LIFT_TICK,
    0, 0, 0, 0, 0, 0
  },
  {
    PIN_HEAD_N1,
    PIN_HEAD_N2,
    PIN_HEAD_P,
#ifdef ROBOT4
    false,
#else
    true,
#endif 
    0,
    PIN_ENCODER_HEADA,
    PIN_ENCODER_HEADB,
    RADIANS_PER_HEAD_TICK,
    0, 0, 0, 0, 0, 0
  },
};

//const u32 MOTOR_COUNT = sizeof(m_motors) / sizeof(MotorInfo);

u32 m_lastState = 0;

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
static void ConfigureTask(u8 motorID, volatile u32 *timer)
{
  MotorInfo* motorInfo = &m_motors[motorID];

  // Zero
  if (motorInfo->nextPWM == 0) {
    nrf_gpiote_task_disable(motorID);
    nrf_gpio_pin_clear(motorInfo->n1Pin);
    nrf_gpio_pin_clear(motorInfo->n2Pin);
  
  // Forward
  } else if ((motorInfo->nextPWM > 0) != motorInfo->isBackward)
  {
    // Drive P2+N1
    nrf_gpiote_task_disable(motorID);
    nrf_gpio_pin_clear(motorInfo->n1Pin);
    nrf_gpio_pin_clear(motorInfo->n2Pin);
    nrf_gpio_pin_clear(motorInfo->pPin);    // P=0 is P2+N1
    
    // XXX: If P drive doesn't match, wait until next update to start motor - to workaround 2.1 hardware glitch
    if (0 != motorInfo->lastP)
    {
      motorInfo->lastP = 0;
      return;   // Don't update oldPWM, so we come in here again
    } else {
      nrf_gpiote_task_configure(motorID, motorInfo->n1Pin,
        NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);      
      nrf_gpiote_task_enable(motorID);
    }
    
  // Backward
  } else {
    if (motorInfo->unitsPerTick > 0)
      motorInfo->unitsPerTick = -motorInfo->unitsPerTick;
        
    // Drive P1+N2
    nrf_gpiote_task_disable(motorID);
    nrf_gpio_pin_clear(motorInfo->n1Pin);
    nrf_gpio_pin_clear(motorInfo->n2Pin);
    nrf_gpio_pin_set(motorInfo->pPin);      // P=1 is P1+N2
    
    // XXX: If P drive doesn't match, wait until next update to start motor - to workaround 2.1 hardware glitch
    if (1 != motorInfo->lastP)
    {
      motorInfo->lastP = 1;
      return;   // Don't update oldPWM, so we come in here again
    } else {
      nrf_gpiote_task_configure(motorID, motorInfo->n2Pin,
        NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);    
      nrf_gpiote_task_enable(motorID);
    }
  }
  
  // Point encoder ticks in same direction as nextPWM
  if (motorInfo->unitsPerTick > 0 && motorInfo->nextPWM < 0)
      motorInfo->unitsPerTick = -motorInfo->unitsPerTick;
  if (motorInfo->unitsPerTick < 0 && motorInfo->nextPWM > 0)
      motorInfo->unitsPerTick = -motorInfo->unitsPerTick;
      
  motorInfo->oldPWM = motorInfo->nextPWM;
  
  // Update the timer channel
  *timer = motorInfo->nextPWM < 0 ? -motorInfo->nextPWM : motorInfo->nextPWM;  
}

void Motors::init()
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
    // Configure the pins for the motor bridge to be outputs and default low (stopped)
    MotorInfo* motorInfo = &m_motors[i];
    nrf_gpio_pin_clear(motorInfo->n1Pin);
    nrf_gpio_pin_clear(motorInfo->n2Pin);
    nrf_gpio_pin_clear(motorInfo->pPin);
    nrf_gpio_cfg_output(motorInfo->n1Pin);
    nrf_gpio_cfg_output(motorInfo->n2Pin);
    nrf_gpio_cfg_output(motorInfo->pPin);
    
    // Enable sensing for each encoder pin (only one per quadrature encoder)
    ConfigurePinSense(motorInfo->encoderPins[0], state);
    if (motorInfo->encoderPins[1] != ENCODER_NONE)
    {
      nrf_gpio_cfg_input(motorInfo->encoderPins[1], NRF_GPIO_PIN_NOPULL);
    }
  }
}

void Motors::setPower(u8 motorID, s16 power)
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
  motorInfo->nextPWM = power;
}

Fixed Motors::getSpeed(u8 motorID)
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

void Motors::update()
{
  // Stop the timer task and clear it, along with GPIO for the motors
  if ((m_motors[0].nextPWM != m_motors[0].oldPWM) ||
      (m_motors[1].nextPWM != m_motors[1].oldPWM))
  {
    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;
    
    // Try to reconfigure the motors - if there are any faults, bail out
    ConfigureTask(MOTOR_LEFT_WHEEL, &(NRF_TIMER1->CC[0]));
    ConfigureTask(MOTOR_RIGHT_WHEEL, &(NRF_TIMER1->CC[1]));

    // Restart the timer
    NRF_TIMER1->TASKS_START = 1;
  }
  
  if ((m_motors[2].nextPWM != m_motors[2].oldPWM) ||
      (m_motors[3].nextPWM != m_motors[3].oldPWM))
  {
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;
    
    // Try to reconfigure the motors - if there are any faults, bail out
    ConfigureTask(MOTOR_LIFT, &(NRF_TIMER2->CC[0]));
    ConfigureTask(MOTOR_HEAD, &(NRF_TIMER2->CC[1]));

    // Restart the timer
    NRF_TIMER2->TASKS_START = 1;
  }
  
  // Update the SPI data structure to send data back to the head
  // Wheel position and speed are recorded in encoder ticks, so we
  // are applying conversions here for now, until code is refactored
  Fixed64 tmp;
  tmp = METERS_PER_TICK;
  tmp *= Motors::getSpeed(0);
  g_dataToHead.speeds[0] = (Fixed)TO_FIXED_8_24_TO_16_16(tmp);
  tmp = METERS_PER_TICK;
  tmp *= Motors::getSpeed(1);
  g_dataToHead.speeds[1] = (Fixed)TO_FIXED_8_24_TO_16_16(tmp);
  g_dataToHead.speeds[2] = Motors::getSpeed(2);
  g_dataToHead.speeds[3] = Motors::getSpeed(3);
  
  tmp = METERS_PER_TICK;
  tmp *= m_motors[0].position;
  g_dataToHead.positions[0] = (s32)TO_FIXED_8_24_TO_16_16(tmp);
  tmp = METERS_PER_TICK;
  tmp *= m_motors[1].position;
  g_dataToHead.positions[1] = (s32)TO_FIXED_8_24_TO_16_16(tmp);
  g_dataToHead.positions[2] = m_motors[2].position;
  g_dataToHead.positions[3] = m_motors[3].position;
}

void Motors::printEncodersRaw()
{
  Fixed64 tmp1, tmp2;
  tmp1 = METERS_PER_TICK;
  tmp1 *= m_motors[0].position;

  tmp2 = METERS_PER_TICK;
  tmp2 *= m_motors[1].position;

  UART::print("L%cR%cA%c%cH%c%c L%7iR%7iA%7iH%7i",
      '0' + nrf_gpio_pin_read(PIN_ENCODER_LEFT),
      '0' + nrf_gpio_pin_read(PIN_ENCODER_RIGHT),
      '0' + nrf_gpio_pin_read(PIN_ENCODER_LIFTA),
      '0' + nrf_gpio_pin_read(PIN_ENCODER_LIFTB),
      '0' + nrf_gpio_pin_read(PIN_ENCODER_HEADA),
      '0' + nrf_gpio_pin_read(PIN_ENCODER_HEADB),
      (Fixed)TO_FIXED_8_24_TO_16_16(tmp1),
      (Fixed)TO_FIXED_8_24_TO_16_16(tmp2),
      (int)m_motors[2].position, (int)m_motors[3].position);
}



// Get wheel ticks
s32 Motors::debugWheelsGetTicks(u8 motorID)
{
  return m_motors[motorID].position;
}

// Encoder code must be optimized for speed - this macro is faster than Nordic's
#define fast_gpio_cfg_sense_input(pin_number, sense_config) \
  NRF_GPIO->PIN_CNF[pin_number] = (sense_config << GPIO_PIN_CNF_SENSE_Pos) \
                                | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) \
                                | (NRF_GPIO_PIN_NOPULL << GPIO_PIN_CNF_PULL_Pos) \
                                | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) \
                                | (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos)

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
      fast_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_SENSE_HIGH);
      
      if (1) // && (count - motorInfo->count) > DEBOUNCE_COUNT)
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
      fast_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_SENSE_LOW);
    }
  }
}

void Motors::printEncoder(u8 motorID) // XXX: wheels are in encoder ticks, not meters
{
  int i = m_motors[motorID].position;
  UART::print("%c%c%c%c", i, i >> 8, i >> 16, i >> 24);
}

// Apologies for the straight-line code - it's required for performance
// The encoders literally lose ticks unless this code can finish within ~30uS
extern "C"
void GPIOTE_IRQHandler()
{
  // Clear the event/interrupt - err on the side of too many interrupts - since we can ignore duplicates below
  NRF_GPIOTE->EVENTS_PORT = 0;
      
  // Look at the input pins and see if anything has changed, if so, process all changes and then check again
  u32 state;
  while (m_lastState != (state = NRF_GPIO->IN))
  {
    u32 whatChanged = state ^ m_lastState;     
    m_lastState = state;
    u32 count = GetCounter();
    
    // Head encoder (since it moves fastest)
    if (whatChanged & (1 << PIN_ENCODER_HEADA))
    {
      m_motors[MOTOR_HEAD].count = count;
      if (!(state & (1 << PIN_ENCODER_HEADA)))  // High to low transition
      {
        fast_gpio_cfg_sense_input(PIN_ENCODER_HEADA, NRF_GPIO_PIN_SENSE_HIGH);      
        if (state & (1 << PIN_ENCODER_HEADB))    // Forward vs backward
          m_motors[MOTOR_HEAD].position += RADIANS_PER_HEAD_TICK;
        else
          m_motors[MOTOR_HEAD].position -= RADIANS_PER_HEAD_TICK;      
      } else {
        fast_gpio_cfg_sense_input(PIN_ENCODER_HEADA, NRF_GPIO_PIN_SENSE_LOW);
        if (state & (1 << PIN_ENCODER_HEADB))   // Forward vs backward
          m_motors[MOTOR_HEAD].position -= RADIANS_PER_HEAD_TICK;
        else
          m_motors[MOTOR_HEAD].position += RADIANS_PER_HEAD_TICK;      
      }    
    }           
    // Lift encoder (next fastest)
    if (whatChanged & (1 << PIN_ENCODER_LIFTA))
    {
      m_motors[MOTOR_LIFT].count = count;
      if (!(state & (1 << PIN_ENCODER_LIFTA)))  // High to low transition
      {
        fast_gpio_cfg_sense_input(PIN_ENCODER_LIFTA, NRF_GPIO_PIN_SENSE_HIGH);      
        if (state & (1 << PIN_ENCODER_LIFTB))   // Forward vs backward
          m_motors[MOTOR_LIFT].position += RADIANS_PER_LIFT_TICK;
        else
          m_motors[MOTOR_LIFT].position -= RADIANS_PER_LIFT_TICK;      
      } else {
        fast_gpio_cfg_sense_input(PIN_ENCODER_LIFTA, NRF_GPIO_PIN_SENSE_LOW);
        if (state & (1 << PIN_ENCODER_LIFTB))   // Forward vs backward
          m_motors[MOTOR_LIFT].position -= RADIANS_PER_LIFT_TICK;
        else
          m_motors[MOTOR_LIFT].position += RADIANS_PER_LIFT_TICK;  
      }    
    }       
    // Left wheel
    if (whatChanged & (1 << PIN_ENCODER_LEFT))
    {
      m_motors[MOTOR_LEFT_WHEEL].count = count;
      m_motors[MOTOR_LEFT_WHEEL].position += m_motors[MOTOR_LEFT_WHEEL].unitsPerTick;
      if (!(state & (1 << PIN_ENCODER_LEFT)))  // High to low transition
        fast_gpio_cfg_sense_input(PIN_ENCODER_LEFT, NRF_GPIO_PIN_SENSE_HIGH);      
      else
        fast_gpio_cfg_sense_input(PIN_ENCODER_LEFT, NRF_GPIO_PIN_SENSE_LOW);
    }       
    // Right wheel
    if (whatChanged & (1 << PIN_ENCODER_RIGHT))
    {
      m_motors[MOTOR_RIGHT_WHEEL].count = count;
      m_motors[MOTOR_RIGHT_WHEEL].position += m_motors[MOTOR_RIGHT_WHEEL].unitsPerTick;
      if (!(state & (1 << PIN_ENCODER_RIGHT)))  // High to low transition
        fast_gpio_cfg_sense_input(PIN_ENCODER_RIGHT, NRF_GPIO_PIN_SENSE_HIGH);      
      else
        fast_gpio_cfg_sense_input(PIN_ENCODER_RIGHT, NRF_GPIO_PIN_SENSE_LOW);
    }       
  }
}

// XXX: The below hack job can be deleted once encoders are proven sound
#if 1
static u16 m_log[4096];
void encoderAnalyzer(void)
{
  u32 now, start = GetCounter() >> 3;
  u32 end = start + ((250 * COUNT_PER_MS) >> 3);
  s16 lastPins = 0xff;
  s16 logLen = 0;
  static s32 s_pos = 0;
  
  do {
    now = NRF_RTC1->COUNTER << 5;
    s16 pins = (NRF_GPIO->IN >> 16) & 0x11;  // Neck encoder pins 16 and 20
    if (pins != lastPins)
    {
      m_log[logLen++] = pins | now;
      lastPins = pins;
    }
  } while (now < end);
  
  // Print the log
  u16 last = start;
  for (int i = 0; 0 && i < logLen; i++)
  {
    UART::print("%2x %i\n", (uint8_t)(m_log[i] & 0x11), (u16)((m_log[i] & 0xffe0) - last));
    last = m_log[i] & 0xffe0;
  }
  // Scan the log to compute how many ticks (up + down)
  // 00, 01, 11, 10, 00
  const s8 dir[] = {  0, 1,-1, 0,     // 00 -> 00, 01, 10, 11
                     -1, 0, 0, 1,     // 01 -> 00, 01, 10, 11
                      1, 0, 0,-1,     // 10 -> 00, 01, 10, 11
                      0,-1, 1, 0};    // 11 -> 00, 01, 10, 11               
  int up = 0, down = 0, error = 0;
  for (int i = 1; i < logLen; i++)
  {
    int fromto = (m_log[i-1] & 1) | ((m_log[i-1] & 0x10) >> 3) |
                 ((m_log[i] & 1) << 2) | ((m_log[i] & 0x10) >> 1);
    if (dir[fromto] > 0)
      down++;
    else if (dir[fromto] < 0)
      up++;
    else
      error++;
  }
  
  s_pos += (down - up);
  UART::print("\nUp: %i  Down: %i Error: %i Entries: %i Pos: %i IRQ Pos: %i\n\n", 
    up, down, error, logLen, s_pos, m_motors[3].position);
}
#endif
