extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "nrf_gpiote.h"
  #include "nrf_sdm.h"
}

#include <limits.h>

#include "hardware.h"
#include "anki/cozmo/robot/spineData.h"

#include "battery.h"
#include "motors.h"
#include "timer.h"
#include "head.h"

#include "messages.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

struct MotorConfig
{
  // These few are constant; will refactor into separate structs again soon.
  u8 n1Pin;
  u8 n2Pin;
  u8 pPin;
  u8 isBackward;   // True if wired backward
  u8 encoderPins[2];
};

struct MotorInfo
{
  u8 lastP;        // Last value of pPin
  
  Fixed unitsPerTick;
  Fixed position;
  Fixed lastPosition;
  u32 count;
  u32 lastCount;
  
  s16 nextPWM;
  s16 oldPWM;
};

// 16 MHz timer with PWM running at 20kHz
const s16 TIMER_TICKS_END = (16000000 / 20000) - 1;

const s16 PWM_DIVISOR = SHRT_MAX / TIMER_TICKS_END;

// Encoder scaling reworked for Cozmo 4.0

// Given a gear ratio of 161.5:1 and 94mm wheel circumference and 2 ticks * 4 teeth
// for 8 encoder ticks per revolution, we compute the meters per tick as:
// Applying a slip factor correction of 94.8%
const u32 METERS_PER_TICK = TO_FIXED_0_32((0.948 * 0.125 * 0.0292 * 3.14159265359) / 173.43);

// Given a gear ratio of 172.68:1 and 4 encoder ticks per revolution, we
// compute the radians per tick on the lift as:
const Fixed RADIANS_PER_LIFT_TICK = TO_FIXED((0.25 * 3.14159265359) / 172.68);

// Given a gear ratio of 348.77:1 and 4 encoder ticks per revolution, we
// compute the radians per tick on the head as:
const Fixed RADIANS_PER_HEAD_TICK = TO_FIXED((0.25 * 3.14159265359) / 348.77);

// Pre-production hardware had 8 ticks
const Fixed RADIANS_PER_HEAD_TICK_PRE_PROD = TO_FIXED((0.125 * 3.14159265359) / 348.77);
Fixed g_radiansPerHeadTick;   // XXX: Remove this once Pilot robots are obsolete

// If no encoder activity for 200ms, we may as well be stopped
const u32 ENCODER_TIMEOUT_COUNT = 200 * COUNT_PER_MS;
const u32 ENCODER_NONE = 0xFF;

// This is a map of all motor drive pins
static const int ALL_N_MOTOR_PINS =
  PIN_LEFT_N1  | PIN_LEFT_N2 |
  PIN_RIGHT_N1 | PIN_RIGHT_N2 |
  PIN_HEAD_N1  | PIN_HEAD_N2 |
  PIN_LIFT_N1  | PIN_LIFT_N2;

static volatile bool motorDisable = true;
static bool motorCalibrate = false;

// NOTE: Do NOT re-order the MotorID enum, because this depends on it
static const MotorConfig m_config[MOTOR_COUNT] = {
  {
    PIN_LEFT_N1,
    PIN_LEFT_N2,
    PIN_LEFT_P,
    false,
    PIN_ENCODER_LEFT,
    ENCODER_NONE,
  },
  {
    PIN_RIGHT_N1,
    PIN_RIGHT_N2,
    PIN_RIGHT_P,
    false,
    PIN_ENCODER_RIGHT,
    ENCODER_NONE,
  },
  {
    PIN_LIFT_N1,
    PIN_LIFT_N2,
    PIN_LIFT_P,
    true,
    PIN_ENCODER_LIFTA,
    PIN_ENCODER_LIFTB,
  },
  {
    PIN_HEAD_N1,
    PIN_HEAD_N2,
    PIN_HEAD_P,
    true,
    PIN_ENCODER_HEADA,
    PIN_ENCODER_HEADB,
  }
};

static MotorInfo m_motors[MOTOR_COUNT] =
{
  {
    0,
    1, // units per tick = 1 tick
    0, 0, 0, 0, 0, 0
  },
  {
    0,
    1, // units per tick = 1 tick
    0, 0, 0, 0, 0, 0
  },
  {
    0,
    RADIANS_PER_LIFT_TICK,
    0, 0, 0, 0, 0, 0
  },
  {
    0,
    RADIANS_PER_HEAD_TICK,
    0, 0, 0, 0, 0, 0
  },
};

u32 m_lastState = 0;

