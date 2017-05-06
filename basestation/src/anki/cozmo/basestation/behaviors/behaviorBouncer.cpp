/**
 * File: behaviorBouncer.cpp
 *
 * Description: Cozmo plays a classic retro arcade game
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorBouncer.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
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

// Enable verbose trace messages?
#define ENABLE_TRACE 0

#if ENABLE_TRACE
#define LOG_TRACE LOG_DEBUG
#else
#define LOG_TRACE(...) {}
#endif

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
  
// How much tilt is needed to move paddle?
CONSOLE_VAR_RANGED(float, kBouncerTiltLeft_deg, CONSOLE_GROUP, -10.f, -15.f, -5.f);
CONSOLE_VAR_RANGED(float, kBouncerTiltRight_deg, CONSOLE_GROUP, 10.f, 5.f, 15.f);
  
} // end anonymous namespace
  
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
    LOG_TRACE("BehaviorBouncer.IsRunnableInternal", "Feature is disabled");
    return false;
  }
  
  // Look for a target
  const auto & faceWorld = robot.GetFaceWorld();
  const auto & faceIDs = faceWorld.GetFaceIDs();
  
  if (faceIDs.empty()) {
    LOG_TRACE("BehaviorBouncer.IsRunnableInternal", "No faces to trace");
    return false;
  }
  
  const auto & whiteboard = robot.GetAIComponent().GetWhiteboard();
  const bool preferKnownFaces = true;
  const auto faceID = whiteboard.GetBestFaceToTrack(faceIDs, preferKnownFaces);
  _target = faceWorld.GetSmartFaceID(faceID);
  
  if (!_target.IsValid()) {
    LOG_WARNING("BehaviorBouncer.IsRunnableInternal", "Best face (%s) is not valid", _target.GetDebugStr().c_str());
    return false;
  }
  
  LOG_TRACE("BehaviorBouncer.IsRunnableInternal", "Behavior is runnable with target %s", _target.GetDebugStr().c_str());
  return true;
  
}
  
Result BehaviorBouncer::InitInternal(Robot& robot)
{
  // Reset behavior state
  SET_STATE(Init);
  
  // Disable reactionary behaviors that we don't want interrupting this
  SmartDisableReactionsWithLock(GetName(), kReactionArray);
  
  // Stash robot parameters
  _displayWidth_px = robot.GetDisplayWidthInPixels();
  _displayHeight_px = robot.GetDisplayHeightInPixels();
  
  // Paddle starts at center of bottom row
  _paddlePos = 0.5f;
  
  return Result::RESULT_OK;
}
  
  
BehaviorBouncer::Status BehaviorBouncer::UpdateInternal(Robot& robot)
{
  // Check elapsed time
  if (GetRunningDuration() > kBouncerTimeout_sec) {
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
  
  const float playerTilt = face->GetHeadRoll().getDegrees();
  float paddlePos = _paddlePos;
  
  // Did paddle start out against a wall?
  const bool wasLeft = Util::IsNear(paddlePos, 0.f);
  const bool wasRight = Util::IsNear(paddlePos, 1.f);
  
  // Width of one pixel, scaled from [0,1]
  const float width_px = (1.f / _displayWidth_px);
  
  // Slide paddle by 1-3 pixels in either direction
  if (playerTilt <= (3 * kBouncerTiltLeft_deg)) {
    // Paddle moves 3x left
    paddlePos -=  (3 * width_px);
  } else if (playerTilt <= (2 * kBouncerTiltLeft_deg)) {
    // Paddle moves 2x left
    paddlePos -= (2 * width_px);
  } else if (playerTilt <= kBouncerTiltLeft_deg) {
    // Paddle moves 1x left
    paddlePos -= width_px;
  } else if (playerTilt >= (3 * kBouncerTiltRight_deg)) {
    // Paddle moves 3x right
    paddlePos += (3 * width_px);
  } else if (playerTilt >= (2 * kBouncerTiltRight_deg)) {
    // Paddle moves 2x right
    paddlePos += (2 * width_px);
  } else if (playerTilt >= kBouncerTiltRight_deg) {
     // Paddle moves 1x right
    paddlePos += width_px;
  }
 
  // Paddle must stay within bounds
  paddlePos = Util::Clamp(paddlePos, 0.f, 1.f);
  
  // Did paddle end up against a wall?
  const bool isLeft = Util::IsNear(paddlePos, 0.f);
  const bool isRight = Util::IsNear(paddlePos, 1.f);
  
  // Did paddle hit a wall on this tick? Play an animation.
  IActionRunner * action = nullptr;
  if (isLeft && !wasLeft) {
    LOG_INFO("BehaviorBouncer.UpdateInternal", "Paddle hits left wall");
    action = new TriggerAnimationAction(robot, AnimationTrigger::SoundOnlyLiftEffortPickup);
  } else if (isRight && !wasRight) {
    LOG_INFO("BehaviorBouncer.UpdateInternal", "Paddle hits right wall");
    action = new TriggerAnimationAction(robot, AnimationTrigger::SoundOnlyLiftEffortPlaceRoll);
  }
  
  if (nullptr != action) {
    StartActing(action);
  }

  // Scale paddle position to display position [0,DISPLAY_WIDTH]
  const u32 paddleX = Util::numeric_cast<u32>(paddlePos * _displayWidth_px);
  
  LOG_INFO("BehaviorBouncer.UpdateInternal", "playerTilt=%.2f paddlePos=%.2f paddleX=%u", playerTilt, paddlePos, paddleX);
  
  // Save state for next tick
  _paddlePos = paddlePos;
  
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
  
  // TODO: Record relevant DAS events here.
 
}
  
} // namespace Cozmo
} // namespace Anki
