/**
 * File: behaviorFindFaces.cpp
 *
 * Author: Lee Crippen
 * Created: 12/22/15
 *
 * Description: Behavior for rotating around to find faces.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorFindFaces.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/emotionScorer.h"
#include "util/math/math.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/actionTypes.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

BehaviorFindFaces::BehaviorFindFaces(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentDriveActionID((uint32_t)ActionConstants::INVALID_TAG)
{
  SetDefaultName("FindFaces");
  
  SubscribeToTags({{
    EngineToGameTag::RobotCompletedAction,
    EngineToGameTag::RobotPickedUp
  }});

  if (GetEmotionScorerCount() == 0)
  {
    // Boredom and loneliness -> LookForFaces
    AddEmotionScorer(EmotionScorer(EmotionType::Excited, Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 0.7f}, {1.0f, 0.05f}}), false));
    AddEmotionScorer(EmotionScorer(EmotionType::Social,  Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 0.7f}, {1.0f, 0.05f}}), false));
  }
}
  
bool BehaviorFindFaces::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  return true;
}

void BehaviorFindFaces::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotCompletedAction:
    {
      const RobotCompletedAction& msg = event.GetData().Get_RobotCompletedAction();
      if (msg.idTag == _currentDriveActionID)
      {
        _currentDriveActionID = (uint32_t)ActionConstants::INVALID_TAG;
      }
      break;
    }
    case EngineToGameTag::RobotPickedUp:
    {
      // Handled in AlwaysHandle
      break;
    }
    default:
      PRINT_NAMED_ERROR("BehaviorLookAround.HandleWhileRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
}
  
void BehaviorFindFaces::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  if (EngineToGameTag::RobotPickedUp == event.GetData().GetTag())
  {
    _faceAngleCenterSet = false;
  }
}
  
Result BehaviorFindFaces::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
  _currentDriveActionID = (uint32_t)ActionConstants::INVALID_TAG;
  _currentState = State::WaitToFinishMoving;
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorFindFaces::UpdateInternal(Robot& robot, double currentTime_sec)
{
  // First time we're updating, set our face angle center
  if (!_faceAngleCenterSet)
  {
    _faceAngleCenter = robot.GetPose().GetRotationAngle();
    _faceAngleCenterSet = true;
  }
  
  switch (_currentState)
  {
    case State::Inactive:
    {
      return Status::Complete;
    }
    case State::StartMoving:
    {
      StartMoving(robot);
      return Status::Running;
    }
    case State::WaitToFinishMoving:
    {
      if (_currentDriveActionID == (uint32_t)ActionConstants::INVALID_TAG)
      {
        _lookPauseTimer = currentTime_sec + (GetRNG().RandDbl() * (kPauseMax_sec - kPauseMin_sec) + kPauseMin_sec);
        _currentState = State::PauseToSee;
      }
      return Status::Running;
    }
    case State::PauseToSee:
    {
      if (currentTime_sec >= _lookPauseTimer)
      {
        _currentState = State::StartMoving;
      }
      return Status::Running;
    }
    default:
    {
      PRINT_NAMED_ERROR("LookAround_Behavior.Update.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  
  return Status::Complete;
}
  
float BehaviorFindFaces::GetRandomPanAmount() const
{
  float randomPan = (float) GetRNG().RandDbl(2.0);
  float panRads;
  // > 1 means positive pan
  if (randomPan > 1.0f)
  {
    randomPan = (randomPan - 1.0f);
    panRads = DEG_TO_RAD(((kPanMax - kPanMin) * randomPan) + kPanMin);
  }
  else
  {
    panRads = -DEG_TO_RAD(((kPanMax - kPanMin) * randomPan) + kPanMin);
  }
  
  return panRads;
}
  
void BehaviorFindFaces::StartMoving(Robot& robot)
{
  Radians currentBodyAngle = robot.GetPose().GetRotationAngle().ToFloat();
  float turnAmount = GetRandomPanAmount();
  
  Radians proposedNewAngle = Radians(currentBodyAngle + turnAmount);
  // If the potential turn takes us outside of our cone of focus, flip the sign on the turn
  if(Anki::Util::Abs((proposedNewAngle - _faceAngleCenter).getDegrees()) > kFocusAreaAngle_deg / 2.0f)
  {
    proposedNewAngle = Radians(currentBodyAngle - turnAmount);
  }
  
  // In the case where this is our first time moving, if our head wasn't already in the ideal range, move the head only
  // and cancel out the body movement
  static bool firstTimeMoving = true;
  if (firstTimeMoving && robot.GetHeadAngle() < kTiltMin)
  {
    proposedNewAngle = currentBodyAngle;
    firstTimeMoving = false;
  }
  
  float randomTilt = (float) GetRNG().RandDbl();
  Radians tiltRads(DEG_TO_RAD(((kTiltMax - kTiltMin) * randomTilt) + kTiltMin));
  
  IActionRunner* moveAction = new PanAndTiltAction(proposedNewAngle, tiltRads, true, true);
  _currentDriveActionID = moveAction->GetTag();
  robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, moveAction);
  
  _currentState = State::WaitToFinishMoving;
}

Result BehaviorFindFaces::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  _currentState = State::Inactive;
  return Result::RESULT_OK;
}
  
void BehaviorFindFaces::StopInternal(Robot& robot, double currentTime_sec)
{
  if (_currentDriveActionID != (uint32_t)ActionConstants::INVALID_TAG)
  {
    robot.GetActionList().Cancel(_currentDriveActionID);
    _currentDriveActionID = (uint32_t)ActionConstants::INVALID_TAG;
  }
}
  
} // namespace Cozmo
} // namespace Anki