static void ConfigureTimer(NRF_TIMER_Type* timer, const u8 taskChannel, const u8 ppiChannel)
{
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

  // Match compares
  // 0: PWM n + 0
  // 1: PWM n + 1
  // 2: timer wrap-around
  // 3: timer wrap-around
  sd_ppi_channel_assign(ppiChannel + 0, &timer->EVENTS_COMPARE[0], &NRF_GPIOTE->TASKS_OUT[taskChannel + 0]);
  sd_ppi_channel_assign(ppiChannel + 1, &timer->EVENTS_COMPARE[2], &NRF_GPIOTE->TASKS_OUT[taskChannel + 0]);
  sd_ppi_channel_assign(ppiChannel + 2, &timer->EVENTS_COMPARE[1], &NRF_GPIOTE->TASKS_OUT[taskChannel + 1]);
  sd_ppi_channel_assign(ppiChannel + 3, &timer->EVENTS_COMPARE[3], &NRF_GPIOTE->TASKS_OUT[taskChannel + 1]);

  // Start the timer
  timer->TASKS_START = 1;
}

void Motors::disable(bool disable) {
  if (motorDisable == disable) {
    return ;
  }

  // Enable motors, recalibrate encoders
  if (!disable) {
    setup();
  } else {
    teardown();
  }
}


// Returns 'true' when the the charger says the battery is full
bool Motors::getChargeOkay() {
  return motorDisable ? nrf_gpio_pin_read(PIN_nCHGOK) : false;
}


void Motors::teardown(void) {
  // Prevent manage from reconfiguring the motors
  motorDisable = true;
  
  // Stop our timers timers (they will be cleared before they start back up)
  NRF_TIMER1->TASKS_STOP = 1;
  NRF_TIMER2->TASKS_STOP = 1;

  // Disable GPIOTE tacks for toggling motor bits
  for (int motorID = 0; motorID < MOTOR_COUNT; motorID++) {
    nrf_gpiote_task_disable(motorID);
  }
  
  // Clear the drive values
  NRF_GPIO->OUTCLR = ALL_N_MOTOR_PINS;

  // Give the motors time to bleed
  MicroWait(5000);

  // Set motor pins to floating
  for (int motorID = 0; motorID < MOTOR_COUNT; motorID++) {
    const MotorConfig* motorConfig = &m_config[motorID];

    nrf_gpio_cfg_default(motorConfig->n1Pin);
    nrf_gpio_cfg_default(motorConfig->n2Pin);
    nrf_gpio_cfg_default(motorConfig->pPin);
  }

  // Enable charge logic
  nrf_gpio_cfg_input(PIN_nCHGOK, NRF_GPIO_PIN_PULLUP);
}

void Motors::setup(void) {
  // Float our charge pin
  nrf_gpio_cfg_default(PIN_nCHGOK);

  // Clear our motor pins just for safety's sake (default low / stopped)
  NRF_GPIO->OUTCLR = ALL_N_MOTOR_PINS;

  // Configure each motor pin as an output
  for (int motorID = 0; motorID < MOTOR_COUNT; motorID++)
  {
    // Configure the pins for the motor bridge to be outputs
    const MotorConfig* motorConfig = &m_config[motorID];

    nrf_gpiote_task_disable(motorID);
    nrf_gpio_cfg_output(motorConfig->n1Pin);
    nrf_gpio_cfg_output(motorConfig->n2Pin);
    nrf_gpio_cfg_output(motorConfig->pPin);
  }
    
  // Motors need to be recalibrated now
  motorCalibrate = true;

  // It is now officially safe for manage to run again
  motorDisable = false;

  // Manage will setup GPIOTE, Pins, and timers
  // Next manage will reconfigure GPIOTE / pin states
  manage();
}

