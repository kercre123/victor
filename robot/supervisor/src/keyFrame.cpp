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

#include "animationController.h"
#include "eyeController.h"
#include "headController.h"
#include "liftController.h"
#include "steeringController.h"

#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities.h"

namespace Anki {
namespace Cozmo {
  
//  static const u8 MIN_TRANSITION_PERCENT = 1;
//  
//  Result StreamedKeyFrame::SetFrom(const Messages::AddAnimKeyFrame_SetHeadAngle &msg)
//  {
//    
//    return RESULT_OK;
//  }
//  
//  static inline f32 GetAngleRad(s32 angle_deg)
//  {
//    return DEG_TO_RAD(static_cast<f32>(angle_deg));
//  }
//  
//  static inline f32 GetTransitionFraction(const u8 transitionPercent)
//  {
//    return 0.01f*static_cast<f32>(MAX(MIN_TRANSITION_PERCENT, transitionPercent));
//  }
//  
//  
//  void KeyFrame::TransitionOutOf(const TimeStamp_t animStartTime_us, const u8 nextTransitionIn) const
//  {
//    switch(type)
//    {
//      case KeyFrame::START_HEAD_NOD:
//      {
//        HeadController::StartNodding(GetAngleRad(StartHeadNod.lowAngle_deg),
//                                     GetAngleRad(StartHeadNod.highAngle_deg),
//                                     StartHeadNod.period_ms,
//                                     0,
//                                     GetTransitionFraction(transitionOut),
//                                     GetTransitionFraction(nextTransitionIn));
//        break;
//      }
//        
//      case KeyFrame::STOP_HEAD_NOD:
//      {
//        HeadController::StopNodding();
//        //HeadController::SetDesiredAngle(StopHeadNod.finalAngle);
//        break;
//      }
//        
//      case KeyFrame::START_LIFT_NOD:
//      {
//        LiftController::StartNodding(StartLiftNod.lowHeight,
//                                     StartLiftNod.highHeight,
//                                     StartLiftNod.period_ms,
//                                     0,
//                                     GetTransitionFraction(transitionOut),
//                                     GetTransitionFraction(nextTransitionIn));
//        break;
//      }
//        
//      case KeyFrame::STOP_LIFT_NOD:
//      {
//        LiftController::StopNodding();
//        break;
//      }
//        
//      /*
//      case KeyFrame::SET_LED_COLORS:
//      {
//        // TODO: Move this into some kind of LightController file/namespace
//        for(s32 iLED=0; iLED<NUM_LEDS; ++iLED) {
//          const u32& currLEDcolor = SetLEDcolors.led[iLED];
//          if(currLEDcolor != KeyFrame::UNSPECIFIED_COLOR) {
//            HAL::SetLED(static_cast<LEDId>(iLED), currLEDcolor);
//          }
//        }
//        break;
//      }
//       */
//        
//      case KeyFrame::POINT_TURN:
//      {
//        SteeringController::ExecuteDirectDrive(0.f, 0.f);
//        break;
//      }
//        
//      case KeyFrame::DRIVE_LINE_SEGMENT:
//      {
//        SteeringController::ExecuteDirectDrive(0.f, 0.f);
//        break;
//      }
//        
//      case KeyFrame::PLAY_SOUND:
//      {
//        // TODO: Play streamed-in sound directly using robot speaker once available
//        // For now, though, we'll just request the basestation do it for us
//        Messages::PlaySoundOnBaseStation msg;
//        msg.soundID  = PlaySound.soundID;
//        msg.numLoops = PlaySound.numLoops;
//        msg.volume   = PlaySound.volume;
//        HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::PlaySoundOnBaseStation), &msg);
//        break;
//      }
//        
//      case KeyFrame::STOP_SOUND:
//      {
//        Messages::StopSoundOnBaseStation msg;
//        HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::StopSoundOnBaseStation), &msg);
//        break;
//      }
//        /*
//      case KeyFrame::BLINK_EYES:
//      {
//        EyeController::SetEyeColor(BlinkEyes.color);
//        EyeController::SetBlinkVariability(BlinkEyes.variability_ms);
//        EyeController::StartBlinking(BlinkEyes.timeOn_ms, BlinkEyes.timeOff_ms);
//        break;
//      }
//        
//      case KeyFrame::FLASH_EYES:
//      {
//        EyeController::SetEyeColor(FlashEyes.color);
//        EyeController::StartFlashing(FlashEyes.shape, FlashEyes.timeOn_ms, FlashEyes.timeOff_ms);
//        break;
//      }
//        
//      case KeyFrame::SET_EYE:
//      {
//        switch(SetEye.whichEye)
//        {
//          case EYE_LEFT:
//            EyeController::SetEyeColor(SetEye.color, LED_CURRENT_COLOR);
//            EyeController::SetEyeShape(SetEye.shape, EYE_CURRENT_SHAPE);
//            break;
//
//          case EYE_RIGHT:
//            EyeController::SetEyeColor(LED_CURRENT_COLOR, SetEye.color);
//            EyeController::SetEyeShape(EYE_CURRENT_SHAPE, SetEye.shape);
//            break;
//
//          case EYE_BOTH:
//            EyeController::SetEyeColor(SetEye.color);
//            EyeController::SetEyeShape(SetEye.shape);
//            break;
//
//          default:
//            AnkiError("KeyFrame.TranisionOutOf.UnknownWhichEye",
//                      "Invalid specification for SetEye.whichEye\n");
//            break;
//        }
//        
//        break;
//      }
//        
//      case KeyFrame::SPIN_EYES:
//      {
//        EyeController::SetEyeColor(SpinEyes.color);
//        EyeController::StartSpinning(SpinEyes.period_ms,
//                                     SpinEyes.leftClockwise,
//                                     SpinEyes.rightClockWise);
//        break;
//      }
//      
//      case KeyFrame::STOP_EYES:
//      {
//        EyeController::StopAnimating();
//        break;
//      }
//        */
//        
//        /*
//      case KeyFrame::TRIGGER_ANIMATION:
//      {
//        AnimationController::Play(TriggerAnimation.animID, TriggerAnimation.numLoops);
//        break;
//      }
//      */
//        
//      default:
//      {
//        // Do nothing if no TransitionOutOf behavior defined for this type
//        break;
//      }
//        
//    } // switch(type)
//    
//  } // TransitionOutOf()
//
//  
//  void KeyFrame::TransitionInto(const TimeStamp_t animStartTime_ms,
//                                const u8 prevTransitionOut) const
//  {
//    switch(type)
//    {
//      case KeyFrame::HEAD_ANGLE:
//      {
//        s32 angleAdj = 0;
//        if(SetHeadAngle.variability_deg > 0) {
//          angleAdj = Embedded::RandS32(-static_cast<s32>(SetHeadAngle.variability_deg),
//                                        static_cast<s32>(SetHeadAngle.variability_deg));
//        }
//        
//        const f32 angle_rad = GetAngleRad(static_cast<s32>(SetHeadAngle.angle_deg) + angleAdj);
//        
//        if(relTime_ms == 0) {
//          // Get into position ASAP:
//          HeadController::SetMaxSpeedAndAccel(10*M_PI, 1000.f);
//          HeadController::SetDesiredAngle(angle_rad);
//        } else {
//          const f32 duration_sec = (animStartTime_ms + relTime_ms - HAL::GetTimeStamp())*0.001f;
//          HeadController::SetDesiredAngle(angle_rad,
//                                          GetTransitionFraction(prevTransitionOut),
//                                          GetTransitionFraction(transitionIn),
//                                          duration_sec);
//        }
//        
//        
//      
//        // TODO: Switch to method that takes desired time into account:
//        /*
//         HeadController::SetDesiredAngleAndTime(SetHeadAngle.targetAngle,
//         SetHeadAngle.targetSpeed,
//         (relTime_ms + animStartTime_ms)*1000);
//         */
//        break;
//      }
//        
//      case KeyFrame::START_HEAD_NOD:
//      {
//        if(relTime_ms == 0) {
//          // Get the head in position to start animation [as fast as possible?]
//          HeadController::SetMaxSpeedAndAccel(10*M_PI, 1000.f);
//          HeadController::SetDesiredAngle(GetAngleRad(StartHeadNod.lowAngle_deg));
//          
//          // TODO: Switch to method that takes desired time into account:
//          /*
//           LiftController::SetDesiredHeightAndTime(SetLiftHeight.targetHeight,
//           SetLiftHeight.targetSpeed,
//           (relTime_ms + animStartTime_ms)*1000);
//           */
//        }
//        break;
//      }
//        
//      case KeyFrame::LIFT_HEIGHT:
//      {
//        if(relTime_ms == 0) {
//          // Get to start position ASAP
//          LiftController::SetMaxSpeedAndAccel(1000.f, 1000.f);
//          LiftController::SetDesiredHeight(SetLiftHeight.targetHeight);
//        } else {
//          const f32 duration_sec = (animStartTime_ms + relTime_ms - HAL::GetTimeStamp())*0.001f;
//          LiftController::SetDesiredHeight(SetLiftHeight.targetHeight,
//                                           GetTransitionFraction(prevTransitionOut),
//                                           GetTransitionFraction(transitionIn),
//                                           duration_sec);
//        }
//        
//        // TODO: Switch to method that takes desired time into account:
//        /*
//         LiftController::SetDesiredHeightAndTime(SetLiftHeight.targetHeight,
//         SetLiftHeight.targetSpeed,
//         (relTime_ms + animStartTime_ms)*1000);
//         */
//        break;
//      }
//        
//      case KeyFrame::START_LIFT_NOD:
//      {
//        if(relTime_ms == 0) {
//          // Get the lift in position to start animation [as fast as possible?]
//          LiftController::SetMaxSpeedAndAccel(1000.f, 1000.f);
//          LiftController::SetDesiredHeight(StartLiftNod.lowHeight);
//          
//          // TODO: Switch to method that takes desired time into account:
//          /*
//           LiftController::SetDesiredHeightAndTime(SetLiftHeight.targetHeight,
//           SetLiftHeight.targetSpeed,
//           (relTime_ms + animStartTime_ms)*1000);
//           */
//        }
//        break;
//      }
//        
//      case KeyFrame::POINT_TURN:
//      {
//        // TODO: Switch to new method that accepts duration and computes the right velocity profile
//        
//        s32 angleAdj = 0;
//        if(TurnInPlace.variability_deg > 0) {
//          angleAdj = Embedded::RandS32(-static_cast<s32>(TurnInPlace.variability_deg),
//                                        static_cast<s32>(TurnInPlace.variability_deg));
//        }
//        
//        const f32 angle_rad   = GetAngleRad(static_cast<s32>(TurnInPlace.relativeAngle_deg) + angleAdj);
//        const f32 duration_sec = (animStartTime_ms + relTime_ms - HAL::GetTimeStamp()) * 0.001f;
//        const f32 wheelSpeed  = (angle_rad * WHEEL_DIST_MM) / (duration_sec * 2.f);
//
//        SteeringController::ExecuteDirectDrive(wheelSpeed, -wheelSpeed);
//        
//        break;
//      }
//        
//      case KeyFrame::DRIVE_LINE_SEGMENT:
//      {
//        // TODO: Switch to new method that accepts duration and computes the right velocity profile
//        const f32 duration_ms = animStartTime_ms + relTime_ms - HAL::GetTimeStamp();
//        const f32 wheelSpeed = (DriveLineSegment.relativeDistance * 1000.f) / duration_ms;
//        SteeringController::ExecuteDirectDrive(wheelSpeed, wheelSpeed);
//        break;
//      }
//        
//      default:
//      {
//        // Do nothing if no TransitionInto behavior defined for this type
//        break;
//      }
//        
//    } // switch(type)
//    
//  } // TransitionInto()
//  
//  bool KeyFrame::IsInPosition()
//  {
//    switch(type)
//    {
//      case KeyFrame::START_HEAD_NOD:
//      case KeyFrame::HEAD_ANGLE:
//        return HeadController::IsInPosition();
//
//      case KeyFrame::START_LIFT_NOD:
//      case KeyFrame::LIFT_HEIGHT:
//        return LiftController::IsInPosition();
//        
//      case KeyFrame::PLAY_SOUND:
//        // TODO: Add something here to check whether streaming sound from BaseStation is ready?
//        return true;
//        
//      default:
//        return true;
//        
//    } // switch(type)
//    
//  } // IsInPosition()
  
  
} // namespace Cozmo
} // namespace Anki

