/**
 * File: behaviorBouncer.cpp
 *
 * Description: Cozmo plays a classic retro arcade game
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorBouncer.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"

#include "anki/common/basestation/utils/timer.h"

#include "util/console/consoleInterface.h"

// Log options
#define LOG_CHANNEL    "Bouncer"

#define LOG_ERROR      PRINT_NAMED_ERROR
#define LOG_WARNING    PRINT_NAMED_WARNING
#define LOG_INFO(...)  PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)
#define LOG_DEBUG(...) PRINT_CH_DEBUG(LOG_CHANNEL, ##__VA_ARGS__)

namespace Anki {
namespace Cozmo {

// Transition to a new state and update the debug name for logging/debugging
#define SET_STATE(s) do{ _state = State::s; DEBUG_SET_STATE(s); } while(0)
  
namespace {
  
// Reaction triggers to disable
constexpr ReactionTriggerHelpers::FullReactionArray kReactionArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::PyramidInitialDetection,      true},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, true},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           false}
};
  
static_assert(ReactionTriggerHelpers::IsSequentialArray(kReactionArray), "Invalid reaction triggers");

// Console parameters
#define CONSOLE_GROUP "Behavior.Bouncer"

// Max time allowed in this behavior
CONSOLE_VAR_RANGED(float, kBouncerTimeout_sec, CONSOLE_GROUP, 60.f, 0.f, 300.f);
  
// Clamp head tilt to this range
CONSOLE_VAR_RANGED(float, kBouncerMinRoll_deg, CONSOLE_GROUP, -45.f, -90.f, -15.f);
CONSOLE_VAR_RANGED(float, kBouncerMaxRoll_deg, CONSOLE_GROUP, 45.f, 15.f, 90.f);
  
} // end anonymous namespace
  
static inline float GetBaseTimeInSeconds()
{
  BaseStationTimer* timer = BaseStationTimer::getInstance();
  DEV_ASSERT(timer != nullptr, "BehaviorBouncer.GetBaseTimeInSeconds.InvalidTimer");
  return timer->GetCurrentTimeInSeconds();
}

static inline bool IsFeatureEnabled(const Robot & robot)
{
  // Is this feature enabled?
  const auto * ctx = robot.GetContext();
  const auto * featureGate = ctx->GetFeatureGate();
  return featureGate->IsFeatureEnabled(Anki::Cozmo::FeatureType::Bouncer);
}

BehaviorBouncer::BehaviorBouncer(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("Bouncer");
}

bool BehaviorBouncer::IsRunnableInternal(const BehaviorPreReqRobot & preReq) const
{
  const Robot & robot = preReq.GetRobot();
  
  // Is this feature enabled?
  if (!IsFeatureEnabled(robot)) {
    LOG_DEBUG("BehaviorBouncer.IsRunnableInternal", "Feature is disabled");
    return false;
  }
  
  // Look for a target
  const auto & faceWorld = robot.GetFaceWorld();
  const auto & faceIDs = faceWorld.GetFaceIDs();
  
  for (auto faceID : faceIDs) {
    _target = faceWorld.GetSmartFaceID(faceID);
    if (_target.IsValid()) {
      break;
    }
  }
  
  if (!_target.IsValid()) {
    LOG_DEBUG("BehaviorBouncer.IsRunnableInternal", "No valid target");
    return false;
  }
  
  LOG_DEBUG("BehaviorBouncer.IsRunnableInternal", "Behavior is runnable with target %s", _target.GetDebugStr().c_str());

  return true;
}
  
bool BehaviorBouncer::IsRunnableInternal(const BehaviorPreReqAcknowledgeFace & preReq) const
{
   const auto & robot = preReq.GetRobot();
  
  // Is this feature enabled?
  if (!IsFeatureEnabled(robot)) {
    LOG_DEBUG("BehaviorBouncer.IsRunnableInternal", "Feature is disabled");
    return false;
  }
  
  const auto & targets = preReq.GetDesiredTargets();
  if (targets.empty()) {
    LOG_WARNING("BehaviorBounder.IsRunnableInternal", "No trigger faces");
    return false;
  }
  
  // Pick the first (best) target available
  const auto & faceWorld = robot.GetFaceWorld();
  SmartFaceID target;
  for (const auto faceID : targets) {
    target = faceWorld.GetSmartFaceID(faceID);
    if (target.IsValid()) {
      break;
    }
  }
  
  if (!target.IsValid()) {
    LOG_WARNING("BehaviorBouncer.IsRunnableInternal", "No valid target");
    return false;
  }
  
  _target = std::move(target);

  LOG_DEBUG("BehaviorBouncer.IsRunnableInternal", "Behavior is runnable with target %s", _target.GetDebugStr().c_str());
  
  return true;
}

  
Result BehaviorBouncer::InitInternal(Robot& robot)
{
  // Disable reactionary behaviors that we don't want interrupting this:
  SmartDisableReactionsWithLock(GetName(), kReactionArray);
  
  // Reset behavior state
  SET_STATE(Init);
  
  // Reset timer
  _startedAt_sec = GetBaseTimeInSeconds();
  
  return Result::RESULT_OK;
}
  
  
BehaviorBouncer::Status BehaviorBouncer::UpdateInternal(Robot& robot)
{
  // Check elapsed time
  const float now_sec = GetBaseTimeInSeconds();
  const float elapsed_sec = (now_sec - _startedAt_sec);
  
  if (elapsed_sec > kBouncerTimeout_sec) {
    LOG_WARNING("BehaviorBouncer.UpdateInternal", "Behavior has timed out");
    return Status::Failure;
  }
  
  // Check target state
  if (!_target.IsValid()) {
    LOG_WARNING("BehaviorBouncer.UpdateInternal", "Target face (%s) is not valid", _target.GetDebugStr().c_str());
    return Status::Failure;
  }
  
  // Get target face
  const auto & faceWorld = robot.GetFaceWorld();
  const auto * face = faceWorld.GetFace(_target);
  if (nullptr == face) {
    LOG_WARNING("BehaviorBouncer.UpdateInternal", "Target face (%s) has disappeared", _target.GetDebugStr().c_str());
    return Status::Failure;
  }
  
  const float pitch = face->GetHeadPitch().getDegrees();
  const float roll = face->GetHeadRoll().getDegrees();
  const float yaw = face->GetHeadYaw().getDegrees();
  
  // Clamp face roll to 45 degrees on either side of 0 (vertical)
  const float clampedRoll = Util::Clamp(roll, kBouncerMinRoll_deg, kBouncerMaxRoll_deg);
  
  // Range bounds ensure that (min < max) so we don't have to worry about divide by zero.
  DEV_ASSERT(kBouncerMinRoll_deg < kBouncerMaxRoll_deg, "BehaviorBouncer.UpdateInternal.InvalidClamp");
  
  // Scale face roll to range [0,1]
  const float scaledRoll = (clampedRoll - kBouncerMinRoll_deg) / (kBouncerMaxRoll_deg - kBouncerMinRoll_deg);
  
  // Scale face roll to display position [0,DISPLAY_WIDTH]
  const u32 displayWidth = robot.GetDisplayWidthInPixels();
  const u32 playerX = Util::numeric_cast<u32>(scaledRoll * displayWidth);
  
  LOG_INFO("BehaviorBouncer.UpdateInternal", "pitch=%.2f roll=%.2f yaw=%.2f playerX=%u", pitch, roll, yaw, playerX);
  
  // Game logic goes here
  switch (_state) {
    case State::Init:
    {
      break;
    }
    case State::Timeout:
    {
      break;
    }
    case State::Complete:
    {
      return Status::Complete;
    }
  }
  
  return Status::Running;
}


void BehaviorBouncer::StopInternal(Robot& robot)
{
  LOG_DEBUG("BehaviorBouncer.StopInternal", "Stop behavior");
  
  // Stop light cube animations:
  robot.GetCubeLightComponent().StopAllAnims();
  
  // TODO: Record relevant DAS events here.
 
}
  
  
void BehaviorBouncer::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  // Handle messages
  const auto & data = event.GetData();
  const auto tag = data.GetTag();
  
  LOG_DEBUG("BehaviorBouncer.HandleWhileRunning", "Handle event %hhu (%s)", tag, MessageEngineToGameTagToString(tag));
  
}
  
} // namespace Cozmo
} // namespace Anki
