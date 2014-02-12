#include "anki/cozmo/robot/hal.h"
#include "movidius.h"
#include <math.h>

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static const u32 FREQUENCY_KHZ              = 20;

      static const u8 DISCONNECTED_MODE           = 7;

      static const u8 LEFT_WHEEL_FORWARD_PIN      = 3;
      static const u8 LEFT_WHEEL_FORWARD_MODE     = 3;
      static const u8 LEFT_WHEEL_BACKWARD_PIN     = 0;
      static const u8 LEFT_WHEEL_BACKWARD_MODE    = 4;
      static const u8 LEFT_WHEEL_PWM              = 0;

      static const u8 RIGHT_WHEEL_FORWARD_PIN     = 90;
      static const u8 RIGHT_WHEEL_FORWARD_MODE    = 3;
      static const u8 RIGHT_WHEEL_BACKWARD_PIN    = 81;
      static const u8 RIGHT_WHEEL_BACKWARD_MODE   = 5;
      static const u8 RIGHT_WHEEL_PWM             = 4;

      static const u8 LIFT_UP_PIN                 = 93;
      static const u8 LIFT_UP_MODE                = 1;
      static const u8 LIFT_DOWN_PIN               = 82;
      static const u8 LIFT_DOWN_MODE              = 5;
      static const u8 LIFT_PWM                    = 5;

      static const u8 GRIPPER_UP_PIN              = 80;
      static const u8 GRIPPER_UP_MODE             = 5;
      static const u8 GRIPPER_DOWN_PIN            = 83;
      static const u8 GRIPPER_DOWN_MODE           = 1;
      static const u8 GRIPPER_PWM                 = 3;

      static const u8 HEAD_UP_PIN                 = 45;
      static const u8 HEAD_UP_MODE                = 1;
      static const u8 HEAD_DOWN_PIN               = 17;
      static const u8 HEAD_DOWN_MODE              = 4;
      static const u8 HEAD_PWM                    = 1;

      struct MotorInfo
      {
        u8 pwm;
        u8 forwardUpPin;
        u8 forwardUpMode;
        u8 backwardDownPin;
        u8 backwardDownMode;
      };

      struct MotorPosition
      {
        f32 position;
        u32 lastTick;
        u32 delta;
        f32 unitsPerTick;
        u8 pin;
      };

      static const MotorInfo m_motors[] =
      {
        {
          LEFT_WHEEL_PWM,
          LEFT_WHEEL_FORWARD_PIN,
          LEFT_WHEEL_FORWARD_MODE,
          LEFT_WHEEL_BACKWARD_PIN,
          LEFT_WHEEL_BACKWARD_MODE
        },
        {
          RIGHT_WHEEL_PWM,
          RIGHT_WHEEL_FORWARD_PIN,
          RIGHT_WHEEL_FORWARD_MODE,
          RIGHT_WHEEL_BACKWARD_PIN,
          RIGHT_WHEEL_BACKWARD_MODE
        },
        {
          LIFT_PWM,
          LIFT_UP_PIN,
          LIFT_UP_MODE,
          LIFT_DOWN_PIN,
          LIFT_DOWN_MODE
        },
        {
          GRIPPER_PWM,
          GRIPPER_UP_PIN,
          GRIPPER_UP_MODE,
          GRIPPER_DOWN_PIN,
          GRIPPER_DOWN_MODE
        },
        {
          HEAD_PWM,
          HEAD_UP_PIN,
          HEAD_UP_MODE,
          HEAD_DOWN_PIN,
          HEAD_DOWN_MODE
        }
      };

      // Given a gear ratio of 91.7:1 and 125.67mm wheel circumference, we
      // compute the mm per tick as (using 4 magnets):
      static const f32 MM_PER_TICK = 125.67 / 182.0 / 4.0;

      // Given a gear ratio of 815.4:1 and 4 encoder ticks per revolution, we
      // compute the radians per tick on the lift as:
      static const f32 RADIANS_PER_LIFT_TICK = (0.5 * M_PI) / 815.4;

      // If no encoder activity for 200ms, we may as well be stopped
      static const u32 ENCODER_TIMEOUT_US = 200000;

      // Set a max speed and reject all noise above it
      static const u32 MAX_SPEED_MM_S = 500;
      static const u32 DEBOUNCE_US = (MM_PER_TICK * 1000) / MAX_SPEED_MM_S;

      // Setup the GPIO indices
      static const u32 ENCODER_COUNT = 3;
      static const u32 ENCODER_1_PIN = 106;
      static const u32 ENCODER_2_PIN = 32;
      static const u32 ENCODER_3_PIN = 35;

      // NOTE: Do NOT re-order the MotorID enum, because this depends on it
      static MotorPosition m_motorPositions[MOTOR_COUNT] =
      {
        {0, 0, 0, MM_PER_TICK, ENCODER_1_PIN},  // MOTOR_LEFT_WHEEL
        {0, 0, 0, MM_PER_TICK, ENCODER_2_PIN},  // MOTOR_RIGHT_WHEEL
        {0, 0, 0, RADIANS_PER_LIFT_TICK, ENCODER_3_PIN},  // MOTOR_LIFT
        {0}  // Zero out the rest
      };

      static const u32 m_src = D_GPIO_IRQ_SRC_0;
      static const u32 m_irq = IRQ_GPIO_0;
      static const u32 m_priority = 4;
      static const u32 m_type = POS_EDGE_INT;

      // Update the encoder values
      void MotorIRQ(u32 source)
      {
        // Only update the motors with encoder feedback
        for (int i = 0; i < ENCODER_COUNT; i++)
        {
          u32 ticks = GetMicroCounter();
          MotorPosition* motorPosition = &m_motorPositions[i];

          // Make sure not to use DrvGpioGetPin, because it changes the mode
          u32 state = DrvGpioGet(motorPosition->pin);
          if (state && (ticks - motorPosition->lastTick) > DEBOUNCE_US)
          {
            u32 mode = DrvGpioGetMode(motorPosition->pin);
            // Only switch if it was a real logic-level rising edge
            if (!(mode & D_GPIO_DATA_INV_ON))
            {
              motorPosition->delta = ticks - motorPosition->lastTick;
              motorPosition->lastTick = ticks;
              motorPosition->position += motorPosition->unitsPerTick;
            }

            DrvGpioMode(motorPosition->pin, mode ^ D_GPIO_DATA_INV_ON);
          }
        }

        DrvIcbIrqClear(m_irq);
      }

      void MotorInit()
      {
        // Enable encoder interrupts
        DrvGpioIrqConfig(ENCODER_1_PIN, m_src, m_type, m_priority, MotorIRQ);
        DrvGpioIrqConfig(ENCODER_2_PIN, m_src, m_type, m_priority, MotorIRQ);
        DrvGpioIrqConfig(ENCODER_3_PIN, m_src, m_type, m_priority, MotorIRQ);

        // Set the correct pad attributes
        u32 pad = D_GPIO_PAD_NO_PULL |
          D_GPIO_PAD_DRIVE_2mA |
          D_GPIO_PAD_VOLT_2V5 |
          D_GPIO_PAD_SLEW_SLOW |
          D_GPIO_PAD_SCHMITT_ON |
          D_GPIO_PAD_RECEIVER_ON |
          D_GPIO_PAD_BIAS_2V5 |
          D_GPIO_PAD_LOCALCTRL_OFF |
          D_GPIO_PAD_LOCALDATA_LO |
          D_GPIO_PAD_LOCAL_PIN_OUT;

        GpioPadSet(ENCODER_1_PIN, pad);
        GpioPadSet(ENCODER_2_PIN, pad);
        GpioPadSet(ENCODER_3_PIN, pad);

        for (int i = 0; i < MOTOR_COUNT; i++)
        {
          const MotorInfo* motorInfo = &m_motors[i];
          GpioPadSet(motorInfo->forwardUpPin, pad);
          GpioPadSet(motorInfo->backwardDownPin, pad);
        }
      }
 
      void MotorSetPower(MotorID motor, f32 power)
      {
        //ASSERT(motor < MOTOR_COUNT);
        
        if (power > 1.0f)
          power = 1.0f;
        else if (power < -1.0f)
          power = -1.0f;

        u32 period = (DrvCprGetSysClockKhz() / FREQUENCY_KHZ);
        u32 lowCount = period * fabs(power);
        u32 highCount = period - lowCount;

        const MotorInfo* motorInfo = &m_motors[motor];
        MotorPosition* motorPosition = &m_motorPositions[motor];

        if (power > 0)
        {
          // Disable the opposite direction from influencing the motor driver
          DrvGpioMode(motorInfo->backwardDownPin, DISCONNECTED_MODE);
          DrvGpioSetPinHi(motorInfo->backwardDownPin);
          DrvGpioMode(motorInfo->forwardUpPin, motorInfo->forwardUpMode);
          motorPosition->unitsPerTick = fabs(motorPosition->unitsPerTick);
        } else if (power < 0) {
          // Disable the opposite direction from incluencing the motor driver
          DrvGpioMode(motorInfo->forwardUpPin, DISCONNECTED_MODE);
          DrvGpioSetPinHi(motorInfo->forwardUpPin);
          DrvGpioMode(motorInfo->backwardDownPin, motorInfo->backwardDownMode);
          motorPosition->unitsPerTick = -fabs(motorPosition->unitsPerTick);
        } else {
          DrvGpioMode(motorInfo->forwardUpPin, DISCONNECTED_MODE);
          DrvGpioMode(motorInfo->backwardDownPin, DISCONNECTED_MODE);
          DrvGpioSetPinLo(motorInfo->backwardDownPin);
          DrvGpioSetPinLo(motorInfo->forwardUpPin);

          // Disable the PWM
          u32 pwmLeadInAddress = GPIO_PWM_LEADIN0_ADR + (4 * motorInfo->pwm);
          SET_REG_WORD(pwmLeadInAddress, 0);
       }

        DrvPwmConfigure(motorInfo->pwm, 0, 0, highCount, lowCount, 1);
      }

      void MotorResetPosition(MotorID motor)
      {
        m_motorPositions[motor].position = 0;
      }

      f32 MotorGetSpeed(MotorID motor)
      {
        MotorPosition* motorPosition = &m_motorPositions[motor];
        u32 howLate = GetMicroCounter() - motorPosition->lastTick;

        if (motorPosition->delta > ENCODER_TIMEOUT_US ||
            howLate > ENCODER_TIMEOUT_US)
        {
          return 0;
        }

        return (motorPosition->unitsPerTick * 1000000.0f) / motorPosition->delta;
      }

      f32 MotorGetPosition(MotorID motor)
      {
        return m_motorPositions[motor].position;
      }
    }
  }
}

