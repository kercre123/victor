#include "anki/cozmo/robot/hal.h"
#include <limits.h>

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      extern volatile GlobalDataToHead g_dataToHead;
      extern volatile GlobalDataToBody g_dataToBody;
      
      void MotorSetPower(MotorID motor, f32 power)
      {
        // ASSERT(motor < MOTOR_COUNT);
        
        // Change the data for the next SPI packet being sent to the body
        if (power > 1.0f)
          power = 1.0f;
        else if (power < -1.0f)
          power = -1.0f;
        
        g_dataToBody.motorPWM[motor] = (s16)(power * (f32)SHRT_MAX);
      }
      
      void MotorResetPosition(MotorID motor)
      {
        // ASSERT(motor < MOTOR_COUNT);
      }
      
      f32 MotorGetSpeed(MotorID motor)
      {
        // ASSERT(motor < MOTOR_COUNT);
        return 0;
      }
      
      f32 MotorGetPosition(MotorID motor)
      {
        // ASSERT(motor < MOTOR_COUNT);
        return 0;
      }
      
      s32 MotorGetLoad()
      {
        return 0;
      }
    }
  }
}
