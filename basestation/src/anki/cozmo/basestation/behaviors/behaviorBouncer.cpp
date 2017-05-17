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
#include "anki/cozmo/basestation/actions/setFaceAction.h"
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
  
// Fixed display parameters.
constexpr u32 kBouncerPaddleRow_px = 0;
constexpr u32 kBouncerPaddleHeight_px = 4;
  
// Fixed speed parameters, per tick, measured from [0,1]. Note that ball has minimum speed
// in both X and Y directions so it can't bounce straight up/down or straight left/right.
constexpr float kBouncerMaxPaddleSpeed = 0.10;
constexpr float kBouncerMinBallSpeed = 0.01;
constexpr float kBouncerMaxBallSpeed = 0.05;

  
// Console parameters
#define CONSOLE_GROUP "Behavior.Bouncer"

// Max time allowed in this behavior, in seconds
CONSOLE_VAR_RANGED(float, kBouncerTimeout_sec, CONSOLE_GROUP, 60.f, 0.f, 300.f);
  
// Max time allowed without seeing player, in seconds
CONSOLE_VAR_RANGED(float, kBouncerMissingFace_sec, CONSOLE_GROUP, 5.f, 0.f, 30.f);
  
// How much tilt is needed to move paddle?  Note that for positional motion, range of
// tilt is constrained by image recognition.  If head tilts past about 30deg, the
// face is no longer recognized and pose readings are not updated.
CONSOLE_VAR_RANGED(float, kBouncerTiltLeft_deg, CONSOLE_GROUP, -20.f, -30.f, -15.f);
CONSOLE_VAR_RANGED(float, kBouncerTiltRight_deg, CONSOLE_GROUP, 20.f, 15.f, 30.f);

// Width to draw on either side of paddle center
CONSOLE_VAR_RANGED(u32, kBouncerPaddleWidth_px, CONSOLE_GROUP, 10, 0, 15);

// Ball radius, in pixels
CONSOLE_VAR_RANGED(s32, kBouncerBallRadius_px, CONSOLE_GROUP, 3, 1, 5);

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
  
} // end anonymous namespace

// Check if feature gate is open
static inline bool IsFeatureEnabled(const Robot & robot)
{
  // Is this feature enabled?
  const auto * ctx = robot.GetContext();
  const auto * featureGate = ctx->GetFeatureGate();
  return featureGate->IsFeatureEnabled(Anki::Cozmo::FeatureType::Bouncer);
}

// Get random value in range [minVal,maxVal]
static float GetRandomFloat(Robot & robot, float minVal, float maxVal)
{
  auto & rng = robot.GetRNG();
  double val = rng.RandDblInRange(minVal, maxVal);
  return static_cast<float>(val);
}
  
static void DrawPaddle(Vision::Image& image, u32 paddleX)
{
  // How wide is paddle? Draw this many pixels left and right of center.
  const u32 numRows = image.GetNumRows();
  const u32 numCols = image.GetNumCols();
  const u32 minX = (paddleX < kBouncerPaddleWidth_px) ? 0 : (paddleX - kBouncerPaddleWidth_px);
  const u32 maxX = (paddleX + kBouncerPaddleWidth_px) > numCols ? numCols : (paddleX + kBouncerPaddleWidth_px);
  const u32 minY = kBouncerPaddleRow_px;
  const u32 maxY = (kBouncerPaddleRow_px + kBouncerPaddleHeight_px);
  
  // Draw from upper left <x,y> to lower right <x+w,y+h>
  Rectangle<f32> rect(minX, numRows-maxY-1, (maxX-minX), maxY-minY);
  image.DrawFilledRect(rect, NamedColors::WHITE);
  
}
  
static void DrawBall(Vision::Image& image, u32 ballX, u32 ballY)
{
  const u32 minX = 0;
  const u32 maxX = image.GetNumCols()-1;
  const u32 minY = 0;
  const u32 maxY = image.GetNumRows()-1;
    
  ballX = Util::Clamp(ballX, minX, maxX);
  ballY = Util::Clamp(ballY, minY, maxY);
    
  Point2f center(ballX, ballY);
  image.DrawFilledCircle(center, NamedColors::WHITE, kBouncerBallRadius_px);
}
  

BehaviorBouncer::BehaviorBouncer(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
}

// Check if behavior can run at this time
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

