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
                                   StartHeadNod.speed,
                                   StartHeadNod.accel,
                                   0);
      break;
    }
      
    case KeyFrame::SET_LED_COLORS:
    {
      // TODO: Move this into some kind of LightController file/namespace
      for(s32 iLED=0; iLED<HAL::NUM_LEDS; ++iLED) {
        const u32& currLEDcolor = SetLEDcolors.led[iLED];
        if(currLEDcolor != KeyFrame::UNSPECIFIED_COLOR) {
          HAL::SetLED(static_cast<HAL::LEDId>(iLED), currLEDcolor);
        }
      }
      break;
    }
      
  } // switch(currKeyFrame.type)
  
} // TransitionOutOf()


void KeyFrame::TransitionInto(const TimeStamp_t animStartTime_us) const
{
  switch(type)
  {
    case KeyFrame::STOP_HEAD_NOD:
    {
      HeadController::StopNodding();
      break;
    }
      
    case KeyFrame::HEAD_ANGLE:
    {
      HeadController::SetAngularVelocity(SetHeadAngle.targetSpeed);
      HeadController::SetDesiredAngle(SetHeadAngle.targetAngle);
      
      // TODO: Switch to method that takes desired time into account:
      /*
      HeadController::SetDesiredAngleAndTime(SetHeadAngle.targetAngle,
                                             SetHeadAngle.targetSpeed,
                                             (relTime_ms + animStartTime_ms)*1000);
       */
      break;
    }
      
    case KeyFrame::LIFT_ANGLE:
    {
      
      LiftController::SetAngularVelocity(SetLiftHeight.targetSpeed);
      LiftController::SetDesiredHeight(SetLiftHeight.targetHeight);
      
      // TODO: Switch to method that takes desired time into account:
      /*
      LiftController::SetDesiredHeightAndTime(SetLiftHeight.targetHeight,
                                              SetLiftHeight.targetSpeed,
                                              (relTime_ms + animStartTime_ms)*1000);
       */
      break;
    }
      
  } // switch(nextKeyFrame.type)
  
} // TransitionInto()
  
} // namespace Cozmo
} // namespace Anki

