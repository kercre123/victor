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
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

#define DO_BACK_UP_AFTER_POUNCE 1

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static std::vector<std::string> _animReactions = {
  "Demo_Motion_Reaction",
};
  
static std::string kLookDownAnimName = "Loco_Neutral2converge_01";
static std::string kLookUpAndDownAnimName = "ID_converge2MHold_01";
static std::string kLookUpAnimName = "Loco_converge2Neutral_01";

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

Result BehaviorFollowMotion::InitInternal(Robot& robot, double currentTime_sec)
{

# if DO_BACK_UP_AFTER_POUNCE
  if( _initialReactionAnimPlayed ) {
      PRINT_NAMED_INFO("BehaviorFollowMotion.Init.CheckBackup",
                   "have driven %f forward",
                   _totalDriveForwardDist);
    _state = State::BackingUp;
    SetStateName("BackUp");
  }
# endif

  // Do the initial reaction for first motion each time we restart this behavior
  // (but only if it's been long enough since last interruption)
  if(currentTime_sec - _lastInterruptTime_sec > _initialReactionWaitTime_sec) {
    _initialReactionAnimPlayed = false;
    if( _state != State::BackingUp ) {
      _state = State::WaitingForFirstMotion;
      SetStateName("Wait");
    }
  } else {
    _initialReactionAnimPlayed = true;
    if( _state != State::BackingUp ) {
      StartTracking(robot);
      _state = State::Tracking;
      SetStateName("Tracking");
    }
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

    case State::BackingUp:
      if( _backingUpAction == (u32)ActionConstants::INVALID_TAG ) {
        float distToBackUp = _totalDriveForwardDist + _additionalBackupDist;
        if( distToBackUp > _maxBackupDistance ) {
          distToBackUp = _maxBackupDistance;
        }

        PRINT_NAMED_INFO("BehaviorFollowMotion.BackingUp",
                         "driving backwards %fmm to reset position",
                         distToBackUp);
        
        DriveStraightAction* backupAction = new DriveStraightAction(-distToBackUp, -_backupSpeed);
        _backingUpAction = backupAction->GetTag();
        robot.GetActionList().QueueActionNow(backupAction);

        _totalDriveForwardDist = 0.0f;
      }
      break;

    case State::HoldingHeadDown:
      LiftShouldBeLocked(robot);
      if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() >= _holdHeadDownUntil) {
        
        PlayAnimationAction* action = new PlayAnimationAction(kLookUpAnimName);
        _actionRunning = action->GetTag();
        
        // Note that queuing action "now" will cancel the tracking action
        robot.GetActionList().QueueActionNow(action);
        
        _state = State::BringingHeadUp;
        SetStateName("BringingHeadUp");
      }
      break;
      
    case State::BringingHeadUp:
    {
      // Just wait til the PlayAnimationAction we started in HoldingHeadDown is done, then transition to tracking 
      if (_actionRunning == (u32)ActionConstants::INVALID_TAG)
      {
        StartTracking(robot); // puts us in Tracking state
      }
      break;
    }
      
    case State::Tracking:
    {
      LiftShouldBeLocked(robot);

      // keep the lift out of the FOV
      if( !robot.GetMoveComponent().IsLiftMoving() && robot.GetLiftHeight() > LIFT_HEIGHT_LOWDOCK + 6.0f ) {
        MoveLiftToHeightAction* liftAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
        // use animation slot, since we know no one is using the lift
        robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, liftAction);
      }
      
      break;
    }
    
    case State::DrivingForward:
      LiftShouldBeLocked(robot);
      break;

    case State::WaitingForFirstMotion:
      break;

  } // switch(_state)
  
  return status;
}

Result BehaviorFollowMotion::InterruptInternal(Robot& robot, double currentTime_sec)
{
  _state = State::Interrupted;
  SetStateName("Interrupted");
  
  return Result::RESULT_OK;
}
  
void BehaviorFollowMotion::StopInternal(Robot& robot, double currentTime_sec)
{
  robot.GetActionList().Cancel(_actionRunning);
  
  _actionRunning = (u32)ActionConstants::INVALID_TAG;
  _lastInterruptTime_sec = currentTime_sec;
  _holdHeadDownUntil = -1.0f;

  LiftShouldBeUnlocked(robot);
  
  _state = State::Interrupted;
  SetStateName("Interrupted");
  
  LiftShouldBeUnlocked(robot);
}
  
