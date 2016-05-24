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
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;
  
static const std::string s_PounceAnimGroup        = "ag_pounceOnMotionPounce";
static const std::string s_PounceSuccessAnimGroup = "ag_pounceOnMotionSuccess";
static const std::string s_PounceFailAnimGroup    = "ag_pounceOnMotionFail";
static const std::string s_pounceDriveFaceAnimGroup = "ag_pounceDriveFace";
static const std::string s_PounceInitalWarningAnimGroup = "ag_pounceWarningInitial";
static const char* kMaxNoMotionBeforeBored_Sec    = "maxNoGroundMotionBeforeBored_Sec";
static const char* kTimeBeforeRotate_Sec          = "TimeBeforeRotate_Sec";
// combination of offset between lift and robot orign and motion built into animation
static constexpr float kDriveForwardUntilDist = 31.f;
static constexpr float kAnimationMotionPadding = 15.f;

BehaviorPounceOnMotion::BehaviorPounceOnMotion(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("PounceOnMotion");

  
  SubscribeToTags({{
    EngineToGameTag::RobotObservedMotion,
    EngineToGameTag::RobotCompletedAction
  }});

  _maxTimeSinceNoMotion_sec = config.get(kMaxNoMotionBeforeBored_Sec, _maxTimeSinceNoMotion_sec).asFloat();
  _maxTimeBeforeRotate = config.get(kTimeBeforeRotate_Sec, _maxTimeBeforeRotate).asFloat();
  
  SET_STATE(Inactive);
  _lastMotionTime = -1000.f;
}

bool BehaviorPounceOnMotion::IsRunnableInternal(const Robot& robot) const
{
  // Can always run now. Used to be more motion based.
  return true;
}

float BehaviorPounceOnMotion::EvaluateScoreInternal(const Robot& robot) const
{
  // more likely to run if we did happen to see ground motion recently.
  // This isn't likely unless cozmo is looking down in explore mode, but possible
  float multiplier = 1.f;
  if( !IsRunning() )
  {
    double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ( _lastMotionTime + _maxTimeSinceNoMotion_sec > currentTime_sec )
    {
      multiplier = 1.2f;
    }
  }
  return IBehavior::EvaluateScoreInternal(robot) * multiplier;
}