//
// Behavior is starting. Reset behavior state.
//
Result BehaviorBouncer::InitInternal(Robot& robot)
{
  // Reset behavior state
  SET_STATE(Init);
  
  // Disable reactionary behaviors that we don't want interrupting this
  SmartDisableReactionsWithLock(GetIDStr(), kReactionArray);
  
  // Disable idle animation
  auto & animationStreamer = robot.GetAnimationStreamer();
  animationStreamer.PushIdleAnimation(AnimationTrigger::Count);
  
  // Stash robot parameters
  _displayWidth_px = robot.GetDisplayWidthInPixels();
  _displayHeight_px = robot.GetDisplayHeightInPixels();
  
  // Paddle starts at center of bottom row
  _paddlePosX = 0.5f;
  _paddleSpeedX = kBouncerMaxPaddleSpeed;

  // Ball starts at center top with small random velocity.
  _ballPosX = 0.5f;
  _ballPosY = 1.0f;
  _ballSpeedX = GetRandomFloat(robot, kBouncerMinBallSpeed, kBouncerMaxBallSpeed);
  _ballSpeedY = GetRandomFloat(robot, kBouncerMinBallSpeed, kBouncerMaxBallSpeed);
  
  // No pending bounce sounds
  _paddleHitLeft = false;
  _paddleHitRight = false;
  
  return Result::RESULT_OK;
}

void BehaviorBouncer::UpdatePaddle(const Vision::TrackedFace * face)
{
  // Fetch previous paddle state
  float paddlePosX = _paddlePosX;
  float paddleSpeedX = _paddleSpeedX;
  
  // Did paddle start out against a wall?
  const bool wasLeft = Util::IsNear(paddlePosX, 0.f);
  const bool wasRight = Util::IsNear(paddlePosX, 1.f);
  
  // Get tilt of player's head, clamped to allowable range
  float playerTilt = Util::Clamp(face->GetHeadRoll().getDegrees(), kBouncerTiltLeft_deg, kBouncerTiltRight_deg);
    
  // Scale tilt to [0,1]. Note that we are converting distance along a curve to distance along a line.
  // It might make more sense to use cos(tilt) to isolate the horizontal component, or switch
  // to "directional" control where player nudges paddle left or right.
  float playerPosX = (playerTilt - kBouncerTiltLeft_deg)/(kBouncerTiltRight_deg-kBouncerTiltLeft_deg);
  
  // If paddle is close to player position, jump straight to it, else move one step closer
  if (Util::IsNear(playerPosX, paddlePosX, paddleSpeedX)) {
    paddlePosX = playerPosX;
  } else if (playerPosX < paddlePosX) {
    paddlePosX -= paddleSpeedX;
  } else {
    paddlePosX += paddleSpeedX;
  }
    
  // Paddle must stay within bounds
  paddlePosX = Util::Clamp(paddlePosX, 0.f, 1.f);
    
  // Did paddle end up against a wall?
  const bool isLeft = Util::IsNear(paddlePosX, 0.f);
  const bool isRight = Util::IsNear(paddlePosX, 1.f);
    
  if (isLeft && !wasLeft) {
    LOG_INFO("BehaviorBouncer.UpdateInternal", "Paddle hits left wall");
    _paddleHitLeft = true;
  } else if (isRight && !wasRight) {
    LOG_INFO("BehaviorBouncer.UpdateInternal", "Paddle hits right wall");
    _paddleHitRight = true;
  }
    
  // Save new paddle state
  _paddlePosX = paddlePosX;

}
  
void BehaviorBouncer::UpdateBall()
{
  // Fetch previous ball state
  float ballSpeedX = _ballSpeedX;
  float ballSpeedY = _ballSpeedY;
  float ballPosX = _ballPosX;
  float ballPosY = _ballPosY;
  
  // Move the ball
  ballPosX += ballSpeedX;
  ballPosY += ballSpeedY;
  
  // Bounce the ball
  if (ballPosX <= 0.f) {
    ballPosX = (0.f - ballPosX);
    ballSpeedX *= -1.f;
  } else if (ballPosX >= 1.0f) {
    ballPosX = 1.0 - (ballPosX - 1.0);
    ballSpeedX *= -1.f;
  }
  
  if (ballPosY <= 0.f) {
    ballPosY = (0.f - ballPosY);
    ballSpeedY *= -1.f;
  } else if (ballPosY >= 1.0f) {
    ballPosY = 1.0 - (ballPosY - 1.0);
    ballSpeedY *= -1.f;
  }
  
  // Ensure that ball never leaves [0,1]
  ballPosX = Util::Clamp(ballPosX, 0.f, 1.f);
  ballPosY = Util::Clamp(ballPosY, 0.f, 1.f);
  
  // Save new ball state
  _ballPosX = ballPosX;
  _ballPosY = ballPosY;
  _ballSpeedX = ballSpeedX;
  _ballSpeedY = ballSpeedY;
  
}

