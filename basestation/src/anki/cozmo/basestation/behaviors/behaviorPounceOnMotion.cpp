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
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;

// TODO:(bn) add animation for face / sound

BehaviorPounceOnMotion::BehaviorPounceOnMotion(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _maxPounceDist(80.0f)
  , _minGroundAreaForPounce(0.01f)
  , _prePouncePitch(0.0f)
  , _lastValidPouncePose(0.0f)
  , _numValidPouncePoses(0)
  , _state(State::Inactive)
  , _waitForActionTag(0)
  , _stopRelaxingTime(0.0f)
{
  _name = "PounceOnMotion";

  std::vector<MessageEngineToGameTag> subscribedEvents = {
    MessageEngineToGameTag::RobotObservedMotion,
    MessageEngineToGameTag::RobotCompletedAction,
  };
  
  SubscribeToTags(std::move(subscribedEvents));
}

bool BehaviorPounceOnMotion::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  // we can only run if there is a pounce pose to pounce on
  return _numValidPouncePoses > 1 || _state != State::Inactive;
}

float BehaviorPounceOnMotion::EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const
{
  // TODO:(bn) emotion stuff here?

  // if we are running, always give a score of 1.0 so we keep running
  if( _state != State::Inactive ) {
    return 1.0f;
  }
  
  // give 0 score if it has been too long
  if( currentTime_sec - _lastValidPouncePose > 1.5f ) {
    return 0.0f;
  }
  
  // current hack: 1.0 when we've got 5, linearly down from there
  int fullScoreNum = 5;
  if( _numValidPouncePoses > fullScoreNum ) {
    return 1.0f;
  }
  
  return _numValidPouncePoses * (1.0 / fullScoreNum);
}