void BehaviorPounceOnMotion::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      // be more likely to run with observed motion
      const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
      const bool inGroundPlane = motionObserved.ground_area > _minGroundAreaForPounce;
      if( inGroundPlane )
      {
        const float robotOffsetX = motionObserved.ground_x;
        const float robotOffsetY = motionObserved.ground_y;
        float distSquared = robotOffsetX * robotOffsetX + robotOffsetY * robotOffsetY;
        float maxDistSquared = _maxPounceDist * _maxPounceDist;
        if( distSquared <= maxDistSquared )
        {
          double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          _lastMotionTime = (float)currentTime_sec;
        }
      }
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
            TransitionToTurnToMotion(robot, motionObserved.img_x, motionObserved.img_y);
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
        if( _state == State::InitialAnim)
        {
          TransitionToBringingHeadDown(robot);
        }
        else if( _state == State::BringingHeadDown || _state == State::RotateToWatchingNewArea )
        {
          TransitionToWaitForMotion(robot);
        }
        else if( _state == State::TurnToMotion)
        {
          TransitionToPounce(robot);
        }
        else if( _state == State::Pouncing )
        {
          TransitionToRelaxLift(robot);
        }
        else if( _state == State::PlayingFinalReaction )
        {
          EnableCliffReacts(true,robot);
          // We exit out of this mode during the update when we time out so just start waiting for motion again after backing up
          _numValidPouncePoses = 0; // wait until we're seeing motion again
          if( GetDriveDistance() > 0.f )
          {
            TransitionToBackUp(robot);
          }
          else
          {
            TransitionToBringingHeadDown(robot);
          }
        }
        else if( _state == State::BackUp )
        {
          TransitionToBringingHeadDown(robot);
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

void BehaviorPounceOnMotion::TransitionToInitialWarningAnim(Robot& robot)
{
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.TransitionToInitialWarningAnim","BehaviorPounceOnMotion.TransitionToInitialWarningAnim");
  SET_STATE(InitialAnim);
  IActionRunner* animAction = new PlayAnimationGroupAction(robot, s_PounceInitalWarningAnimGroup);
  robot.GetActionList().QueueActionAtEnd(animAction);
  _waitForActionTag = animAction->GetTag();
}

void BehaviorPounceOnMotion::TransitionToBringingHeadDown(Robot& robot)
{
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.TransitionToBringingHeadDown","BehaviorPounceOnMotion.TransitionToBringingHeadDown");
  SET_STATE(BringingHeadDown);
  Radians tiltRads;
  tiltRads.setDegrees(-10.f);
  IActionRunner* moveAction = new PanAndTiltAction(robot, 0, tiltRads, false, true);
  robot.GetActionList().QueueActionAtEnd(moveAction);
  _waitForActionTag = moveAction->GetTag();
}
  
void BehaviorPounceOnMotion::TransitionToRotateToWatchingNewArea(Robot& robot)
{
  SET_STATE( RotateToWatchingNewArea );

  constexpr static float kPanMin_Deg = 20;
  constexpr static float kPanMax_Deg = 60;
  Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(kPanMin_Deg,kPanMax_Deg)));
  // other direction half the time
  if( GetRNG().RandDbl() < 0.5 )
  {
    panAngle *= -1.f;
  }
  PanAndTiltAction* turnAction = new PanAndTiltAction(robot, panAngle, 0, false, false);
  robot.GetActionList().QueueActionAtEnd(turnAction);
  _waitForActionTag = turnAction->GetTag();
}
  
void BehaviorPounceOnMotion::TransitionToWaitForMotion(Robot& robot)
{
  SET_STATE( WaitingForMotion);
  _lastTimeRotate = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}
  
void BehaviorPounceOnMotion::TransitionToTurnToMotion(Robot& robot, int16_t motion_img_x, int16_t motion_img_y)
{
  SET_STATE(TurnToMotion);
  
  const Point2f motionCentroid(motion_img_x, motion_img_y);
  Radians absPanAngle;
  Radians absTiltAngle;
  robot.GetVisionComponent().GetCamera().ComputePanAndTiltAngles(motionCentroid, absPanAngle, absTiltAngle);
  PanAndTiltAction* turnAction = new PanAndTiltAction(robot, absPanAngle, 0, false, false);
  robot.GetActionList().QueueActionAtEnd(turnAction);
  _waitForActionTag = turnAction->GetTag();
}

float BehaviorPounceOnMotion::GetDriveDistance()
{
  return _lastPoseDist - kDriveForwardUntilDist;
}

