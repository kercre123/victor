#include "anki/common/robot/trig_fast.h"
#include "proxSensors.h"
#include "headController.h"
#include "liftController.h"
#include "anki/cozmo/robot/hal.h"



namespace Anki {
  namespace Cozmo {
    namespace ProxSensors {
      
      namespace {
        f32 _proxLeft, _proxFwd, _proxRight;
        bool _blockedLeft, _blockedFwd, _blockedRight;
        
        const f32 FILT_COEFF = 0.1f;
        
      } // "private" namespace
      
      
      Result Update()
      {
        Result retVal = RESULT_OK;
        
        // Get current readings and filter
        HAL::ProximityValues currProxVals;
        HAL::GetProximity(&currProxVals);
        
        _proxLeft = (FILT_COEFF * currProxVals.left) + ((1.f - FILT_COEFF) * _proxLeft);
        _proxFwd = (FILT_COEFF * currProxVals.forward) + ((1.f - FILT_COEFF) * _proxFwd);
        _proxRight = (FILT_COEFF * currProxVals.right) + ((1.f - FILT_COEFF) * _proxRight);
        
        
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
        left = MIN(static_cast<u8>(FLT_ROUND(_proxLeft)), u8_MAX);
        forward = MIN(static_cast<u8>(FLT_ROUND(_proxFwd)), u8_MAX);
        right = MIN(static_cast<u8>(FLT_ROUND(_proxRight)), u8_MAX);
        
        //PERIODIC_PRINT(200, "PROX: %f  %f  %f (%d %d %d)\n",
        //               _proxLeft, _proxFwd, _proxRight, left, forward, right);
      }


      bool IsLeftBlocked() {return _blockedLeft;}
      bool IsForwardBlocked() {return _blockedFwd;}
      bool IsRightBlocked() {return _blockedRight;}
      
      
    } // namespace ProxSensors
  } // namespace Cozmo
} // namespace Anki
