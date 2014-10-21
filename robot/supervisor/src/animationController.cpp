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

#define USE_HARDCODED_ANIMATIONS 1

namespace Anki {
namespace Cozmo {
namespace AnimationController {
  
  namespace {
    const s32 MAX_KNOWN_ANIMATIONS = 64;
    
    //static const AnimationID_t ANIM_IDLE = MAX_KNOWN_ANIMATIONS;
    
    AnimationID_t currAnimID_   = ANIM_IDLE;
    AnimationID_t queuedAnimID_ = ANIM_IDLE;
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
//  void ReallyStop();
//  void StopCurrent();
  
  
  
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
  
  static void DefineHardCodedAnimations()
  {
#if USE_HARDCODED_ANIMATIONS
    //
    // FAST HEAD NOD
    //
    {
      ClearCannedAnimation(ANIM_HEAD_NOD);
      KeyFrame kf;
      kf.transitionIn  = KeyFrame::LINEAR;
      kf.transitionOut = KeyFrame::LINEAR;
      
      // Start the nod
      kf.type = KeyFrame::START_HEAD_NOD;
      kf.relTime_ms = 0;
      kf.StartHeadNod.lowAngle  = DEG_TO_RAD(-20);
      kf.StartHeadNod.highAngle = DEG_TO_RAD( 20);
      kf.StartHeadNod.speed     = 30.f;
      kf.StartHeadNod.accel     = 10.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD, Animation::HEAD);
      
      // Stop the nod
      kf.type = KeyFrame::STOP_HEAD_NOD;
      kf.relTime_ms = 1000;
      kf.StopHeadNod.finalAngle = 0.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD, Animation::HEAD);
      
    } // FAST HEAD NOD
    
    //
    // SLOW HEAD NOD
    //
    {
      ClearCannedAnimation(ANIM_HEAD_NOD_SLOW);
      KeyFrame kf;
      kf.transitionIn  = KeyFrame::LINEAR;
      kf.transitionOut = KeyFrame::LINEAR;
      
      // Start the nod
      kf.type = KeyFrame::START_HEAD_NOD;
      kf.relTime_ms = 0;
      kf.StartHeadNod.lowAngle  = DEG_TO_RAD(-25);
      kf.StartHeadNod.highAngle = DEG_TO_RAD( 25);
      kf.StartHeadNod.speed     = 5.f;
      kf.StartHeadNod.accel     = 5.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD_SLOW, Animation::HEAD);
      
      // Stop the nod
      kf.type = KeyFrame::STOP_HEAD_NOD;
      kf.relTime_ms = 2000;
      kf.StopHeadNod.finalAngle = 0.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD_SLOW, Animation::HEAD);
      
    } // SLOW HEAD NOD
    
    
    //
    // BLINK
    //
    {
      ClearCannedAnimation(ANIM_BLINK);
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
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK, Animation::LIGHTS);
      
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
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK, Animation::LIGHTS);
      
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
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK, Animation::LIGHTS);
      
      // Turn on left/right segments first
      kf.relTime_ms = 1850;
      kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_BOTTOM]  = HAL::LED_OFF;
      kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_LEFT]    = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_RIGHT]   = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_TOP]     = HAL::LED_OFF;
      kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_BOTTOM] = HAL::LED_OFF;
      kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_LEFT]   = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_RIGHT]  = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_TOP]    = HAL::LED_OFF;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK, Animation::LIGHTS);
      
      // Turn on all segments shortly thereafter
      kf.relTime_ms = 1900;
      kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_BOTTOM]  = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_LEFT]    = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_RIGHT]   = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_LEFT_EYE_TOP]     = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_BOTTOM] = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_LEFT]   = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_RIGHT]  = HAL::LED_BLUE;
      kf.SetLEDcolors.led[HAL::LED_RIGHT_EYE_TOP]    = HAL::LED_BLUE;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK, Animation::LIGHTS);
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
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT, Animation::HEAD);
      
      // Move lift down
      kf.type = KeyFrame::LIFT_HEIGHT;
      kf.relTime_ms = 0;
      kf.SetLiftHeight.targetHeight = 0.f;
      kf.SetLiftHeight.targetSpeed  = 50.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT, Animation::LIFT);
      
      // Move head down
      kf.type = KeyFrame::HEAD_ANGLE;
      kf.relTime_ms = 750;
      kf.SetHeadAngle.targetAngle = DEG_TO_RAD(-25.f);
      kf.SetHeadAngle.targetSpeed = 5.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT, Animation::HEAD);
      
      // Move lift up
      kf.type = KeyFrame::LIFT_HEIGHT;
      kf.relTime_ms = 750;
      kf.SetLiftHeight.targetHeight = 75.f;
      kf.SetLiftHeight.targetSpeed  = 50.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT, Animation::LIFT);
      
      // Turn left
      kf.type = KeyFrame::POINT_TURN;
      kf.relTime_ms = 1250;
      kf.TurnInPlace.relativeAngle = DEG_TO_RAD(-45);
      kf.TurnInPlace.targetSpeed = 100.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT, Animation::POSE);

      // Turn right
      kf.type = KeyFrame::POINT_TURN;
      kf.relTime_ms = 2250;
      kf.TurnInPlace.relativeAngle = DEG_TO_RAD(90);
      kf.TurnInPlace.targetSpeed = 100.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT, Animation::POSE);
      
      // Turn back to center
      kf.type = KeyFrame::POINT_TURN;
      kf.relTime_ms = 2750;
      kf.TurnInPlace.relativeAngle = DEG_TO_RAD(-45);
      kf.TurnInPlace.targetSpeed = 100.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT, Animation::POSE);
      
    } // Up/Down/Left/Right
    
