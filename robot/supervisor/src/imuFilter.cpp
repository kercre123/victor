//#include "anki/common/robot/config.h"
#include "anki/common/robot/trig_fast.h"
//#include "anki/common/robot/geometry.h"

#include "imuFilter.h"
#include "headController.h"
#include "anki/cozmo/robot/hal.h"
//#include "anki/cozmo/robot/cozmoConfig.h"

namespace Anki {
  namespace Cozmo {
    namespace IMUFilter {
      
      namespace {
        
        // Orientation and speed in XY-plane (i.e. horizontal plane) of robot
        Radians orientation_ = 0;   // radians
        f32 rotationSpeed_ = 0; // rad/s
        
      } // "private" namespace
      
      
      void Reset()
      {
        orientation_ = 0;
        rotationSpeed_ = 0;
      }
      
      ReturnCode Update()
      {
        ReturnCode retVal = EXIT_FAILURE;
      
        f32 headAngle = HeadController::GetAngleRad();
        
        
        
        
        
        return retVal;
        
      } // Update()
      
      
      Radians GetOrientation()
      {
        return orientation_;
      }
      

    } // namespace IMUFilter
  } // namespace Cozmo
} // namespace Anki
