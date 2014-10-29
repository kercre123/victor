#include "animationController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"
#include "anki/common/shared/velocityProfileGenerator.h"

#include "animation.h"
#include "keyFrame.h"
#include "localization.h"
#include "headController.h"
#include "liftController.h"
#include "wheelController.h"
#include "steeringController.h"
#include "speedController.h"


#define DEBUG_ANIMATION_CONTROLLER 0

#define USE_HARDCODED_ANIMATIONS 0

namespace Anki {
namespace Cozmo {
namespace AnimationController {
  
  namespace {
    
    const AnimationID_t ANIM_IDLE = -1;
    //static const AnimationID_t ANIM_IDLE = MAX_KNOWN_ANIMATIONS;
    
    AnimationID_t _currAnimID   = ANIM_IDLE;
    AnimationID_t _queuedAnimID = ANIM_IDLE;
    Animation     _cannedAnimations[MAX_CANNED_ANIMATIONS];
    
    s32 _currDesiredLoops   = 0;
    s32 _queuedDesiredLoops = 0;
    s32 _numLoopsComplete   = 0;
    u32 _waitUntilTime_us   = 0;
    
    f32 _currCmdWheelSpeed  = 0;
//    f32 currCmdHeadAngle_ = 0;
    
    Radians _startingRobotAngle = 0;
//    f32 startingHeadAngle_ = 0;
//    f32 startingHeadMaxSpeed_ = 0;
//    f32 startingHeadStartAccel_ = 0;
    
    bool _isStopping  = false;
    bool _wasStopping = false;
    
    /*
    // Function pointers for start/update/stop behavior of each animation.
    typedef void (*animFuncPtr)(void);
    animFuncPtr animStartFn_[ANIM_NUM_ANIMATIONS] = {0};
    animFuncPtr animUpdateFn_[ANIM_NUM_ANIMATIONS] = {0};  // Update functions must increment numLoops_.
    animFuncPtr animStopFn_[ANIM_NUM_ANIMATIONS] = {0};    // Stop functions must set isStopping_ to false once stop conditions are met.
    
    // Start/Update/Stop functions for each type of KeyFrame
    typedef Result (*KeyFrameFuncPtr)(const KeyFrame&);
    KeyFrameFuncPtr keyFrameStartFn_[KeyFrame::NUM_TYPES]  = {0};
    KeyFrameFuncPtr keyFrameUpdateFn_[KeyFrame::NUM_TYPES] = {0};
    KeyFrameFuncPtr keyFrameStopFn_[KeyFrame::NUM_TYPES]   = {0};
    */
    
    // ANIM_BACK_AND_FORTH_EXCITED
    const f32 BAF_SPEED_MMPS = 30;
    const u32 BAF_SWITCH_PERIOD_US = 300000;
    
    // ANIM_WIGGLE
    const f32 WIGGLE_PERIOD_US = 300000;
    const f32 WIGGLE_WHEEL_SPEED_MMPS = 30;

    
  } // "private" members

  
  // Forward decl.
//  void ReallyStop();
//  void StopCurrent();
  
  
  
  // ============= Animation Start/Update/Stop functions ===============
  
  

  
  // TODO: Move back-and-forth stuff to WheelController (??)
  
  void BackAndForthStart()
  {
    _currCmdWheelSpeed = BAF_SPEED_MMPS;
  }
  
  void BackAndForthUpdate()
  {
    if (_waitUntilTime_us < HAL::GetMicroCounter()) {
      _currCmdWheelSpeed *= -1;
      SteeringController::ExecuteDirectDrive(_currCmdWheelSpeed, _currCmdWheelSpeed);
      _waitUntilTime_us = HAL::GetMicroCounter() + BAF_SWITCH_PERIOD_US;
      if (_currCmdWheelSpeed < 0) {
        ++_numLoopsComplete;
      }
    }
  }
  
  
  // TODO: Move wiggling stuff to SteeringController (??)
  
  void WiggleStart()
  {
    _currCmdWheelSpeed = WIGGLE_WHEEL_SPEED_MMPS;
  }
  
  void WiggleUpdate()
  {
    if (_waitUntilTime_us < HAL::GetMicroCounter()) {
      _currCmdWheelSpeed *= -1;
      SteeringController::ExecuteDirectDrive(_currCmdWheelSpeed, -_currCmdWheelSpeed);
      _waitUntilTime_us = HAL::GetMicroCounter() + WIGGLE_PERIOD_US;
      if (_currCmdWheelSpeed < 0) {
        ++_numLoopsComplete;
      }
    }
  }
  