void BehaviorPounceOnMotion::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  if( _state != State::Inactive ) {
    // this will be handled by HandleWhileRunning
    return;
  }
  
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
      const bool inGroundPlane = motionObserved.ground_area > _minGroundAreaForPounce;
      const float robotOffsetX = motionObserved.ground_x;
      const float robotOffsetY = motionObserved.ground_y;
      // const u32 imageTimestamp = motionObserved.timestamp;

      const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      
      bool gotPose = false;
      
      // we haven't started the pounce, so update the pounce location
      if( inGroundPlane ) {
        float dist = std::sqrt( std::pow( robotOffsetX, 2 ) + std::pow( robotOffsetY, 2) );
        if( dist <= _maxPounceDist ) {
          gotPose = true;
          _numValidPouncePoses++;
          _lastValidPouncePose = currTime;
          PRINT_NAMED_INFO("BehaviorPounceOnMotion.GotPose", "got valid pose with dist = %f. Now have %d",
                           dist, _numValidPouncePoses);
        }
        else {
          PRINT_NAMED_INFO("BehaviorPounceOnMotion.IgnorePose", "got pose, but dist of %f is too large, ignoring",
                           dist);
        }
      }
      else {
        PRINT_NAMED_INFO("BehaviorPounceOnMotion.IgnorePose", "got pose, but ground plane area is %f, which is too low",
                         motionObserved.ground_area);
      }

      // reset everything if it's been this long since we got a valid pose
      const float abortTime = 4.0f;
      
      if( ! gotPose && currTime >= _lastValidPouncePose + abortTime ) {
        if( _numValidPouncePoses > 0 ) {
          PRINT_NAMED_INFO("BehaviorPounceOnMotion.ResetValid",
                           "resetting num valid poses because it has been %f seconds since the last one",
                           currTime - _lastValidPouncePose);
          _numValidPouncePoses = 0;
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
  if( _state == State::Inactive ) {
    // this will be handled by AlwaysHandle
    return;
  }

  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      // don't update the pounce location while we are active
      break;
    }

    case MessageEngineToGameTag::RobotCompletedAction: {
      if( _waitForActionTag == event.GetData().Get_RobotCompletedAction().idTag ) {
        if( _state == State::Pouncing ) {
          PRINT_NAMED_INFO("BehaviorPounceOnMotion.RelaxLift", "pounce animation complete, relaxing lift");
          robot.GetMoveComponent().EnableLiftPower(false); // TEMP: make sure this gets cleaned up
          _state = State::RelaxingLift;
          const float relaxTime = 0.1f;
          _stopRelaxingTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + relaxTime;
        }
        else if( _state == State::PlayingFinalReaction ) {
          PRINT_NAMED_INFO("BehaviorPounceOnMotion.Complete", "post-pounce reaction animation complete");
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
  _lastValidPouncePose = 0.0f;
}

  
Result BehaviorPounceOnMotion::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
  if( _numValidPouncePoses == 0 ) {
    PRINT_NAMED_WARNING("BehaviorPounceOnMotion.Init.NoPouncePose", "");
    return Result::RESULT_FAIL;
  }

  _prePouncePitch = robot.GetPitchAngle();
  
  // TEMP: 
  std::string _pounceAnimation = "invDemo_P05_CatchFwd"; // TEMP: 
  
  _state = State::Pouncing;
  IActionRunner* newAction = new PlayAnimationAction( _pounceAnimation );
  _waitForActionTag = newAction->GetTag();
  robot.GetActionList().QueueActionNow(0, newAction);

  return Result::RESULT_OK;
}

void BehaviorPounceOnMotion::CheckPounceResult(Robot& robot)
{
  // Arbitrarily tuned for robot A0
  const float liftHeightThresh = 36.5f;
  const float bodyAngleThresh = 0.025f;

  float robotBodyAngleDelta = robot.GetPitchAngle() - _prePouncePitch;
    
  // check the lift angle, after some time, transition state
  // TEMP:  // TODO:(bn) do this
  PRINT_NAMED_INFO("BehaviorPounceOnMotion.CheckResult", "lift: %f body: %f (%f -> %f)",
                   robot.GetLiftHeight(),
                   robotBodyAngleDelta,
                   _prePouncePitch,
                   robot.GetPitchAngle());

  bool caught = robot.GetLiftHeight() > liftHeightThresh || robotBodyAngleDelta > bodyAngleThresh;
  
  IActionRunner* newAction = nullptr;
  if( caught ) {
    newAction = new PlayAnimationAction( "happy" );
    PRINT_NAMED_INFO("BehaviorPounceOnMotion.CheckResult.Caught", "got it!");
  }
  else {
    newAction = new PlayAnimationAction( "sad" );
    PRINT_NAMED_INFO("BehaviorPounceOnMotion.CheckResult.Miss", "missed...");
  }
  
  _waitForActionTag = newAction->GetTag();
  robot.GetActionList().QueueActionNow(0, newAction);
  _state = State::PlayingFinalReaction;
  
  robot.GetMoveComponent().EnableLiftPower(true);
}

IBehavior::Status BehaviorPounceOnMotion::UpdateInternal(Robot& robot, double currentTime_sec)
{

  float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  switch( _state ) {
    case State::Inactive: {
      return Status::Complete;
    }
    case State::RelaxingLift: {
      if( currTime > _stopRelaxingTime ) {
        CheckPounceResult(robot);
      }
      break;
    }

    case State::Complete: {
      PRINT_NAMED_INFO("BehaviorPounceOnMotion.Complete", "");
      // move the head and lift back down the the default positions
      robot.GetActionList().QueueActionNext(0, new MoveHeadToAngleAction(0));
      robot.GetActionList().QueueActionNext(0, new MoveLiftToHeightAction( MoveLiftToHeightAction::Preset::LOW_DOCK ));
      Cleanup(robot);
      return Status::Complete;
    }

    default: break;
  }
  
  return Status::Running;
}

Result BehaviorPounceOnMotion::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  // We don't want to be interrupted unless we're done reacting
  if( _state == State::Inactive ) {
    PRINT_NAMED_INFO("BehaviorPounceOnMotion.Interrupt", "stopping behavior");
    Cleanup(robot);
    return Result::RESULT_OK;
  }

  return Result::RESULT_FAIL;
}


}
}
