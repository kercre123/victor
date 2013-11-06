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

      static const u8 LEFT_WHEEL_FORWARD_PIN      = 0;
      static const u8 LEFT_WHEEL_FORWARD_MODE     = 4;
      static const u8 LEFT_WHEEL_BACKWARD_PIN     = 3;
      static const u8 LEFT_WHEEL_BACKWARD_MODE    = 3;
      static const u8 LEFT_WHEEL_PWM              = 0;

      static const u8 RIGHT_WHEEL_FORWARD_PIN     = 81;
      static const u8 RIGHT_WHEEL_FORWARD_MODE    = 5;
      static const u8 RIGHT_WHEEL_BACKWARD_PIN    = 90;
      static const u8 RIGHT_WHEEL_BACKWARD_MODE   = 3;
      static const u8 RIGHT_WHEEL_PWM             = 4;

      static const u8 LIFT_UP_PIN                 = 82;
      static const u8 LIFT_UP_MODE                = 5;
      static const u8 LIFT_DOWN_PIN               = 93;
      static const u8 LIFT_DOWN_MODE              = 1;
      static const u8 LIFT_PWM                    = 5;

      static const u8 GRIPPER_UP_PIN              = 80;
      static const u8 GRIPPER_UP_MODE             = 5;
      static const u8 GRIPPER_DOWN_PIN            = 83;
      static const u8 GRIPPER_DOWN_MODE           = 1;
      static const u8 GRIPPER_PWM                 = 3;

      static const u8 HEAD_UP_PIN                 = 17;
      static const u8 HEAD_UP_MODE                = 4;
      static const u8 HEAD_DOWN_PIN               = 45;
      static const u8 HEAD_DOWN_MODE              = 1;
      static const u8 HEAD_PWM                    = 1;

      struct MotorInfo
      {
        u8 pwm;
        u8 forwardUpPin;
        u8 forwardUpMode;
        u8 backwardDownPin;
        u8 backwardDownMode;
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

      static f32 m_motorSpeeds[MOTOR_COUNT] = {0};
      static f32 m_motorPositions[MOTOR_COUNT] = {0};

      static const u32 m_encoder1Pin = 106;
      static const u32 m_encoder2Pin = 32;
      static const u32 m_encoder3Pin = 35;

      static const u32 m_src = D_GPIO_IRQ_SRC_0;
      static const u32 m_irq = IRQ_GPIO_0;
      static const u32 m_priority = 5;
      static const u32 m_type = POS_EDGE_INT;

      static u64 m_encoder1LastTick = 0;
      static u64 m_encoder2LastTick = 0;
      static u64 m_encoder3LastTick = 0;
      static u64 m_encoder1Difference = 0;
      static u64 m_encoder2Difference = 0;
      static u64 m_encoder3Difference = 0;

      // Update the encoder values
      void MotorIRQ(u32 source)
      {
        u8 state1 = DrvGpioGetPin(m_encoder1Pin);
        u8 state2 = DrvGpioGetPin(m_encoder2Pin);
        u8 state3 = DrvGpioGetPin(m_encoder3Pin);

        u32 mode1 = DrvGpioGetMode(m_encoder1Pin);
        u32 mode2 = DrvGpioGetMode(m_encoder2Pin);
        u32 mode3 = DrvGpioGetMode(m_encoder3Pin);

        if (state1)
        {
          // Only switch if it was a real logic-level rising edge
          if (!(mode1 & D_GPIO_DATA_INV_ON))
          {
            u64 ticks = GetMicroCounter();
            m_encoder1Difference = ticks - m_encoder1LastTick;
            m_encoder1LastTick = ticks;
          }

          DrvGpioMode(m_encoder1Pin, mode1 ^ D_GPIO_DATA_INV_ON);
        }
        if (state2)
        {
          if (!(mode2 & D_GPIO_DATA_INV_ON))
          {
            u64 ticks = GetMicroCounter();
            m_encoder2Difference = ticks - m_encoder2LastTick;
            m_encoder2LastTick = ticks;
          }

          DrvGpioMode(m_encoder2Pin, mode2 ^ D_GPIO_DATA_INV_ON);
        }
        if (state3)
        {
          if (!(mode3 & D_GPIO_DATA_INV_ON))
          {
            u64 ticks = GetMicroCounter();
            m_encoder3Difference = ticks - m_encoder3LastTick;
            m_encoder3LastTick = ticks;
          }

          DrvGpioMode(m_encoder3Pin, mode3 ^ D_GPIO_DATA_INV_ON);
        }

        DrvIcbIrqClear(m_irq);
      }

      void MotorInit()
      {
        DrvGpioIrqConfig(m_encoder1Pin, m_src, m_type, m_priority, MotorIRQ);
        DrvGpioIrqConfig(m_encoder2Pin, m_src, m_type, m_priority, MotorIRQ);
        DrvGpioIrqConfig(m_encoder3Pin, m_src, m_type, m_priority, MotorIRQ);

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

        GpioPadSet(m_encoder1Pin, pad);
        GpioPadSet(m_encoder2Pin, pad);
        GpioPadSet(m_encoder3Pin, pad);

        u64 ticks = GetMicroCounter();
        m_encoder1LastTick = m_encoder2LastTick = m_encoder3LastTick = ticks;

        u32 selection = m_encoder1Pin |
          (m_encoder2Pin << 8) |
          (m_encoder3Pin << 16);

        SET_REG_WORD(GPIO_PWM_SEL0_ADR, selection);
        SET_REG_WORD(GPIO_PWM_CLR_ADR, 7);  // Clear the 3 encoders
        SET_REG_WORD(GPIO_PWM_EN_ADR, 7);   // Enable the 3 encoders
      }
 
      void MotorSetPower(MotorID motor, f32 power)
      {
        //ASSERT(motor < MOTOR_COUNT);

        if (power > 1.0f)
          power = 1.0f;
        else if (power < -1.0f)
          power = -1.0f;

        u32 period = (DrvCprGetSysClockKhz() / FREQUENCY_KHZ);
        u32 highCount = period * fabs(power);
        u32 lowCount = period - highCount;
        
        const MotorInfo* motorInfo = &m_motors[motor];

        // Disable the PWM
        u32 pwmLeadInAddress = GPIO_PWM_LEADIN0_ADR + (4 * motorInfo->pwm);
        SET_REG_WORD(pwmLeadInAddress, 0);

        if (power > 0)
        {
          // Disable the opposite direction from influencing the motor driver
          DrvGpioMode(motorInfo->backwardDownPin, DISCONNECTED_MODE);
          DrvGpioSetPinLo(motorInfo->backwardDownPin);
          DrvGpioMode(motorInfo->forwardUpPin, motorInfo->forwardUpMode);
        } else if (power < 0) {
          // Disable the opposite direction from incluencing the motor driver
          DrvGpioMode(motorInfo->forwardUpPin, DISCONNECTED_MODE);
          DrvGpioSetPinLo(motorInfo->forwardUpPin);
          DrvGpioMode(motorInfo->backwardDownPin, motorInfo->backwardDownMode);
        } else {
          DrvGpioMode(motorInfo->forwardUpPin, DISCONNECTED_MODE);
          DrvGpioMode(motorInfo->backwardDownPin, DISCONNECTED_MODE);
          DrvGpioSetPinHi(motorInfo->backwardDownPin);
          DrvGpioSetPinHi(motorInfo->forwardUpPin);
        }

        DrvPwmConfigure(motorInfo->pwm, 0, 0, highCount, lowCount, 1);
      }

      void MotorResetPosition(MotorID motor)
      {
        //ASSERT(motor < MOTOR_COUNT);

        m_motorPositions[motor] = 0;
        m_motorSpeeds[motor] = 0;
      }

      f32 MotorGetSpeed(MotorID motor)
      {
        //ASSERT(motor < MOTOR_COUNT);

        return m_motorSpeeds[motor];
      }

      f32 MotorGetPosition(MotorID motor)
      {
        //ASSERT(motor < MOTOR_COUNT);

        return m_motorPositions[motor];
      }
    }
  }
}

