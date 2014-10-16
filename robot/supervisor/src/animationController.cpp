#include "animationController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"
#include "anki/common/shared/velocityProfileGenerator.h"

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
    AnimationID_t currAnim_ = ANIM_IDLE;
    AnimationID_t queuedAnim_ = ANIM_IDLE;
    s32 currDesiredLoops_ = 0;
    s32 queuedDesiredLoops_ = 0;
    s32 numLoops_ = 0;
    u32 waitUntilTime_us_ = 0;
    
    f32 currCmdWheelSpeed_ = 0;
    f32 currCmdHeadAngle_ = 0;
    
    Radians startingRobotAngle_ = 0;
    f32 startingHeadAngle_ = 0;
    f32 startingHeadMaxSpeed_ = 0;
    f32 startingHeadStartAccel_ = 0;
    
    bool isStopping_ = false;
    bool wasStopping_ = false;
    
    // Function pointers for start/update/stop behavior of each animation.
    typedef void (*animFuncPtr)(void);
    animFuncPtr animStartFn_[ANIM_NUM_ANIMATIONS] = {0};
    animFuncPtr animUpdateFn_[ANIM_NUM_ANIMATIONS] = {0};  // Update functions must increment numLoops_.
    animFuncPtr animStopFn_[ANIM_NUM_ANIMATIONS] = {0};    // Stop functions must set isStopping_ to false once stop conditions are met.
    
    // ANIM_HEAD_NOD
    const f32 HEAD_NOD_UPPER_ANGLE = DEG_TO_RAD(20);
    const f32 HEAD_NOD_LOWER_ANGLE = DEG_TO_RAD(-20);
    const f32 HEAD_NOD_SPEED_RAD_PER_S = 10;
    const f32 HEAD_NOD_ACCEL_RAD_PER_S2 = 40;
    
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
  
  void HeadNodStart()
  {
    currCmdHeadAngle_ = HEAD_NOD_LOWER_ANGLE;
    HeadController::SetSpeedAndAccel(HEAD_NOD_SPEED_RAD_PER_S, HEAD_NOD_ACCEL_RAD_PER_S2);
  }
  
  void HeadNodUpdate()
  {
    if (HeadController::IsInPosition()) {
      if (currCmdHeadAngle_ == HEAD_NOD_UPPER_ANGLE) {
        currCmdHeadAngle_ = HEAD_NOD_LOWER_ANGLE;
      } else if (currCmdHeadAngle_ == HEAD_NOD_LOWER_ANGLE) {
        currCmdHeadAngle_ = HEAD_NOD_UPPER_ANGLE;
        ++numLoops_;
      }
      HeadController::SetDesiredAngle(currCmdHeadAngle_);
    }
  }
  
  void HeadNodStop()
  {
    if (!wasStopping_ && isStopping_) {
      HeadController::SetDesiredAngle(startingHeadAngle_);
    } else if (HeadController::IsInPosition()) {
      HeadController::SetSpeedAndAccel(startingHeadMaxSpeed_, startingHeadStartAccel_);
      isStopping_ = false;
    }
  }

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
        ++numLoops_;
      }
    }
  }
  
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
        ++numLoops_;
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
    
    return RESULT_OK;
  }
  
  
  
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
        if ((currDesiredLoops_ > 0) && (numLoops_ >= currDesiredLoops_)) {
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

  void Play(const AnimationID_t anim, const u32 numLoops)
  {
    if (anim == ANIM_NUM_ANIMATIONS) {
      return;
    }
    
    // If an animation is currently playing, stop it and queue this one.
    if (IsPlaying()) {
      StopCurrent();
      queuedAnim_ = anim;
      queuedDesiredLoops_ = currDesiredLoops_;
      return;
    }
    
    // Playing IDLE animation is equivalent to stopping currently playing
    // animation and clearing queued animation
    if (anim == ANIM_IDLE) {
      ReallyStop();
      queuedAnim_ = ANIM_IDLE;
      return;
    }
    
    currAnim_ = anim;
    currDesiredLoops_ = numLoops;
    numLoops_ = -1;
    waitUntilTime_us_ = 0;
    
    startingRobotAngle_ = Localization::GetCurrentMatOrientation();
    startingHeadAngle_ = HeadController::GetAngleRad();
    HeadController::GetSpeedAndAccel(startingHeadMaxSpeed_, startingHeadStartAccel_);
    
    // Initialize the animation
    if (animStartFn_[currAnim_])
    {
      animStartFn_[currAnim_]();
    }
  }

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
