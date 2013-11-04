#ifndef SIM_ROBOT_H
#define SIM_ROBOT_H

#include "anki/common/types.h"

namespace Anki {
  
  namespace Cozmo {
    
    namespace Robot {
      
      ReturnCode Init();
      void Destroy();
      
      //
      // Stepping Functions
      //
      // We will have (at least?) two threads to step along:
      //
      // 1. The low-level functions which happen at a strict, determinstic rate
      //    (like motor control and sensor updates)
      //
      // 2. The slower functions which take longer and will be run "as quickly
      //    as possible (like the vision system for finding block markers)
      //
      ReturnCode step_MainExecution();
      ReturnCode step_LongExecution();
      
      //
      // State Machine Operation Modes
      //
      enum OperationMode {
        INITIALIZING,
        WAITING,
        DOCK,
        FOLLOW_PATH
      };
      
      OperationMode GetOperationMode();
      void SetOperationMode(OperationMode newMode);

      
      // TODO: Move this to HAL?
      //Sets an open loop speed to the two motors. The open loop speed value ranges
      //from: [0..MOTOR_PWM_MAXVAL] and HAS to be within those boundaries
      void SetOpenLoopMotorSpeed(s16 leftSpeed, s16 rightSpeed);
           

      
    } // namespace Robot
    
  } // namespace Cozmo
  
} // namespace Anki

#endif