void BehaviorBouncer::UpdateSound(Robot& robot)
{
  // Do we need to play a bounce sound?
  // TODO: Trigger on ball movement, not paddle movement
  bool playBounceSound = false;
  
  if (!_isSoundActionInProgress && !_isFaceActionInProgress) {
    if (_paddleHitLeft) {
      _paddleHitLeft = false;
      playBounceSound = true;
    } else if (_paddleHitRight) {
      _paddleHitRight = false;
      playBounceSound = true;
    }
  }
  
  // TODO: Use arcade bounce sounds when available
  if (playBounceSound) {
    _isSoundActionInProgress = true;
    IActionRunner * soundAction = new TriggerAnimationAction(robot, AnimationTrigger::SoundOnlyTurnSmall);
    StartActing(soundAction, &BehaviorBouncer::SoundActionComplete);
  }
}
  
void BehaviorBouncer::UpdateDisplay(Robot& robot)
{
  // Scale positions from [0,1] to display position [0,DISPLAY_WIDTH]
  const u32 paddleX = Util::numeric_cast<u32>(_paddlePosX * _displayWidth_px);
  const u32 ballX = Util::numeric_cast<u32>(_ballPosX * _displayWidth_px);
  const u32 ballY = Util::numeric_cast<u32>(_ballPosY * _displayHeight_px);
  
  LOG_INFO("BehaviorBouncer.UpdateInternal", "paddleX=%u ballX=%u ballY=%u", paddleX, ballX, ballY);
  
  // Do we need to draw a new face?
  if (!_isSoundActionInProgress && !_isFaceActionInProgress) {
    
    // Init background
    Vision::Image image(_displayHeight_px, _displayWidth_px, NamedColors::BLACK);
    
    DrawPaddle(image, paddleX);
    DrawBall(image, ballX, ballY);
    
    // Display image
    _isFaceActionInProgress = true;
    const u32 duration_ms = IKeyFrame::SAMPLE_LENGTH_MS;
    IActionRunner * faceAction = new SetFaceAction(robot, image, duration_ms);
    StartActing(faceAction, &BehaviorBouncer::FaceActionComplete);
  }

}

BehaviorStatus BehaviorBouncer::UpdateInternal(Robot& robot)
{
  // Check elapsed time
  if (GetRunningDuration() > kBouncerTimeout_sec) {
    LOG_WARNING("BehaviorBouncer.UpdateInternal", "Behavior has timed out");
    return BehaviorStatus::Complete;
  }
  
  // Validate target state
  if (!_target.IsValid()) {
    LOG_WARNING("BehaviorBouncer.UpdateInternal", "Target face (%s) is not valid", _target.GetDebugStr().c_str());
    return BehaviorStatus::Complete;
  }
  
  // Get target face
  const auto & faceWorld = robot.GetFaceWorld();
  const auto * face = faceWorld.GetFace(_target);
  if (nullptr == face) {
    LOG_WARNING("BehaviorBouncer.UpdateInternal", "Target face (%s) has disappeared", _target.GetDebugStr().c_str());
    return BehaviorStatus::Complete;
  }
  
  // Validate face
  TimeStamp_t lastObserved_ms = face->GetTimeStamp();
  TimeStamp_t lastImage_ms = robot.GetLastImageTimeStamp();
  if (lastImage_ms - lastObserved_ms > Util::SecToMilliSec(kBouncerMissingFace_sec)) {
    LOG_WARNING("BehaviorBouncer.UpdateInternal", "Target face (%s) has gone stale",
                _target.GetDebugStr().c_str());
    return BehaviorStatus::Complete;
  }
  
  UpdatePaddle(face);
  
  UpdateBall();
  
  UpdateSound(robot);
  
  UpdateDisplay(robot);
  
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
      return BehaviorStatus::Complete;
    }
  }
  
  return BehaviorStatus::Running;
  
}

void BehaviorBouncer::SoundActionComplete()
{
  LOG_INFO("BehaviorBouncer.SoundActionComplete", "Sound action complete");
  _isSoundActionInProgress = false;
}
  
void BehaviorBouncer::FaceActionComplete()
{
  LOG_INFO("BehaviorBouncer.FaceActionComplete", "Face action complete");
  _isFaceActionInProgress = false;
}
  
void BehaviorBouncer::StopInternal(Robot& robot)
{
  LOG_DEBUG("BehaviorBouncer.StopInternal", "Stop behavior");
  
  // TODO: Record relevant DAS events here.
 
  // Restore idle animation
  auto & animationStreamer = robot.GetAnimationStreamer();
  animationStreamer.PopIdleAnimation();
}
  
} // namespace Cozmo
} // namespace Anki
