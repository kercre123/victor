/**
 * File: keyFrame.cpp
 *
 * Author: Andrew Stein
 * Created: 10/16/2014
 *
 * Description:
 *
 *   Defines a KeyFrame for animations.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "keyFrame.h"
#include "headController.h"
#include "liftController.h"
#include "steeringController.h"

namespace Anki {
namespace Cozmo {
  
  void KeyFrame::TransitionOutOf(const TimeStamp_t animStartTime_us) const
  {
    switch(type)
    {
      case KeyFrame::START_HEAD_NOD:
      {
        HeadController::StartNodding(StartHeadNod.lowAngle,
                                     StartHeadNod.highAngle,
                                     StartHeadNod.period_ms,
                                     0);
        break;
      }
        
      case KeyFrame::STOP_HEAD_NOD:
      {
        HeadController::StopNodding();
        //HeadController::SetDesiredAngle(StopHeadNod.finalAngle);
        break;
      }
        
      case KeyFrame::START_LIFT_NOD:
      {
        LiftController::StartNodding(StartLiftNod.lowHeight,
                                     StartLiftNod.highHeight,
                                     StartLiftNod.period_ms,
                                     0);
        break;
      }
        
      case KeyFrame::STOP_LIFT_NOD:
      {
        LiftController::StopNodding();
        break;
      }
        
      case KeyFrame::SET_LED_COLORS:
      {
        // TODO: Move this into some kind of LightController file/namespace
        for(s32 iLED=0; iLED<NUM_LEDS; ++iLED) {
          const u32& currLEDcolor = SetLEDcolors.led[iLED];
          if(currLEDcolor != KeyFrame::UNSPECIFIED_COLOR) {
            HAL::SetLED(static_cast<LEDId>(iLED), currLEDcolor);
          }
        }
        break;
      }
        
      case KeyFrame::POINT_TURN:
      {
        SteeringController::ExecuteDirectDrive(0.f, 0.f);
        break;
      }
        
      case KeyFrame::DRIVE_LINE_SEGMENT:
      {
        SteeringController::ExecuteDirectDrive(0.f, 0.f);
        break;
      }
        
      default:
      {
        // Do nothing if no TransitionOutOf behavior defined for this type
        break;
      }
        
    } // switch(type)
    
  } // TransitionOutOf()
  
  
  void KeyFrame::TransitionInto(const TimeStamp_t animStartTime_ms) const
  {
    switch(type)
    {
      case KeyFrame::HEAD_ANGLE:
      {
        const f32 dt_ms = animStartTime_ms + relTime_ms - HAL::GetTimeStamp();
        const f32 dAngle = SetHeadAngle.targetAngle - HeadController::GetAngleRad();
        HeadController::SetAngularVelocity((dAngle*1000.f) / dt_ms);
        HeadController::SetDesiredAngle(SetHeadAngle.targetAngle);
        
        // TODO: Switch to method that takes desired time into account:
        /*
         HeadController::SetDesiredAngleAndTime(SetHeadAngle.targetAngle,
         SetHeadAngle.targetSpeed,
         (relTime_ms + animStartTime_ms)*1000);
         */
        break;
      }
        
      case KeyFrame::LIFT_HEIGHT:
      {
        const f32 dt_ms = animStartTime_ms + relTime_ms - HAL::GetTimeStamp();
        const f32 dHeight = SetLiftHeight.targetHeight - LiftController::GetHeightMM();
        LiftController::SetLinearVelocity((dHeight*1000.f) / dt_ms);
        LiftController::SetDesiredHeight(SetLiftHeight.targetHeight);
        
        // TODO: Switch to method that takes desired time into account:
        /*
         LiftController::SetDesiredHeightAndTime(SetLiftHeight.targetHeight,
         SetLiftHeight.targetSpeed,
         (relTime_ms + animStartTime_ms)*1000);
         */
        break;
      }
        
      case KeyFrame::POINT_TURN:
      {
        // TODO: Switch to new method that accepts duration and computes the right velocity profile
        const f32 duration_ms = animStartTime_ms + relTime_ms - HAL::GetTimeStamp();
        const f32 wheelSpeed = (TurnInPlace.relativeAngle * WHEEL_DIST_MM * 1000.f) / (duration_ms * 2.f);
        //const f32 wheelSpeed = (TurnInPlace.relativeAngle < 0 ? -TurnInPlace.targetSpeed : TurnInPlace.targetSpeed);
        SteeringController::ExecuteDirectDrive(wheelSpeed, -wheelSpeed);
        
        break;
      }
        
      case KeyFrame::DRIVE_LINE_SEGMENT:
      {
        // TODO: Switch to new method that accepts duration and computes the right velocity profile
        const f32 duration_ms = animStartTime_ms + relTime_ms - HAL::GetTimeStamp();
        const f32 wheelSpeed = (DriveLineSegment.relativeDistance * 1000.f) / duration_ms;
        SteeringController::ExecuteDirectDrive(wheelSpeed, wheelSpeed);
        break;
      }
        
      default:
      {
        // Do nothing if no TransitionInto behavior defined for this type
        break;
      }
        
    } // switch(type)
    
  } // TransitionInto()
  
  bool KeyFrame::IsInPosition()
  {
    switch(type)
    {
      case KeyFrame::HEAD_ANGLE:
        return HeadController::IsInPosition();
        
      case KeyFrame::LIFT_HEIGHT:
        return LiftController::IsInPosition();
        
      default:
        return true;
        
    } // switch(type)
    
  } // IsInPosition()
  
  
  void KeyFrame::Stop()
  {
    switch(type)
    {
      case KeyFrame::START_HEAD_NOD:
      {
        HeadController::StopNodding();
        break;
      }
        
      case KeyFrame::START_LIFT_NOD:
      {
        LiftController::StopNodding();
        break;
      }
        
      case KeyFrame::HEAD_ANGLE:
      {
        HeadController::SetAngularVelocity(0.f);
        break;
      }
        
      case KeyFrame::LIFT_HEIGHT:
      {
        LiftController::SetAngularVelocity(0.f);
        break;
      }
        
      case KeyFrame::POINT_TURN:
      {
        SteeringController::ExecuteDirectDrive(0.f, 0.f);
        break;
      }
        
      case KeyFrame::DRIVE_LINE_SEGMENT:
      {
        SteeringController::ExecuteDirectDrive(0.f, 0.f);
        break;
      }
        
      default:
      {
        // No stopping function defined
        break;
      }
        
    } // switch(type)
  }
  
} // namespace Cozmo
} // namespace Anki