// Reset Nordic tasks to allow less glitchy changes.
// Without a reset, the polarity will become permanently inverted.
static void ConfigureTask(u8 motorID, volatile u32 *timer)
{
  const MotorConfig* motorConfig = &m_config[motorID];
  MotorInfo* motorInfo = &m_motors[motorID];

  // Zero
  if (motorInfo->nextPWM == 0) {
    nrf_gpiote_task_disable(motorID);
    nrf_gpio_pin_clear(motorConfig->n1Pin);
    nrf_gpio_pin_clear(motorConfig->n2Pin);
  
  // Forward
  } else if ((motorInfo->nextPWM > 0) != motorConfig->isBackward)
  {
    // Drive P2+N1
    nrf_gpiote_task_disable(motorID);
    nrf_gpio_pin_clear(motorConfig->n1Pin);
    nrf_gpio_pin_clear(motorConfig->n2Pin);
    nrf_gpio_pin_clear(motorConfig->pPin);    // P=0 is P2+N1
    
    // XXX: If P drive doesn't match, wait until next update to start motor - to workaround 2.1 hardware glitch
    if (0 != motorInfo->lastP)
    {
      motorInfo->lastP = 0;
      return;   // Don't update oldPWM, so we come in here again
    } else {
      nrf_gpiote_task_configure(motorID, motorConfig->n1Pin,
        NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
      nrf_gpiote_task_enable(motorID);
    }

  // Backward
  } else {
    if (motorInfo->unitsPerTick > 0)
      motorInfo->unitsPerTick = -motorInfo->unitsPerTick;
        
    // Drive P1+N2
    nrf_gpiote_task_disable(motorID);
    nrf_gpio_pin_clear(motorConfig->n1Pin);
    nrf_gpio_pin_clear(motorConfig->n2Pin);
    nrf_gpio_pin_set(motorConfig->pPin);      // P=1 is P1+N2
    
    // XXX: If P drive doesn't match, wait until next update to start motor - to workaround 2.1 hardware glitch
    if (1 != motorInfo->lastP)
    {
      motorInfo->lastP = 1;
      return;   // Don't update oldPWM, so we come in here again
    } else {
      nrf_gpiote_task_configure(motorID, motorConfig->n2Pin,
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
  // NOTE: Motors are not actually enabled, this only stages them
  
  // Configure TIMER1 and TIMER2 with the appropriate task and PPI channels
  ConfigureTimer(NRF_TIMER1, 0, 0);
  ConfigureTimer(NRF_TIMER2, 2, 4);
  
  // Setup head encoder tick rate for production or pre-production hardware
  g_radiansPerHeadTick = RADIANS_PER_HEAD_TICK;
  if (BODY_VER < BODY_VER_PROD)
    g_radiansPerHeadTick = RADIANS_PER_HEAD_TICK_PRE_PROD;

  // Enable PPI channels for timer PWM and reset
  sd_ppi_channel_enable_set(0xFF);

  // Clear pending interrupts and enable the GPIOTE interrupt
  sd_nvic_ClearPendingIRQ(GPIOTE_IRQn);
  sd_nvic_SetPriority(GPIOTE_IRQn, ENCODER_PRIORITY);
  sd_nvic_EnableIRQ(GPIOTE_IRQn);

  // Clear all GPIOTE interrupts
  NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;
    
  // Clear pending events
  NRF_GPIOTE->EVENTS_PORT = 0;
  
  // Get the current state of the input pins
  u32 state = NRF_GPIO->IN;
  
  // Enable interrupt on port event
  NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
  
  // Motors start disabled, nCHGOK is configured for input
  teardown();
  
  // Configure all encoder pins
  for (int i = 0; i < MOTOR_COUNT; i++)
  {
    const MotorConfig* motorConfig = &m_config[i];

    // Enable sensing for each encoder pin (only one per quadrature encoder)
    const u8 pin = motorConfig->encoderPins[0];
    const u32 mask = 1 << pin;
      
    // Configure initial pin sensing (used for inversion with whack-a-mole)
    if (state & mask)
    {
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);
      m_lastState |= mask;
    } else {
      nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
    }

    if (motorConfig->encoderPins[1] != ENCODER_NONE)
    {
      nrf_gpio_cfg_input(motorConfig->encoderPins[1], NRF_GPIO_PIN_NOPULL);
    }
  }
}

static s16 limitPower(u8 motorID, const int speed_scale[], const int base_speed[]) {
  MotorInfo* motorInfo = &m_motors[motorID];

  int current_speed = ABS((int)motorInfo->position - (int)motorInfo->lastPosition);
  int64_t speed_offset = (int64_t)current_speed *  (int64_t)speed_scale[motorID];
  int inst_speed = base_speed[motorID] + (int)(speed_offset >> 16);

  // Clamp max power to a rational value (fix underflow)
  if (inst_speed > 0x7FFF || inst_speed < 0) { inst_speed = 0x7FFF; }

  return inst_speed;
}

void Motors::setPower(u8 motorID, s16 power)
{
  // Setup burnout protection
  static s16 avg_max_power[MOTOR_COUNT];
  int inst_speed;
  
  if (Battery::onContacts) {
    // Eventually we should do a current draw calculation across all motors
    // to allow animations driving slower to work better
    
    static const int base_speed[MOTOR_COUNT] = {
      0x1000,
      0x1000,
       0x800,
      0x1000,
    };

    static const int speed_scale[MOTOR_COUNT] = {
      TO_FIXED(0x4000),
      TO_FIXED(0x4000),
      TO_FIXED(0x4000),
      TO_FIXED(0x2000)
    };

    inst_speed = limitPower(motorID, speed_scale, base_speed);
  } else {
    /*
    static const int base_speed[MOTOR_COUNT] = { 
      0x3FFF, 
      0x3FFF,
      0x1FFF,
      0x7FFF
    };
    static const int speed_scale[MOTOR_COUNT] = { 
      TO_FIXED(0x4000),
      TO_FIXED(0x4000),
      TO_FIXED(0x2000),
      0
    };

    inst_speed = limitPower(motorID, speed_scale, base_speed);
    */
    inst_speed = 0x7FFF;
  }

  avg_max_power[motorID] = (s16)((inst_speed + avg_max_power[motorID] * 15) / 16);
  
  s16 max_power = avg_max_power[motorID];
    
  // Don't let power exceed safe value
  if (ABS(power) > max_power) {
    power = (power > 0) ? max_power : -max_power;
  }

  // Scale from [0, SHRT_MAX] to [0, TIMER_TICKS_END-1]
  power /= PWM_DIVISOR;

  // Clamp the PWM power
  if (power >= TIMER_TICKS_END)
    power = TIMER_TICKS_END - 1;
  else if (power <= -TIMER_TICKS_END)
    power = -TIMER_TICKS_END + 1;

  // Store the PWM value for the MotorsUpdate() and keep the sign
  m_motors[motorID].nextPWM = power;
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

void Motors::manage()
{
  // Update the SPI data structure to send data back to the head
  g_dataToHead.speeds[0] = FIXED_MUL(Motors::getSpeed(0), METERS_PER_TICK);
  g_dataToHead.speeds[1] = FIXED_MUL(Motors::getSpeed(1), METERS_PER_TICK);
  g_dataToHead.speeds[2] = Motors::getSpeed(2);
  g_dataToHead.speeds[3] = Motors::getSpeed(3);
  
  g_dataToHead.positions[0] = FIXED_MUL(m_motors[0].position, METERS_PER_TICK);
  g_dataToHead.positions[1] = FIXED_MUL(m_motors[1].position, METERS_PER_TICK);
  g_dataToHead.positions[2] = m_motors[2].position;
  g_dataToHead.positions[3] = m_motors[3].position;

  // Update our power settings
  if (!Head::spokenTo || motorDisable) {
    // Head not spoken to, motors are disabled, set power to 0
    for (int i = 0; i < MOTOR_COUNT; i++) {
      Motors::setPower(i, 0);
    }
    return ;
  }

  if (motorCalibrate) {
    using namespace Anki::Cozmo::RobotInterface;
  
    StartMotorCalibration msg;
    msg.calibrateHead = true;
    msg.calibrateLift = true;
    SendMessage(msg);
    
    motorCalibrate = false;
  }

  // Copy (valid) data to update motors
  for (int i = 0; i < MOTOR_COUNT; i++) {
    Motors::setPower(i, g_dataToBody.motorPWM[i]);
  }

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
}

// Encoder code must be optimized for speed - this macro is faster than Nordic's
#define fast_gpio_cfg_sense_input(pin_number, sense_config) \
  NRF_GPIO->PIN_CNF[pin_number] = (sense_config << GPIO_PIN_CNF_SENSE_Pos) \
                                | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) \
                                | (NRF_GPIO_PIN_NOPULL << GPIO_PIN_CNF_PULL_Pos) \
                                | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) \
                                | (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos)

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
          m_motors[MOTOR_HEAD].position += g_radiansPerHeadTick;
        else
          m_motors[MOTOR_HEAD].position -= g_radiansPerHeadTick;
      } else {
        fast_gpio_cfg_sense_input(PIN_ENCODER_HEADA, NRF_GPIO_PIN_SENSE_LOW);
        if (state & (1 << PIN_ENCODER_HEADB))   // Forward vs backward
          m_motors[MOTOR_HEAD].position -= g_radiansPerHeadTick;
        else
          m_motors[MOTOR_HEAD].position += g_radiansPerHeadTick;      
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