void BehaviorPounceOnMotion::TransitionToPounce(Robot& robot)
{
  SET_STATE(Pouncing);
  
  IActionRunner* animAction = new PlayAnimationGroupAction(robot, s_PounceAnimGroup);
  IActionRunner* actionToRun = animAction;
  
  if( _lastPoseDist > kDriveForwardUntilDist + 5.0f ) {
    
    float distToDrive = GetDriveDistance();
    
    PRINT_NAMED_INFO("BehaviorPounceOnMotion.SprintForward",
                     "driving forward %fmm before playing pounce animation",
                     distToDrive);
    // We want a smooth transition to pounce so don't use DriveStraightAction which will do a stop then the anim.
    // Instead turn on the wheels, and then animation will take over naturally and cause us to lock the body track and stop.
    float driveSpeed = MAX_WHEEL_SPEED_MMPS;
    
    float driveTime = (distToDrive - kAnimationMotionPadding) / driveSpeed;
    robot.SendRobotMessage<RobotInterface::DriveWheels>(driveSpeed,driveSpeed,0.f,0.f);
    
    // We also want a different keep alive during this time
    IActionRunner* faceDriveAnim = new PlayAnimationGroupAction(robot, s_pounceDriveFaceAnimGroup, 10, true);
    robot.GetActionList().AddConcurrentAction(faceDriveAnim);
    
    u32 faceDriveID = faceDriveAnim->GetTag();
    IActionRunner* driveAction = new WaitAction(robot,driveTime);
    driveAction->AddCompletionCallback([this,&robot,faceDriveID](ActionResult result)
                                        {
                                          if( result != ActionResult::SUCCESS)
                                          {
                                            robot.SendRobotMessage<RobotInterface::DriveWheels>(0.f,0.f,0.f,0.f);
                                          }
                                          robot.GetActionList().Cancel(faceDriveID);
                                          EnableCliffReacts(false,robot);
                                        }
                                       );
    actionToRun = new CompoundActionSequential(robot, {driveAction, animAction});
  }
  else
  {
    // Don't disable cliff reacts until the animation actually starts.
    // when we're driving forward is when cliff reacts are most likely
    EnableCliffReacts(false,robot);
  }
  
  _waitForActionTag = actionToRun->GetTag();
  robot.GetActionList().QueueActionNow(actionToRun);
}
  
void BehaviorPounceOnMotion::TransitionToRelaxLift(Robot& robot)
{
  robot.GetMoveComponent().EnableLiftPower(false); // TEMP: make sure this gets cleaned up
  SET_STATE(RelaxingLift);
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
  SET_STATE(PlayingFinalReaction);
  
  robot.GetMoveComponent().EnableLiftPower(true);
}
  
void BehaviorPounceOnMotion::TransitionToBackUp(Robot& robot)
{
  SET_STATE(BackUp);
  // back up some of the way
  IActionRunner* driveAction = new DriveStraightAction(robot, -GetDriveDistance() * 0.75f, 75.0f);
  _waitForActionTag = driveAction->GetTag();
  robot.GetActionList().QueueActionNow(driveAction);
}
  
void BehaviorPounceOnMotion::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}
  
Result BehaviorPounceOnMotion::InitInternal(Robot& robot)
{
  _prePouncePitch = robot.GetPitchAngle();
  
  double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastMotionTime = (float)currentTime_sec;
  
  TransitionToInitialWarningAnim(robot);
  
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorPounceOnMotion::UpdateInternal(Robot& robot)
{
  float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  switch( _state )
  {
    case State::Inactive:
    {
      return Status::Complete;
    }
    case State::WaitingForMotion:
    {
      // We're done if we haven't seen motion in a long while or since start.
      if ( _lastMotionTime + _maxTimeSinceNoMotion_sec < currentTime_sec )
      {
        PRINT_NAMED_INFO("BehaviorPounceOnMotion.Timeout", "No motion found, giving up");
        SET_STATE(Complete);
      }
      else if ( _lastTimeRotate + _maxTimeBeforeRotate < currentTime_sec )
      {
        TransitionToRotateToWatchingNewArea(robot);
      }
      break;
    }
    case State::RelaxingLift:
    {
      if( currentTime_sec > _stopRelaxingTime )
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
  
void  BehaviorPounceOnMotion::EnableCliffReacts(bool enable,Robot& robot)
{
  if( _cliffReactEnabled && !enable )
  {
    robot.GetBehaviorManager().GetWhiteboard().DisableCliffReaction(this);
  }
  else if( !_cliffReactEnabled && enable )
  {
    robot.GetBehaviorManager().GetWhiteboard().RequestEnableCliffReaction(this);
  }
  
  _cliffReactEnabled = enable;
}
  
void BehaviorPounceOnMotion::Cleanup(Robot& robot)
{
  if( _state == State::RelaxingLift) {
    robot.GetMoveComponent().EnableLiftPower(true);
  }
  
  EnableCliffReacts(true,robot);
  
  SET_STATE(Inactive);
  _numValidPouncePoses = 0;
  _lastValidPouncePoseTime = 0.0f;
}


}
}
