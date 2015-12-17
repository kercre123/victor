/**
 * File: behaviorFollowMotion.cpp
 *
 * Author: Andrew Stein
 * Created: 11/13/15
 *
 * Description: Behavior for following motion in the image
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviors/behaviorFollowMotion.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/trackingActions.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static std::vector<std::string> _animReactions = {
  "Demo_Motion_Reaction",
};

BehaviorFollowMotion::BehaviorFollowMotion(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("FollowMotion");

  SubscribeToTags({{
    EngineToGameTag::RobotObservedMotion,
    EngineToGameTag::RobotCompletedAction
  }});
}
  
bool BehaviorFollowMotion::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  return true;
}

float BehaviorFollowMotion::EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const
{
  return 0.3f; // for the investor demo, just use a fixed score
}

Result BehaviorFollowMotion::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
  // Store whatever modes the vision system was using so we can restore once this
  // behavior completes
  // NOTE: if vision modes are enabled _while_ this behavior is running, they will be
  //       disabled when it finishes!
  
  _originalVisionModes = robot.GetVisionComponent().GetEnabledModes();
  robot.GetVisionComponent().EnableMode(VisionMode::DetectingMotion, true);
  
  // Do the initial reaction for first motion each time we restart this behavior
  // (but only if it's been long enough since last interruption)
  if(currentTime_sec - _lastInterruptTime_sec > _initialReactionWaitTime_sec) {
    _initialReactionAnimPlayed = false;
    _state = State::WaitingForFirstMotion;
    SetStateName("Wait");
  } else {
    _initialReactionAnimPlayed = true;
    StartTracking(robot);
    _state = State::Tracking;
    SetStateName("Tracking");
  }
  
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorFollowMotion::UpdateInternal(Robot& robot, double currentTime_sec)
{
  IBehavior::Status status = Status::Running;
  
  switch(_state)
  {
    case State::Interrupted:
      status = Status::Complete;
      break;

    case State::HoldingHeadDown:
      if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() >= _holdHeadDownUntil) {
        StartTracking(robot); // puts us in Tracking state
      }
      break;
      
    case State::WaitingForFirstMotion:
    case State::Tracking:
    case State::DrivingForward:
      // Nothing to do
      break;
      
  } // switch(_state)
  
  return status;
}

Result BehaviorFollowMotion::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  robot.GetActionList().Cancel(_actionRunning);
  
  _actionRunning = (u32)ActionConstants::INVALID_TAG;
  _lastInterruptTime_sec = currentTime_sec;
  _holdHeadDownUntil = -1.0f;

  PRINT_NAMED_DEBUG("BehaviorFollowMotion.InterruptInternal", "restoring original vision modes");
  
  // Restore original vision modes
  robot.GetVisionComponent().SetModes(_originalVisionModes);
  
  _state = State::Interrupted;
  SetStateName("Interrupted");
  
  return Result::RESULT_OK;
}
  
void BehaviorFollowMotion::StartTracking(Robot& robot)
{
  TrackMotionAction* action = new TrackMotionAction();
  _actionRunning = action->GetTag();
  
  robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, action);
  
  _state = State::Tracking;
  SetStateName("Tracking");
}
 
  
void BehaviorFollowMotion::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion:
      HandleObservedMotion(event, robot);
      break;
      
    case MessageEngineToGameTag::RobotCompletedAction:
      HandleCompletedAction(event, robot);
      break;
      
    default:
    {
      PRINT_NAMED_ERROR("BehaviorFollowMotion.HandleMotionEvent.UnknownEvent",
                        "Reached unknown state %hhu.", event.GetData().GetTag());
    }
  } // switch(event type)
} // HandeWhileRunning()

  
void BehaviorFollowMotion::HandleObservedMotion(const EngineToGameEvent &event, Robot& robot)

{
  const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
  
  // Convert image positions to desired relative angles
  const Point2f motionCentroid(motionObserved.img_x, motionObserved.img_y);
  
  // Compute the relative pan and tilt angles to put the centroid in the center of the image
  Radians relHeadAngle_rad = 0, relBodyPanAngle_rad = 0;
  robot.GetVisionComponent().GetCamera().ComputePanAndTiltAngles(motionCentroid, relBodyPanAngle_rad, relHeadAngle_rad);
  
  if(State::WaitingForFirstMotion == _state && _actionRunning==0 && motionObserved.img_area > 0)
  {
    if(!_initialReactionAnimPlayed)
    {
      // Robot gets more happy/excited and less calm when he sees motion.
      robot.GetMoodManager().AddToEmotions(EmotionType::Happy,   kEmotionChangeSmall,
                                           EmotionType::Excited, kEmotionChangeSmall,
                                           EmotionType::Calm,   -kEmotionChangeSmall, "MotionReact");
      
      // Turn to face the motion:
      PanAndTiltAction* panTiltAction = new PanAndTiltAction(relBodyPanAngle_rad,
                                                             relHeadAngle_rad,
                                                             false, false);
      robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, panTiltAction);
      
      
      // If this is the first motion reaction, also play the first part of the
      // motion reaction animation, move the head back to the right tilt, and
      // then play the second half of the animation to open the eyes back up
      const Radians finalHeadAngle = robot.GetHeadAngle() + relHeadAngle_rad;
      
      PRINT_NAMED_INFO("BehaviorFollowMotion.HandleWhileRunning.FirstMotion",
                       "Queuing first motion reaction animation and head tilt back to %.1fdeg",
                       finalHeadAngle.getDegrees());
      
      CompoundActionSequential* compoundAction = new CompoundActionSequential({
        new PlayAnimationAction("ID_MotionFollow_ReactToMotion"),
        new MoveHeadToAngleAction(finalHeadAngle, _panAndTiltTol),
        new PlayAnimationAction("ID_MotionFollow_ReactToMotion_end")
      });
      
      // Wait for the animation to complete
      _actionRunning = compoundAction->GetTag();
      
      robot.GetActionList().QueueActionNext(Robot::DriveAndManipulateSlot, compoundAction);
    }
  }
  
  else if(State::Tracking == _state)
  {
    PRINT_NAMED_INFO("BehaviorFollowMotion.HandleWhileRunning.Motion",
                     "Motion area=%d, centroid=(%.1f,%.1f), HeadTilt=%.1fdeg, BodyPan=%.1fdeg",
                     motionObserved.img_area,
                     motionCentroid.x(), motionCentroid.y(),
                     relHeadAngle_rad.getDegrees(), relBodyPanAngle_rad.getDegrees());
    
    const bool inGroundPlane = motionObserved.ground_area > _minGroundAreaToConsider;
    
    if(inGroundPlane)
    {
      const float robotOffsetX = motionObserved.ground_x;
      const float robotOffsetY = motionObserved.ground_y;
      const float groundPlaneDist = std::sqrt( std::pow( robotOffsetX, 2 ) +
                                              std::pow( robotOffsetY, 2) );
      const bool belowMinGroundPlaneDist = groundPlaneDist < _minDriveFrowardGroundPlaneDist_mm;
      
      if( belowMinGroundPlaneDist ) {
        const float timeToHold = 1.25f;
        _holdHeadDownUntil = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + timeToHold;
        MoveHeadToAngleAction* action = new MoveHeadToAngleAction(MIN_HEAD_ANGLE);
        
        // Note that queuing action "now" will cancel the tracking action
        robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, action);
        
        PRINT_NAMED_INFO("BehaviorFollowMotion.HoldingHeadLow",
                         "got %f of image with dist %f, holding head at min angle for %f sec",
                         motionObserved.ground_area,
                         groundPlaneDist,
                         timeToHold);
        
        _state = State::HoldingHeadDown;
        SetStateName("HeadDown");
      }
    } // if(motion in ground plane)
    
    else if (relHeadAngle_rad.getAbsoluteVal() < _driveForwardTol &&
             relBodyPanAngle_rad.getAbsoluteVal() < _driveForwardTol )
    {
      // Move towards the motion since it's centered
      DriveStraightAction* driveAction = new DriveStraightAction(_moveForwardDist_mm, DEFAULT_PATH_SPEED_MMPS*_moveForwardSpeedIncrease);
      driveAction->SetAccel(DEFAULT_PATH_ACCEL_MMPS2*_moveForwardSpeedIncrease);
      _actionRunning = driveAction->GetTag();
      
      _state = State::DrivingForward;
      SetStateName("DriveForward");
      
      // Queue action now will stop the tracking that's currently running
      robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, driveAction);
      
      // PRINT_NAMED_DEBUG("BehaviorFollowMotion.DriveForward",
      //                   "relHeadAngle = %fdeg, relBodyAngle = %fdeg, ground area %f",
      //                   RAD_TO_DEG(relHeadAngle_rad.ToFloat()),
      //                   RAD_TO_DEG(relBodyPanAngle_rad.ToFloat()),
      //                   motionObserved.ground_area);
      
    } // else if(time to drive forward)
    
  } // if(_state == Tracking)

} // HandleObservedMotion()
  

void BehaviorFollowMotion::HandleCompletedAction(const EngineToGameEvent &event, Robot& robot)
{
  auto const& completedAction = event.GetData().Get_RobotCompletedAction();
  
  // If the action we were running completes, allow us to respond to motion again
  if(completedAction.idTag == _actionRunning) {
    _actionRunning = 0;
    
    switch(_state)
    {
      case State::WaitingForFirstMotion:
        ASSERT_NAMED(completedAction.actionType == RobotActionType::COMPOUND,
                     "Expecting completed action to be compound when WaitingForFirstMotion");
        _initialReactionAnimPlayed = true;
        StartTracking(robot);
        _state = State::Tracking;
        SetStateName("Tracking");
        break;
        
      case State::Tracking:
        PRINT_NAMED_INFO("BehaviorFollowMotion.HandleWhileRunning.TrackingCompleted",
                         "Tracking action completed, marking behavior as interruped.");
        _state = State::Interrupted;
        SetStateName("Interrupted");
        break;
        
      case State::HoldingHeadDown:
        if( completedAction.actionType != RobotActionType::MOVE_HEAD_TO_ANGLE ){
          PRINT_NAMED_WARNING("BehaviorFollowMotion.HandleWhileRunning.HoldingHeadDown.InvalidAction",
                              "Expecting completed action to be MoveHeadToAngle, instead got %s",
                              RobotActionTypeToString(completedAction.actionType));
        }
        // Nothing to do: we transition out of this state once timer elapses
        break;
        
      case State::DrivingForward:
        ASSERT_NAMED(completedAction.actionType == RobotActionType::DRIVE_STRAIGHT, "Expecting completed action to be DriveStraight when in state DrivingForward");
        StartTracking(robot);
        break;
        
      default:
        break;
    }
    
  } // if(completedAction.idTag == _actionRunning)
  
} // HandleCompletedAction()
  

} // namespace Cozmo
} // namespace Anki