  void WiggleStop()
  {
    if (!_wasStopping && _isStopping) {
      
      // Which way is closer to turn to starting orientation?
      f32 rotSpeed = PI_F * ( (_startingRobotAngle - Localization::GetCurrentMatOrientation()).ToFloat() > 0 ? 1 : -1);
      SteeringController::ExecutePointTurn(_startingRobotAngle.ToFloat(), rotSpeed, 10, 10);
      
    } else if (SteeringController::GetMode() != SteeringController::SM_POINT_TURN) {
      _isStopping = false;
    }
  }
  

  // ========== End of Animation Start/Update/Stop functions ===========
  
  static void DefineHardCodedAnimations()
  {
#if USE_HARDCODED_ANIMATIONS
    //
    // FAST HEAD NOD - 3 fast nods
    //
    {
      ClearCannedAnimation(ANIM_HEAD_NOD);
      KeyFrame kf;
      kf.transitionIn  = KF_TRANSITION_LINEAR;
      kf.transitionOut = KF_TRANSITION_LINEAR;
      
      
      // Start the nod
      kf.type = KeyFrame::START_HEAD_NOD;
      kf.relTime_ms = 0;
      kf.StartHeadNod.lowAngle  = DEG_TO_RAD(-10);
      kf.StartHeadNod.highAngle = DEG_TO_RAD( 10);
      kf.StartHeadNod.period_ms = 600;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD);
      
      // Stop the nod
      kf.type = KeyFrame::STOP_HEAD_NOD;
      kf.relTime_ms = 1500;
      kf.StopHeadNod.finalAngle = 0.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD);
      
    } // FAST HEAD NOD
    
    //
    // SLOW HEAD NOD - 2 slow nods
    //
    {
      ClearCannedAnimation(ANIM_HEAD_NOD_SLOW);
      KeyFrame kf;
      kf.transitionIn  = KF_TRANSITION_LINEAR;
      kf.transitionOut = KF_TRANSITION_LINEAR;
      
      // Start the nod
      kf.type = KeyFrame::START_HEAD_NOD;
      kf.relTime_ms = 0;
      kf.StartHeadNod.lowAngle  = DEG_TO_RAD(-25);
      kf.StartHeadNod.highAngle = DEG_TO_RAD( 25);
      kf.StartHeadNod.period_ms = 1200;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD_SLOW);
      