void BehaviorFollowMotion::StartTracking(Robot& robot)
{
  TrackMotionAction* action = new TrackMotionAction();
  action->SetMaxHeadAngle( DEG_TO_RAD( 5.0f ) );
  action->SetMoveEyes(true);
  _actionRunning = action->GetTag();
  
  robot.GetActionList().QueueActionNow(action);
  
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
  
  switch(_state)
  {
    case State::WaitingForFirstMotion:
    {
      if(_actionRunning==0 && motionObserved.img_area > 0)
      {
        if(!_initialReactionAnimPlayed)
        {
          // Robot gets more happy/excited and less calm when he sees motion.
          robot.GetMoodManager().AddToEmotions(EmotionType::Happy,   kEmotionChangeSmall,
                                               EmotionType::Excited, kEmotionChangeSmall,
                                               EmotionType::Calm,   -kEmotionChangeSmall, "MotionReact", MoodManager::GetCurrentTimeInSeconds());
          
          // Turn to face the motion, also drop the lift, and lock it from animations
          PanAndTiltAction* panTiltAction = new PanAndTiltAction(relBodyPanAngle_rad,
                                                                 relHeadAngle_rad,
                                                                 false, false);
          MoveLiftToHeightAction* liftAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);

          PlayAnimationAction* reactAnimAction = new PlayAnimationAction("ID_MotionFollow_ReactToMotion");

          CompoundActionParallel* compoundAction = new CompoundActionParallel({panTiltAction, liftAction, reactAnimAction});
          
          LiftShouldBeLocked(robot);
      
      	  // Wait for the animation to complete
      	  _actionRunning = compoundAction->GetTag();
      
          robot.GetActionList().QueueActionNext(compoundAction);
        }
      } // if(_actionRunning==0 && motionObserved.img_area > 0)
      break;
    } // case State::WaitingForFirstMotion
  
    case State::Tracking:
    {
      PRINT_NAMED_INFO("BehaviorFollowMotion.HandleWhileRunning.Motion",
                       "Motion area=%.1f%%, centroid=(%.1f,%.1f), HeadTilt=%.1fdeg, BodyPan=%.1fdeg",
                       motionObserved.img_area * 100.f,
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
        
        if( belowMinGroundPlaneDist )
        {
          _holdHeadDownUntil = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _timeToHoldHeadDown_sec;
          PlayAnimationAction* lookDownAction = new PlayAnimationAction(kLookDownAnimName);
          _actionRunning = lookDownAction->GetTag();
          
          // Note that queuing action "now" will cancel the tracking action
          robot.GetActionList().QueueActionNow(lookDownAction);
          
          // After head gets down and before pouncing, play animation to glance up and down, looping
          PlayAnimationAction* glanceUpAndDownAction = new PlayAnimationAction(kLookUpAndDownAnimName, 0);
          robot.GetActionList().QueueActionNext(glanceUpAndDownAction);
          
          PRINT_NAMED_INFO("BehaviorFollowMotion.HoldingHeadLow",
                           "got %f of image with dist %f, holding head at min angle for %f sec",
                           motionObserved.ground_area,
                           groundPlaneDist,
                           _timeToHoldHeadDown_sec);
          
          _state = State::HoldingHeadDown;
          SetStateName("HeadDown");
        }
      } // if(motion in ground plane)
      
      else if (relHeadAngle_rad.getAbsoluteVal() < _driveForwardTol &&
               relBodyPanAngle_rad.getAbsoluteVal() < _driveForwardTol )
      {
        // Move towards the motion since it's centered
        DriveStraightAction* driveAction = new DriveStraightAction(_moveForwardDist_mm,
                                                                   DEFAULT_PATH_SPEED_MMPS*_moveForwardSpeedIncrease);
        driveAction->SetAccel(DEFAULT_PATH_ACCEL_MMPS2*_moveForwardSpeedIncrease);
        _actionRunning = driveAction->GetTag();
        
        _totalDriveForwardDist += _moveForwardDist_mm;
        
        _state = State::DrivingForward;
        SetStateName("DriveForward");
        
        // Queue action now will stop the tracking that's currently running
        robot.GetActionList().QueueActionNow(driveAction);
        
        // PRINT_NAMED_DEBUG("BehaviorFollowMotion.DriveForward",
        //                   "relHeadAngle = %fdeg, relBodyAngle = %fdeg, ground area %f",
        //                   RAD_TO_DEG(relHeadAngle_rad.ToFloat()),
        //                   RAD_TO_DEG(relBodyPanAngle_rad.ToFloat()),
        //                   motionObserved.ground_area);
        
      } // else if(time to drive forward)
      
      break;
    } // case State::Tracking
 
    case State::HoldingHeadDown:
    {
      // Keep head held down another increment of time while ground plane motion is
      // observed -- at some point we are hoping for Pounce behavior to trigger in here
      const bool inGroundPlane = motionObserved.ground_area > _minGroundAreaToConsider;
      if(inGroundPlane)
      {
        _holdHeadDownUntil = event.GetCurrentTime() + _timeToHoldHeadDown_sec;
      } // if(inGroundPlane)
      
      break;
    } // case State::HoldingHeadDown
      
    default:
      // Nothing to do for other states
      break;
  } // switch(_state)
  
} // HandleObservedMotion()

  
void BehaviorFollowMotion::HandleCompletedAction(const EngineToGameEvent &event, Robot& robot)
{
  auto const& completedAction = event.GetData().Get_RobotCompletedAction();
  
  // If the action we were running completes, allow us to respond to motion again
  if(completedAction.idTag == _actionRunning) {
    _actionRunning = (u32)ActionConstants::INVALID_TAG;
    
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
        if( completedAction.actionType != RobotActionType::PLAY_ANIMATION ){
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

  if( _state == State::BackingUp && completedAction.idTag == _backingUpAction ) {
    _backingUpAction = (u32)ActionConstants::INVALID_TAG;
    PRINT_NAMED_INFO("BehaviorFollowMotion.BackupComplete", "");
    if( _initialReactionAnimPlayed ) {
      StartTracking(robot);
      _state = State::Tracking;
      SetStateName("Tracking");
    }
    else {
      _state = State::WaitingForFirstMotion;
      SetStateName("Wait");
    }
  }
  
} // HandleCompletedAction()

void BehaviorFollowMotion::LiftShouldBeLocked(Robot& robot)
{
  if( ! _lockedLift ) {
    robot.GetMoveComponent().LockTracks(static_cast<u8>(AnimTrackFlag::LIFT_TRACK));
    _lockedLift = true;
  }
}

void BehaviorFollowMotion::LiftShouldBeUnlocked(Robot& robot)
{
  if( _lockedLift ) {
    robot.GetMoveComponent().UnlockTracks(static_cast<u8>(AnimTrackFlag::LIFT_TRACK));
    _lockedLift = false;
  }
}

  

} // namespace Cozmo
} // namespace Anki
