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

namespace Anki {
namespace Cozmo {
namespace AnimationController {
  
  namespace {
    const s32 MAX_KNOWN_ANIMATIONS = 64;
    
    static const AnimationID_t ANIM_IDLE = MAX_KNOWN_ANIMATIONS;
    
    AnimationID_t currAnim_ = ANIM_IDLE;
    AnimationID_t queuedAnim_ = ANIM_IDLE;
    Animation     cannedAnimations[MAX_KNOWN_ANIMATIONS];
    
    s32 currDesiredLoops_ = 0;
    s32 queuedDesiredLoops_ = 0;
    s32 numLoopsComplete_ = 0;
    u32 waitUntilTime_us_ = 0;
    
    f32 currCmdWheelSpeed_ = 0;
//    f32 currCmdHeadAngle_ = 0;
    
    Radians startingRobotAngle_ = 0;
//    f32 startingHeadAngle_ = 0;
//    f32 startingHeadMaxSpeed_ = 0;
//    f32 startingHeadStartAccel_ = 0;
    
    bool isStopping_ = false;
    bool wasStopping_ = false;
    
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
  void ReallyStop();
  void StopCurrent();
  
  
  
  // ============= Animation Start/Update/Stop functions ===============
  
  

  
  // TODO: Move back-and-forth stuff to WheelController (??)
  
  void BackAndForthStart()
  {
    currCmdWheelSpeed_ = BAF_SPEED_MMPS;
  }
  
  void BackAndForthUpdate()
  {
    if (waitUntilTime_us_ < HAL::GetMicroCounter()) {
      currCmdWheelSpeed_ *= -1;
      SteeringController::ExecuteDirectDrive(currCmdWheelSpeed_, currCmdWheelSpeed_);
      waitUntilTime_us_ = HAL::GetMicroCounter() + BAF_SWITCH_PERIOD_US;
      if (currCmdWheelSpeed_ < 0) {
        ++numLoopsComplete_;
      }
    }
  }
  
  
  // TODO: Move wiggling stuff to SteeringController (??)
  
  void WiggleStart()
  {
    currCmdWheelSpeed_ = WIGGLE_WHEEL_SPEED_MMPS;
  }
  
  void WiggleUpdate()
  {
    if (waitUntilTime_us_ < HAL::GetMicroCounter()) {
      currCmdWheelSpeed_ *= -1;
      SteeringController::ExecuteDirectDrive(currCmdWheelSpeed_, -currCmdWheelSpeed_);
      waitUntilTime_us_ = HAL::GetMicroCounter() + WIGGLE_PERIOD_US;
      if (currCmdWheelSpeed_ < 0) {
        ++numLoopsComplete_;
      }
    }
  }
  
  void WiggleStop()
  {
    if (!wasStopping_ && isStopping_) {
      
      // Which way is closer to turn to starting orientation?
      f32 rotSpeed = PI_F * ( (startingRobotAngle_ - Localization::GetCurrentMatOrientation()).ToFloat() > 0 ? 1 : -1);
      SteeringController::ExecutePointTurn(startingRobotAngle_.ToFloat(), rotSpeed, 10, 10);
      
    } else if (SteeringController::GetMode() != SteeringController::SM_POINT_TURN) {
      isStopping_ = false;
    }
  }
  