      // Stop the nod
      kf.type = KeyFrame::STOP_HEAD_NOD;
      kf.relTime_ms = 2400;
      kf.StopHeadNod.finalAngle = 0.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD_SLOW);
      
    } // SLOW HEAD NOD
    
    
    //
    // BLINK
    //
    {
      ClearCannedAnimation(ANIM_BLINK);
      KeyFrame kf;
      kf.type = KeyFrame::SET_LED_COLORS;
      kf.transitionIn  = KF_TRANSITION_INSTANT;
      kf.transitionOut = KF_TRANSITION_INSTANT;
      
      // Start with all eye segments on:
      kf.relTime_ms = 0;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_BLUE;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
      
      // Turn off top/bottom segments first
      kf.relTime_ms = 1700;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_OFF;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
      
      // Turn off all segments shortly thereafter
      kf.relTime_ms = 1750;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_OFF;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
      
      // Turn on left/right segments first
      kf.relTime_ms = 1850;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_OFF;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
      
      // Turn on all segments shortly thereafter
      kf.relTime_ms = 1900;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_BLUE;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
    } // BLINK
    
    //
    // Up/Down/Left/Right
    //  Move lift and head up and down (in opposite directions)
    //  Then turn to the left and the to the right
    {
      ClearCannedAnimation(ANIM_UPDOWNLEFTRIGHT);
      KeyFrame kf;
      
      // Move head up
      kf.type = KeyFrame::HEAD_ANGLE;
      kf.relTime_ms = 0;
      kf.SetHeadAngle.targetAngle = DEG_TO_RAD(25.f);
      kf.SetHeadAngle.targetSpeed = 5.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Move lift down
      kf.type = KeyFrame::LIFT_HEIGHT;
      kf.relTime_ms = 0;
      kf.SetLiftHeight.targetHeight = 0.f;
      kf.SetLiftHeight.targetSpeed  = 50.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Move head down
      kf.type = KeyFrame::HEAD_ANGLE;
      kf.relTime_ms = 750;
      kf.SetHeadAngle.targetAngle = DEG_TO_RAD(-25.f);
      kf.SetHeadAngle.targetSpeed = 5.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Move lift up
      kf.type = KeyFrame::LIFT_HEIGHT;
      kf.relTime_ms = 750;
      kf.SetLiftHeight.targetHeight = 75.f;
      kf.SetLiftHeight.targetSpeed  = 50.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Turn left
      kf.type = KeyFrame::POINT_TURN;
      kf.relTime_ms = 1250;
      kf.TurnInPlace.relativeAngle = DEG_TO_RAD(-45);
      kf.TurnInPlace.targetSpeed = 100.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);

      // Turn right
      kf.type = KeyFrame::POINT_TURN;
      kf.relTime_ms = 2250;
      kf.TurnInPlace.relativeAngle = DEG_TO_RAD(90);
      kf.TurnInPlace.targetSpeed = 100.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Turn back to center
      kf.type = KeyFrame::POINT_TURN;
      kf.relTime_ms = 2750;
      kf.TurnInPlace.relativeAngle = DEG_TO_RAD(-45);
      kf.TurnInPlace.targetSpeed = 100.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
    } // Up/Down/Left/Right
    
    //
    // BACK_AND_FORTH_EXCITED
    //
    {
      ClearCannedAnimation(ANIM_BACK_AND_FORTH_EXCITED);
      KeyFrame kf;
      
      kf.type = KeyFrame::DRIVE_LINE_SEGMENT;
      kf.relTime_ms = 300;
      kf.DriveLineSegment.relativeDistance = -9;
      kf.DriveLineSegment.targetSpeed = 30;
      AddKeyFrameToCannedAnimation(kf, ANIM_BACK_AND_FORTH_EXCITED);
      
      kf.type = KeyFrame::DRIVE_LINE_SEGMENT;
      kf.relTime_ms = 600;
      kf.DriveLineSegment.relativeDistance = 9;
      kf.DriveLineSegment.targetSpeed = 30;
      AddKeyFrameToCannedAnimation(kf, ANIM_BACK_AND_FORTH_EXCITED);
      
    } // BACK_AND_FORTH_EXCITED
    
