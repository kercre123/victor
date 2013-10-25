#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static const u32 FREQUENCY                  = 20000;

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

      // TODO: Implement real calculations for rad_per_sec
      void SetLeftWheelAngularVelocity(f32 rad_per_sec)
      {
        u32 period = (DrvCprGetSysClockKhz() * 1000 / FREQUENCY);
        u32 highCount = period * rad_per_sec;
        u32 lowCount = period - highCount;
        DrvPwmConfigure(LEFT_WHEEL_PWM, 0, 0, highCount, lowCount, 1);
        if (rad_per_sec > 0)
        {
          DrvGpioMode(LEFT_WHEEL_BACKWARD_PIN, DISCONNECTED_MODE);
          DrvGpioMode(LEFT_WHEEL_FORWARD_PIN, LEFT_WHEEL_FORWARD_MODE);
        } else if (rad_per_sec < 0) {
          DrvGpioMode(LEFT_WHEEL_FORWARD_PIN, DISCONNECTED_MODE);
          DrvGpioMode(LEFT_WHEEL_BACKWARD_PIN, LEFT_WHEEL_BACKWARD_MODE);
        } else {
          DrvGpioMode(LEFT_WHEEL_FORWARD_PIN, DISCONNECTED_MODE);
          DrvGpioMode(LEFT_WHEEL_BACKWARD_PIN, DISCONNECTED_MODE);

          u32 pwmLeadInAddr = GPIO_PWM_LEADIN0_ADR + (4 * LEFT_WHEEL_PWM);
          SET_REG_WORD(pwmLeadInAddr, 0);
        }
      }

      void SetRightWheelAngularVelocity(f32 rad_per_sec)
      {
        u32 period = (DrvCprGetSysClockKhz() * 1000 / FREQUENCY);
        u32 highCount = period * rad_per_sec;
        u32 lowCount = period - highCount;
        DrvPwmConfigure(RIGHT_WHEEL_PWM, 0, 0, highCount, lowCount, 1);
        if (rad_per_sec > 0)
        {
          DrvGpioMode(RIGHT_WHEEL_BACKWARD_PIN, DISCONNECTED_MODE);
          DrvGpioMode(RIGHT_WHEEL_FORWARD_PIN, RIGHT_WHEEL_FORWARD_MODE);
        } else if (rad_per_sec < 0) {
          DrvGpioMode(RIGHT_WHEEL_FORWARD_PIN, DISCONNECTED_MODE);
          DrvGpioMode(RIGHT_WHEEL_BACKWARD_PIN, RIGHT_WHEEL_BACKWARD_MODE);
        } else {
          DrvGpioMode(RIGHT_WHEEL_FORWARD_PIN, DISCONNECTED_MODE);
          DrvGpioMode(RIGHT_WHEEL_BACKWARD_PIN, DISCONNECTED_MODE);
 
          u32 pwmLeadInAddr = GPIO_PWM_LEADIN0_ADR + (4 * RIGHT_WHEEL_PWM);
          SET_REG_WORD(pwmLeadInAddr, 0);
        }
      }

      void SetWheelAngularVelocity(f32 left_rad_per_sec, f32 right_rad_per_sec)
      {
        SetLeftWheelAngularVelocity(left_rad_per_sec);
        SetRightWheelAngularVelocity(right_rad_per_sec);
      }
    }
  }
}

