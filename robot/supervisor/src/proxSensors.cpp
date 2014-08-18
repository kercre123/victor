#include "anki/common/robot/trig_fast.h"
#include "proxSensors.h"
#include "headController.h"
#include "liftController.h"
#include "anki/cozmo/robot/hal.h"



namespace Anki {
  namespace Cozmo {
    namespace ProxSensors {
      
      namespace {
        HAL::ProximityValues _proxVals;
        bool _blockedLeft, _blockedFwd, _blockedRight;
        
      } // "private" namespace
      
      
      Result Update()
      {
        Result retVal = RESULT_OK;
        
        HAL::GetProximity(&_proxVals);
        //PERIODIC_PRINT(200, "PROX: %d  %d  %d\n",
        //               _proxVals.left, _proxVals.forward, _proxVals.right);
        
        
        
        // TODO: Logic for when proximity sensors are blocked by the lift.
        //       Eventually, this should be computed from actual geometry,
        //       but for now just have a few very conservative cases.
        
        // If lift is in low carry position all readings always valid since
        // nothing can block the sensors.
        // If lift is in carry position, assume readings below a certain
        // head angle are valid.
        // If lift is any other position, assume all readings are invalid.
        
        if (LiftController::GetDesiredHeight() == LIFT_HEIGHT_LOWDOCK) {
          _blockedLeft = _blockedFwd = _blockedRight = false;
        } else if (LiftController::GetDesiredHeight() == LIFT_HEIGHT_CARRY) {
          
          f32 headAngle = HeadController::GetAngleRad();
          
          // Check when forward sensor is valid
          if (headAngle < 0.1f || headAngle > 0.5f) {
            _blockedFwd = false;
          } else {
            _blockedFwd = true;
          }
          
          // Check when side sensors are valid
          if (headAngle < 0.3f) {
            _blockedLeft = _blockedRight = false;
          } else {
            _blockedLeft = _blockedRight = true;
          }
          
        } else {
          _blockedLeft = _blockedFwd = _blockedRight = true;
        }

        
        return retVal;
        
      } // Update()
      
    
      
      // Returns the proximity sensor values
      void GetValues(u8 &left, u8 &forward, u8 &right)
      {
        left = MIN(_proxVals.left, u8_MAX);
        forward = MIN(_proxVals.forward, u8_MAX);
        right = MIN(_proxVals.right, u8_MAX);
      }


      bool IsLeftBlocked() {return _blockedLeft;}
      bool IsForwardBlocked() {return _blockedFwd;}
      bool IsRightBlocked() {return _blockedRight;}
      
      
    } // namespace ProxSensors
  } // namespace Cozmo
} // namespace Anki