#endif // USE_HARDCODED_ANIMATIONS
    
  } // DefineHardCodedAnimations()
  
  
  Result Init()
  {
    DefineHardCodedAnimations();
    
    return RESULT_OK;
  }
  
  
  /*
  void Update()
  {
    
    if (IsPlaying()) {
      if (isStopping_)
      {
        // Execute stopping action
        if (animStopFn_[currAnim_])
        {
          animStopFn_[currAnim_]();
          
          // Stopping conditions have been met.
          // Stop the robot.
          if (!isStopping_) {
            ReallyStop();
          }
        } else {
          PRINT("WARNING (AnimationController): Entered stopping state without a defined stoppping function\n");
          isStopping_ = false;
        }
        
      } else {
        
        // Execute animation
        if (animUpdateFn_[currAnim_])
        {
          animUpdateFn_[currAnim_]();
        }
        
        // Stop once the commanded number of animation loops has been reached
        if ((currDesiredLoops_ > 0) && (numLoopsComplete_ >= currDesiredLoops_)) {
          Stop();
        }
      }
    } else {
      // If there's a queued animation, start it.
      if (queuedAnim_ != ANIM_IDLE) {
        Play(queuedAnim_, queuedDesiredLoops_);
        queuedAnim_ = ANIM_IDLE;
      }
    }
    
    wasStopping_ = isStopping_;
  }
   */
  
  Result ClearCannedAnimation(const AnimationID_t whichAnimation)
  {
    AnkiConditionalErrorAndReturnValue(whichAnimation>=0 && whichAnimation < MAX_CANNED_ANIMATIONS, RESULT_FAIL,
                                       "AnimationController.ClearCannedAnimation.InvalidAnimationID",
                                       "Out-of-range animation ID = %d\n", whichAnimation);
    
    _cannedAnimations[whichAnimation].Clear();
    _cannedAnimations[whichAnimation].SetID(whichAnimation);
    
    return RESULT_OK;
  }
  
  Result AddKeyFrameToCannedAnimation(const KeyFrame&     keyframe,
                                      const AnimationID_t whichAnimation)
  {
    AnkiConditionalErrorAndReturnValue(whichAnimation>=0 && whichAnimation < MAX_CANNED_ANIMATIONS, RESULT_FAIL,
                                       "AnimationController.AddKeyFrameToCannedAnimation.InvalidAnimationID",
                                       "Out-of-range animation ID = %d\n", whichAnimation);
    
    return _cannedAnimations[whichAnimation].AddKeyFrame(keyframe);
  }
  
  
  void Update()
  {
    if (IsPlaying()) {
      
      _cannedAnimations[_currAnimID].Update();
      
      if(!_cannedAnimations[_currAnimID].IsPlaying()) {
        // If current animation just finished
        ++_numLoopsComplete;
        if(_currDesiredLoops == 0 ||  _numLoopsComplete < _currDesiredLoops) {
          // Looping: play again
          _cannedAnimations[_currAnimID].Init();
        } else {
          _currAnimID = ANIM_IDLE;
        }
      }
      
    } else if (_queuedAnimID != ANIM_IDLE) {
      PRINT("Playing queued animation %d, %d loops\n", _queuedAnimID, _queuedDesiredLoops);
      // If there's a queued animation, start it.
      Play(_queuedAnimID, _queuedDesiredLoops);
      _queuedAnimID = ANIM_IDLE;
    }
    
  } // Update()

  
  void Play(const AnimationID_t anim, const u32 numLoops)
  {
    AnkiConditionalErrorAndReturn(anim < MAX_CANNED_ANIMATIONS,
                                  "AnimationController.Play.InvalidAnimation",
                                  "Animation ID out of range.\n");
    
    AnkiConditionalWarnAndReturn(_cannedAnimations[anim].IsDefined(),
                                 "AnimationController.Play.EmptyAnimation",
                                 "Asked to play empty animation %d. Ignoring.\n", anim);
    
    // If an animation is currently playing, stop it and queue this one.
    if (IsPlaying()) {
      Stop();
      _queuedAnimID = anim;
      _queuedDesiredLoops = numLoops;
      return;
    }
    
    // Playing IDLE animation is equivalent to stopping currently playing
    // animation and clearing queued animation
    if (anim == ANIM_IDLE) {
      Stop();
      return;
    }
    
    PRINT("Playing Animation %d, %d loops\n", anim, numLoops);
    
    _currAnimID       = anim;
    _currDesiredLoops = numLoops;
    _numLoopsComplete = 0;
    _waitUntilTime_us = 0;
    
    _cannedAnimations[_currAnimID].Init();
    
    /*
    startingRobotAngle_ = Localization::GetCurrentMatOrientation();
    startingHeadAngle_ = HeadController::GetAngleRad();
    HeadController::GetSpeedAndAccel(startingHeadMaxSpeed_, startingHeadStartAccel_);
    
    // Initialize the animation
    if (animStartFn_[currAnim_])
    {
      animStartFn_[currAnim_]();
    }
     */
    
  } // Play()

  /*
  // Stops the current animation
  void StopCurrent() {

    // If a stop function is defined, set stopping flag.
    // Otherwise, just stop.
    if (animStopFn_[currAnim_]) {
      isStopping_ = true;
    } else {
      ReallyStop();
    }
  }

  
  // Stops current animation and clears queued animation if one exists.
  void Stop()
  {
    StopCurrent();
    queuedAnim_ = ANIM_IDLE;
  }
  
  // Stops all motors. Called when stopping conditions have finally been met.
  void ReallyStop()
  {
   */
  void Stop()
  {
    if(_currAnimID != ANIM_IDLE) {
      _cannedAnimations[_currAnimID].Stop();
    }
    
    _isStopping = _wasStopping = false;
    _currAnimID = _queuedAnimID = ANIM_IDLE;
    
    // Stop all motors, lights, and sounds
    LiftController::Enable();
    LiftController::SetAngularVelocity(0);
    HeadController::Enable();
    HeadController::SetAngularVelocity(0);
    
    SteeringController::ExecuteDirectDrive(0, 0);
    SpeedController::SetBothDesiredAndCurrentUserSpeed(0);
  }
  
  
  bool IsPlaying()
  {
    return _currAnimID != ANIM_IDLE;
  }
  
  bool IsDefined(const AnimationID_t anim)
  {
    if(anim < 0 || anim >= MAX_CANNED_ANIMATIONS) {
      return false;
    }
    return _cannedAnimations[anim].IsDefined();
  }
  
} // namespace AnimationController
} // namespace Cozmo
} // namespace Anki
