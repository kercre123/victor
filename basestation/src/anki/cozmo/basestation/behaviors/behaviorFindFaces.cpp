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

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviors/behaviorFindFaces.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/emotionScorer.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/actionTypes.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

static const char* kUseFaceAngleCenterKey = "center_face_angle";
static const bool kUseFaceAngleCenterDefault = true;
static const char* kMinimumFaceAgeKey = "minimum_face_age_s";
static const double kMinimumFaceAgeDefault = 180.0;
static const char* kPauseMinSecKey = "minimum_pause_s";
static const float kPauseMinSecDefault = 1.0f;
static const char* kPauseMaxSecKey = "maximum_pause_s";
static const float kPauseMaxSecDefault = 3.5f;
static const char* kMinScoreWhileActiveKey = "min_score_while_active";
static const float kMinScoreWhileActiveDefault = 0.0f;
static const char* kStopOnAnyFaceKey = "stop_on_any_face";
static const float kStopOnAnyFaceDefault = false;
  
#define DISABLE_IDLE_DURING_FIND_FACES 0

using namespace ExternalInterface;

#define DEBUG_BEHAVIOR_FIND_FACES_CONFIG 1

BehaviorFindFaces::BehaviorFindFaces(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentDriveActionID((uint32_t)ActionConstants::INVALID_TAG)
{
  SetDefaultName("FindFaces");

  _useFaceAngleCenter = config.get(kUseFaceAngleCenterKey, kUseFaceAngleCenterDefault).asBool();
  _minimumTimeSinceSeenLastFace_sec = config.get(kMinimumFaceAgeKey, kMinimumFaceAgeDefault).asDouble();
  _pauseMin_s = config.get(kPauseMinSecKey, kPauseMinSecDefault).asFloat();
  _pauseMax_s = config.get(kPauseMaxSecKey, kPauseMaxSecDefault).asFloat();
  _minScoreWhileActive = config.get(kMinScoreWhileActiveKey, kMinScoreWhileActiveDefault).asFloat();
  _stopOnAnyFace = config.get(kStopOnAnyFaceKey, kStopOnAnyFaceDefault).asBool();

  if(DEBUG_BEHAVIOR_FIND_FACES_CONFIG) {
    Json::Value debugOutput;
    debugOutput[kUseFaceAngleCenterKey] = _useFaceAngleCenter;
    debugOutput[kMinimumFaceAgeKey] = _minimumTimeSinceSeenLastFace_sec;
    debugOutput[kPauseMinSecKey] = _pauseMin_s;
    debugOutput[kPauseMaxSecKey] = _pauseMax_s;

    // PRINT_NAMED_INFO("BehaviorFindFaces.Config.Debug", "\n%s",
    //                  debugOutput.toStyledString().c_str());
  }
    
  SubscribeToTags({{
    EngineToGameTag::RobotCompletedAction,
    EngineToGameTag::RobotOffTreadsStateChanged
  }});

  if (GetEmotionScorerCount() == 0)
  {
    // Boredom and loneliness -> LookForFaces
    AddEmotionScorer(EmotionScorer(EmotionType::Excited, Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 0.7f}, {1.0f, 0.05f}}), false));
    AddEmotionScorer(EmotionScorer(EmotionType::Social,  Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 0.7f}, {1.0f, 0.05f}}), false));
  }
}
  
bool BehaviorFindFaces::IsRunnableInternal(const Robot& robot) const
{
  return true;
}
  
float BehaviorFindFaces::EvaluateScoreInternal(const Anki::Cozmo::Robot &robot) const
{
  Pose3d facePose;
  auto lastFaceTime = robot.GetFaceWorld().GetLastObservedFace(facePose);

  // can't compare current basestation time with robot timestamp, so get last image timestamp from robot
  auto currRobotTime = robot.GetLastMsgTimestamp();

  if (_currentState != State::Inactive ||
      lastFaceTime == 0 ||
      lastFaceTime < currRobotTime + SEC_TO_MILIS(_minimumTimeSinceSeenLastFace_sec)) {
    return IBehavior::EvaluateScoreInternal(robot);
  }
  
  return 0.0f;
}
  

float BehaviorFindFaces::EvaluateRunningScoreInternal(const Robot& robot) const
{
  double startTime_s = GetTimeStartedRunning_s();
  Pose3d facePose;
  auto lastFaceTime_ms = robot.GetFaceWorld().GetLastObservedFace(facePose);
  double lastFaceTime_s = MILIS_TO_SEC(lastFaceTime_ms);

  if( _stopOnAnyFace && lastFaceTime_s > startTime_s ) {
    // once this behavior finds a face, it doesn't want to run any more
    return 0.0f;
  }  
  else {

    float minScore = 0;
    if( _currentState != State::Inactive ) {
      minScore = _minScoreWhileActive;
    }
    
    return std::max(minScore, super::EvaluateRunningScoreInternal(robot));
  }
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
    case EngineToGameTag::RobotOffTreadsStateChanged:
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
  if (_useFaceAngleCenter && EngineToGameTag::RobotOffTreadsStateChanged == event.GetData().GetTag())
  {
    if(event.GetData().Get_RobotOffTreadsStateChanged().treadsState != OffTreadsState::OnTreads){
      _faceAngleCenterSet = false;
    }
  }
}
  
  
Result BehaviorFindFaces::InitInternal(Robot& robot)
{
  if( DISABLE_IDLE_DURING_FIND_FACES ) {
    robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::Count);
  }
  _currentDriveActionID = (uint32_t)ActionConstants::INVALID_TAG;
  _currentState = State::WaitToFinishMoving;
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorFindFaces::UpdateInternal(Robot& robot)
{
  
  // First time we're updating, set our face angle center
  if (_useFaceAngleCenter && !_faceAngleCenterSet)
  {
    _faceAngleCenter = robot.GetPose().GetRotationAngle();
    _faceAngleCenterSet = true;
  }

  const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

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
        _lookPauseTimer = currentTime_sec + (GetRNG().RandDbl() * (_pauseMax_s - _pauseMin_s) + _pauseMin_s);
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
  if(_useFaceAngleCenter &&
     Anki::Util::Abs((proposedNewAngle - _faceAngleCenter).getDegrees()) > kFocusAreaAngle_deg / 2.0f)
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
  
  IActionRunner* moveAction = new PanAndTiltAction(robot, proposedNewAngle, tiltRads, true, true);
  _currentDriveActionID = moveAction->GetTag();
  robot.GetActionList().QueueActionAtEnd(moveAction);
  
  _currentState = State::WaitToFinishMoving;
}
  
void BehaviorFindFaces::StopInternal(Robot& robot)
{
  if( DISABLE_IDLE_DURING_FIND_FACES ) {
    robot.GetAnimationStreamer().PopIdleAnimation();
  }
  if (_currentDriveActionID != (uint32_t)ActionConstants::INVALID_TAG)
  {
    robot.GetActionList().Cancel(_currentDriveActionID);
    _currentDriveActionID = (uint32_t)ActionConstants::INVALID_TAG;
  }
  _currentState = State::Inactive;
}
  
} // namespace Cozmo
} // namespace Anki