  // ========== End of Animation Start/Update/Stop functions ===========
  
  
  Result Init()
  {
    /*
    // Populate function pointer arrays
    animStartFn_[ANIM_IDLE] = 0;
    animUpdateFn_[ANIM_IDLE] = 0;
    animStopFn_[ANIM_IDLE] = 0;
    
    animStartFn_[ANIM_HEAD_NOD] = HeadNodStart;
    animUpdateFn_[ANIM_HEAD_NOD] = HeadNodUpdate;
    animStopFn_[ANIM_HEAD_NOD] = HeadNodStop;

    animStartFn_[ANIM_BACK_AND_FORTH_EXCITED] = BackAndForthStart;
    animUpdateFn_[ANIM_BACK_AND_FORTH_EXCITED] = BackAndForthUpdate;
    
    animStartFn_[ANIM_WIGGLE] = WiggleStart;
    animUpdateFn_[ANIM_WIGGLE] = WiggleUpdate;
    animStopFn_[ANIM_WIGGLE] = WiggleStop;
    */
    
    // TEST: Define BlinkTrack
    const AnimationID_t BLINKING = ANIM_WIGGLE;
    ClearCannedAnimation(BLINKING);
    KeyFrame kf;
    kf.type = KeyFrame::SET_LED_COLORS;
    kf.transitionIn = KeyFrame::INSTANT;
    kf.transitionOut = KeyFrame::INSTANT;
    
    // Start with all eye segments on:
    kf.relTime_ms = 0;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_BOTTOM]  = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_LEFT]    = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_RIGHT]   = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_TOP]     = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_BOTTOM] = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_LEFT]   = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_RIGHT]  = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_TOP]    = HAL::LED_BLUE;
    AddKeyFrameToCannedAnimation(kf, BLINKING, Animation::LIGHTS);
    
    // Turn off top/bottom segments first
    kf.relTime_ms = 1700;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_BOTTOM]  = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_LEFT]    = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_RIGHT]   = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_TOP]     = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_BOTTOM] = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_LEFT]   = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_RIGHT]  = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_TOP]    = HAL::LED_OFF;
    AddKeyFrameToCannedAnimation(kf, BLINKING, Animation::LIGHTS);

    // Turn off all segments shortly thereafter
    kf.relTime_ms = 1750;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_BOTTOM]  = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_LEFT]    = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_RIGHT]   = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_TOP]     = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_BOTTOM] = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_LEFT]   = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_RIGHT]  = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_TOP]    = HAL::LED_OFF;
    AddKeyFrameToCannedAnimation(kf, BLINKING, Animation::LIGHTS);
    
    // Turn on left/right segments first
    kf.relTime_ms = 1800;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_BOTTOM]  = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_LEFT]    = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_RIGHT]   = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_TOP]     = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_BOTTOM] = HAL::LED_OFF;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_LEFT]   = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_RIGHT]  = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_TOP]    = HAL::LED_OFF;
    AddKeyFrameToCannedAnimation(kf, BLINKING, Animation::LIGHTS);

    // Turn on all segments shortly thereafter
    kf.relTime_ms = 1850;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_BOTTOM]  = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_LEFT]    = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_RIGHT]   = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_TOP]     = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_BOTTOM] = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_LEFT]   = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_RIGHT]  = HAL::LED_BLUE;
    kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_TOP]    = HAL::LED_BLUE;
    AddKeyFrameToCannedAnimation(kf, BLINKING, Animation::LIGHTS);
    
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
    AnkiConditionalErrorAndReturnValue(whichAnimation>=0 && whichAnimation < MAX_KNOWN_ANIMATIONS, RESULT_FAIL,
                                       "AnimationController.ClearCannedAnimation.InvalidAnimationID",
                                       "Out-of-range animation ID = %d\n", whichAnimation);
    
    cannedAnimations[whichAnimation].Clear();
    
    return RESULT_OK;
  }
  
  Result AddKeyFrameToCannedAnimation(const KeyFrame&             keyframe,
                                      const AnimationID_t         whichAnimation,
                                      const Animation::SubSystems whichSubSystem)
  {
    AnkiConditionalErrorAndReturnValue(whichAnimation>=0 && whichAnimation < MAX_KNOWN_ANIMATIONS, RESULT_FAIL,
                                       "AnimationController.AddKeyFrameToCannedAnimation.InvalidAnimationID",
                                       "Out-of-range animation ID = %d\n", whichAnimation);
    
    return cannedAnimations[whichAnimation].AddKeyFrame(keyframe, whichSubSystem);
  }
  
  
  void Update()
  {
    if (IsPlaying()) {
      
      cannedAnimations[currAnim_].Update();
      
      if(!cannedAnimations[currAnim_].IsPlaying()) {
        // If current animation just finished
        ++numLoopsComplete_;
        if(currDesiredLoops_ > 0 && numLoopsComplete_ < currDesiredLoops_) {
          // Looping: play again
          cannedAnimations[currAnim_].Init();
        } else {
          currAnim_ = ANIM_IDLE;
        }
      }
      
    } else if (queuedAnim_ != ANIM_IDLE) {
      // If there's a queued animation, start it.
      Play(queuedAnim_, queuedDesiredLoops_);
      queuedAnim_ = ANIM_IDLE;
    }
    
  } // Update()

  
  void Play(const AnimationID_t anim, const u32 numLoops)
  {
    AnkiConditionalError(anim < MAX_KNOWN_ANIMATIONS,
                         "AnimationController.Play.InvalidAnimation",
                         "Animation ID out of range.\n");
    
    if (anim == ANIM_IDLE) {
      // Nothing to do
      return;
    }
    
    // If an animation is currently playing, stop it and queue this one.
    if (IsPlaying()) {
      StopCurrent();
      queuedAnim_ = anim;
      queuedDesiredLoops_ = numLoops;
      return;
    }
    
    // Playing IDLE animation is equivalent to stopping currently playing
    // animation and clearing queued animation
    if (anim == ANIM_IDLE) {
      ReallyStop();
      queuedAnim_ = ANIM_IDLE;
      return;
    }
    
    PRINT("Playing Animation %d, %d loops\n", anim, numLoops);
    
    currAnim_         = anim;
    currDesiredLoops_ = numLoops;
    numLoopsComplete_ = -1;
    waitUntilTime_us_ = 0;
    
    cannedAnimations[currAnim_].Init();
    
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

  // Stops the current animation
  void StopCurrent() {
    /*
    // If a stop function is defined, set stopping flag.
    // Otherwise, just stop.
    if (animStopFn_[currAnim_]) {
      isStopping_ = true;
    } else {
      ReallyStop();
    }
     */
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
    isStopping_ = wasStopping_ = false;
    currAnim_ = ANIM_IDLE;
    
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
    return currAnim_ != ANIM_IDLE;
  }
  
} // namespace AnimationController
} // namespace Cozmo
} // namespace Anki
