/**
 * File: behaviorPounceOnMotion.cpp
 *
 * Author: Brad Neuman
 * Created: 2015-11-19
 *
 * Description: This is a reactionary behavior which "pounces". Basically, it looks for motion nearby in the
 *              ground plane, and then drive to drive towards it and "catch" it underneath it's lift
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviors/behaviorPounceOnMotion.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;
  
static const std::string s_PounceAnimGroup        = "ag_pounceOnMotionPounce";
static const std::string s_PounceSuccessAnimGroup = "ag_pounceOnMotionSuccess";
static const std::string s_PounceFailAnimGroup    = "ag_pounceOnMotionFail";
static const std::string s_PounceInitalWarningAnimGroup = "ag_pounceWarningInitial";
static const char* kMaxNoMotionBeforeBored_Sec    = "maxNoGroundMotionBeforeBored_Sec";
static const char* kBackUpDistance                = "backUpDistance";

BehaviorPounceOnMotion::BehaviorPounceOnMotion(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("PounceOnMotion");
  
  SubscribeToTags({{
    EngineToGameTag::RobotObservedMotion,
    EngineToGameTag::RobotCompletedAction
  }});

  _maxTimeSinceNoMotion_sec = config.get(kMaxNoMotionBeforeBored_Sec, _maxTimeSinceNoMotion_sec).asFloat();
  _backUpDistance = config.get(kBackUpDistance, _backUpDistance).asFloat();
}

bool BehaviorPounceOnMotion::IsRunnableInternal(const Robot& robot) const
{
  // Can run whenever
  return true;
}

float BehaviorPounceOnMotion::EvaluateScoreInternal(const Robot& robot) const
{
  if( _state != State::Inactive )
  {
    return 1.f;
  }
  double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // We're done if we haven't seen motion in a long while or since start.
  if ( _lastMotionTime < currentTime_sec + SEC_TO_MILIS(_maxTimeSinceNoMotion_sec))
  {
    return IBehavior::EvaluateScoreInternal(robot);
  }
  
  return 0.f;
}


void BehaviorPounceOnMotion::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      break;
    }

    case MessageEngineToGameTag::RobotCompletedAction: {
      break;
    }

    default: {
      PRINT_NAMED_ERROR("BehaviorPounceOnMotion.AlwaysHandle.InvalidEvent", "");
      break;
    }
  }
}

void BehaviorPounceOnMotion::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      // don't update the pounce location while we are active but go back.
      const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
      const bool inGroundPlane = motionObserved.ground_area > _minGroundAreaForPounce;
      
      double currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if( inGroundPlane )
      {
        _lastMotionTime = (float)currTime;
      }
      if( _state == State::WaitingForMotion )
      {
        
        const float robotOffsetX = motionObserved.ground_x;
        const float robotOffsetY = motionObserved.ground_y;
        // const u32 imageTimestamp = motionObserved.timestamp;
        
        bool gotPose = false;
        
        // we haven't started the pounce, so update the pounce location
        if( inGroundPlane )
        {
          float dist = std::sqrt( std::pow( robotOffsetX, 2 ) + std::pow( robotOffsetY, 2) );
          if( dist <= _maxPounceDist )
          {
            gotPose = true;
            _numValidPouncePoses++;
            _lastValidPouncePoseTime = currTime;
            _lastPoseDist = dist;
            PRINT_NAMED_INFO("BehaviorPounceOnMotion.GotPose", "got valid pose with dist = %f. Now have %d",
                             dist, _numValidPouncePoses);
          }
          else
          {
            PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.IgnorePose", "got pose, but dist of %f is too large, ignoring",
                              dist);
          }
        }
        else if(_numValidPouncePoses > 0)
        {
          PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.IgnorePose", "got pose, but ground plane area is %f, which is too low",
                            motionObserved.ground_area);
        }
        
        // reset everything if it's been this long since we got a valid pose
        if( ! gotPose && currTime >= _lastValidPouncePoseTime + _maxTimeBetweenPoses ) {
          if( _numValidPouncePoses > 0 ) {
            PRINT_NAMED_INFO("BehaviorPounceOnMotion.ResetValid",
                             "resetting num valid poses because it has been %f seconds since the last one",
                             currTime - _lastValidPouncePoseTime);
            _numValidPouncePoses = 0;
          }
        }
      }
      break;
    }

    case MessageEngineToGameTag::RobotCompletedAction: {
      if( _waitForActionTag == event.GetData().Get_RobotCompletedAction().idTag ) {
        // restore idle
        robot.SetIdleAnimation(_previousIdleAnimation);
        if( _state == State::InitialAnim)
        {
          TransitionToBringingHeadDown(robot);
        }
        else if( _state == State::BringingHeadDown)
        {
          TransitionToWaitForMotion(robot);
        }
        else if( _state == State::Pouncing )
        {
          TransitionToRelaxLift(robot);
        }
        else if( _state == State::PlayingFinalReaction )
        {
          TransitionToBackUp(robot);
        }
        else if( _state == State::BackUp)
        {
          PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.Complete", "post-pounce reaction animation complete");
          _state = State::Complete;
        }
      }
      break;
    }

    default: {
      PRINT_NAMED_ERROR("BehaviorPounceOnMotion.AlwaysHandle.InvalidEvent", "");
      break;
    }
  }
}

void BehaviorPounceOnMotion::Cleanup(Robot& robot)
{
  if( _state == State::RelaxingLift) {
    robot.GetMoveComponent().EnableLiftPower(true);
  }

  _state = State::Inactive;
  _numValidPouncePoses = 0;
  _lastValidPouncePoseTime = 0.0f;
}

  
Result BehaviorPounceOnMotion::InitInternal(Robot& robot)
{
  _prePouncePitch = robot.GetPitchAngle();
  
  double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastMotionTime = (float)currentTime_sec;
  
  TransitionToInitialWarningAnim(robot);
  
  return Result::RESULT_OK;
}

void BehaviorPounceOnMotion::TransitionToInitialWarningAnim(Robot& robot)
{
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.TransitionToInitialWarningAnim","BehaviorPounceOnMotion.TransitionToInitialWarningAnim");
  _state = State::InitialAnim;
  IActionRunner* animAction = new PlayAnimationGroupAction(robot, s_PounceInitalWarningAnimGroup);
  robot.GetActionList().QueueActionAtEnd(animAction);
  _waitForActionTag = animAction->GetTag();
}

void BehaviorPounceOnMotion::TransitionToBringingHeadDown(Robot& robot)
{
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.TransitionToBringingHeadDown","BehaviorPounceOnMotion.TransitionToBringingHeadDown");
  _state = State::BringingHeadDown;
  Radians tiltRads;
  tiltRads.setDegrees(-10.f);
  IActionRunner* moveAction = new PanAndTiltAction(robot, 0, tiltRads, false, true);
  robot.GetActionList().QueueActionAtEnd(moveAction);
  _waitForActionTag = moveAction->GetTag();
}
  
void BehaviorPounceOnMotion::TransitionToWaitForMotion(Robot& robot)
{
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.TransitionToWaitForMotion","BehaviorPounceOnMotion.TransitionToWaitForMotion");
  _state = State::WaitingForMotion;
}
void BehaviorPounceOnMotion::TransitionToPounce(Robot& robot)
{
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.TransitionToPounce", "Motion found, doing pounce");
  _state = State::Pouncing;
  IActionRunner* animAction = new PlayAnimationGroupAction(robot, s_PounceAnimGroup);
  
  IActionRunner* actionToRun = animAction;
  
  if( _lastPoseDist > _driveForwardUntilDist + 5.0f ) {
    
    float distToDrive = _lastPoseDist - _driveForwardUntilDist;
    
    PRINT_NAMED_INFO("BehaviorPounceOnMotion.SprintForward",
                     "driving forward %fmm before playing pounce animation",
                     distToDrive);
    
    IActionRunner* driveAction = new DriveStraightAction(robot, distToDrive, 150.0f);
    actionToRun = new CompoundActionSequential(robot, {driveAction, animAction});
  }
  
  _waitForActionTag = actionToRun->GetTag();
  robot.GetActionList().QueueActionNow(actionToRun);
}
  
void BehaviorPounceOnMotion::TransitionToRelaxLift(Robot& robot)
{
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.RelaxLift", "pounce animation complete, relaxing lift");
  robot.GetMoveComponent().EnableLiftPower(false); // TEMP: make sure this gets cleaned up
  _state = State::RelaxingLift;
  const float relaxTime = 0.15f;
  _stopRelaxingTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + relaxTime;
}

void BehaviorPounceOnMotion::TransitionToResultAnim(Robot& robot)
{
  // Arbitrarily tuned for robot A0
  const float liftHeightThresh = 37.5f;
  const float bodyAngleThresh = 0.02f;

  float robotBodyAngleDelta = robot.GetPitchAngle() - _prePouncePitch;
    
  // check the lift angle, after some time, transition state
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.CheckResult", "lift: %f body: %fdeg (%frad) (%f -> %f)",
                    robot.GetLiftHeight(),
                    RAD_TO_DEG(robotBodyAngleDelta),
                    robotBodyAngleDelta,
                    RAD_TO_DEG(_prePouncePitch),
                    RAD_TO_DEG(robot.GetPitchAngle()));

  bool caught = robot.GetLiftHeight() > liftHeightThresh || robotBodyAngleDelta > bodyAngleThresh;

  // disable idle while animation plays
  _previousIdleAnimation = robot.GetIdleAnimationName();
  robot.SetIdleAnimation("NONE");

  IActionRunner* newAction = nullptr;
  if( caught ) {
    newAction = new PlayAnimationGroupAction(robot, s_PounceSuccessAnimGroup);
    PRINT_NAMED_INFO("BehaviorPounceOnMotion.CheckResult.Caught", "got it!");
  }
  else {
    newAction = new PlayAnimationGroupAction(robot, s_PounceFailAnimGroup );
    PRINT_NAMED_INFO("BehaviorPounceOnMotion.CheckResult.Miss", "missed...");
  }
  
  _waitForActionTag = newAction->GetTag();
  robot.GetActionList().QueueActionNow(newAction);
  _state = State::PlayingFinalReaction;
  
  robot.GetMoveComponent().EnableLiftPower(true);
}
  
void BehaviorPounceOnMotion::TransitionToBackUp(Robot& robot)
{
  _state = State::BackUp;
  IActionRunner* driveAction = new DriveStraightAction(robot, _backUpDistance, -50.0f);
  _waitForActionTag = driveAction->GetTag();
  robot.GetActionList().QueueActionNow(driveAction);
}

IBehavior::Status BehaviorPounceOnMotion::UpdateInternal(Robot& robot)
{

  float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  switch( _state )
  {
    case State::Inactive:
    {
      return Status::Complete;
    }
    case State::WaitingForMotion:
    {
      if( _numValidPouncePoses > 0 ) {
        TransitionToPounce(robot);
      }
      break;
    }
    case State::RelaxingLift:
    {
      if( currTime > _stopRelaxingTime )
      {
        TransitionToResultAnim(robot);
      }
      break;
    }

    case State::Complete:
    {
      PRINT_NAMED_INFO("BehaviorPounceOnMotion.Complete", "");
      Cleanup(robot);
      return Status::Complete;
    }

    default: break;
  }
  
  return Status::Running;
}
  
void BehaviorPounceOnMotion::StopInternal(Robot& robot)
{
  Cleanup(robot);
}


}
}