#endif // USE_HARDCODED_ANIMATIONS
    
  } // DefineHardCodedAnimations()
  
  
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
    AnkiConditionalErrorAndReturnValue(whichAnimation>=0 && whichAnimation < MAX_KNOWN_ANIMATIONS, RESULT_FAIL,
                                       "AnimationController.ClearCannedAnimation.InvalidAnimationID",
                                       "Out-of-range animation ID = %d\n", whichAnimation);
    
    cannedAnimations[whichAnimation].Clear();
    cannedAnimations[whichAnimation].SetID(whichAnimation);
    
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
      
      cannedAnimations[currAnimID_].Update();
      
      if(!cannedAnimations[currAnimID_].IsPlaying()) {
        // If current animation just finished
        ++numLoopsComplete_;
        if(currDesiredLoops_ > 1 && numLoopsComplete_ < currDesiredLoops_) {
          // Looping: play again
          cannedAnimations[currAnimID_].Init();
        } else {
          currAnimID_ = ANIM_IDLE;
        }
      }
      
    } else if (queuedAnimID_ != ANIM_IDLE) {
      PRINT("Playing queued animation %d, %d loops\n", queuedAnimID_, queuedDesiredLoops_);
      // If there's a queued animation, start it.
      Play(queuedAnimID_, queuedDesiredLoops_);
      queuedAnimID_ = ANIM_IDLE;
    }
    
  } // Update()

  
  void Play(const AnimationID_t anim, const u32 numLoops)
  {
    AnkiConditionalError(anim < MAX_KNOWN_ANIMATIONS,
                         "AnimationController.Play.InvalidAnimation",
                         "Animation ID out of range.\n");
    
    // If an animation is currently playing, stop it and queue this one.
    if (IsPlaying()) {
      Stop();
      queuedAnimID_ = anim;
      queuedDesiredLoops_ = numLoops;
      return;
    }
    
    // Playing IDLE animation is equivalent to stopping currently playing
    // animation and clearing queued animation
    if (anim == ANIM_IDLE) {
      Stop();
      return;
    }
    
    PRINT("Playing Animation %d, %d loops\n", anim, numLoops);
    
    currAnimID_         = anim;
    currDesiredLoops_ = numLoops;
    numLoopsComplete_ = 0;
    waitUntilTime_us_ = 0;
    
    cannedAnimations[currAnimID_].Init();
    
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
    if(currAnimID_ != ANIM_IDLE) {
      cannedAnimations[currAnimID_].Stop();
    }
    
    isStopping_ = wasStopping_ = false;
    currAnimID_ = queuedAnimID_ = ANIM_IDLE;
    
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
    return currAnimID_ != ANIM_IDLE;
  }
  
} // namespace AnimationController
} // namespace Cozmo
} // namespace Anki
