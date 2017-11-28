/**
 * File: behaviorBouncer.cpp
 *
 * Description: Cozmo plays a classic retro arcade game
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorBouncer.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/utils/cozmoFeatureGate.h"

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
  
namespace {
  
// Fixed display parameters.
constexpr float kBouncerPaddleHeight_px = 4;
  
// Fixed speed parameters, in pixels per tick. Note that ball has minimum speed in both X and Y directions
// so it can't bounce straight up/down or straight left/right.
constexpr u32 kBouncerMaxPaddleSpeed = 10;
constexpr u32 kBouncerMinBallSpeed = 1;
constexpr u32 kBouncerMaxBallSpeed = 3;

  
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
CONSOLE_VAR_RANGED(u32, kBouncerPaddleWidth_px, CONSOLE_GROUP, 16, 1, 20);

// Ball radius, in pixels
CONSOLE_VAR_RANGED(s32, kBouncerBallRadius_px, CONSOLE_GROUP, 3, 1, 5);
  
} // end anonymous namespace

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Check if feature gate is open
static inline bool IsFeatureEnabled(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  // Is this feature enabled?
  const auto * ctx = robot.GetContext();
  const auto * featureGate = ctx->GetFeatureGate();
  return featureGate->IsFeatureEnabled(Anki::Cozmo::FeatureType::Bouncer);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Get random choice of 1 or -1
static int GetRandomSign(Robot & robot)
{
  auto & rng = robot.GetRNG();
  double val = rng.RandDbl();
  return (val < 0.5 ? 1 : -1);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Get random value in range [minVal,maxVal]
static float GetRandomFloat(Robot & robot, float minVal, float maxVal)
{
  auto & rng = robot.GetRNG();
  double val = rng.RandDblInRange(minVal, maxVal);
  return static_cast<float>(val);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Draw paddle onto image
void BehaviorBouncer::DrawPaddle(Vision::Image& image)
{
  // How wide is paddle? Draw this many pixels left and right of center.
  const float minX = Util::Clamp(_paddlePosX - kBouncerPaddleWidth_px,  0.f, _displayWidth_px);
  const float maxX = Util::Clamp(_paddlePosX + kBouncerPaddleWidth_px, 0.f, _displayWidth_px);
  const float minY = Util::Clamp(_paddlePosY, 0.f, _displayHeight_px);
  const float maxY = Util::Clamp(_paddlePosY + kBouncerPaddleHeight_px, 0.f, _displayHeight_px);
  
  // Draw from upper left <x,y> to lower right <x+w,y+h>
  Rectangle<f32> rect(minX, minY, (maxX-minX), maxY-minY);
  image.DrawFilledRect(rect, NamedColors::WHITE);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Draw ball onto image
void BehaviorBouncer::DrawBall(Vision::Image& image)
{
  const float centerX = Util::Clamp(_ballPosX, 0.f, _displayWidth_px);
  const float centerY = Util::Clamp(_ballPosY, 0.f, _displayHeight_px);
    
  Point2f center(centerX, centerY);
  image.DrawFilledCircle(center, NamedColors::WHITE, kBouncerBallRadius_px);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Draw score onto image
void BehaviorBouncer::DrawScore(Vision::Image& image)
{
  // What are we going to draw?
  const std::string& sTextLeft = std::to_string(_playerHits);
  const std::string& sTextRight = std::to_string(_playerMisses);
  
  // Draw numbers at 50% normal scale.
  // Each digit is about 8px square.
  // Leave a 4px border between text and edge of screen.
  const float kTextScale = 0.5f;
  const float kTextHeight = 8.f;
  const float kTextWidth = 8.f;
  const float kTextBorder = 4.f;
  
  // Where do we draw it?
  const float kTextTop = kTextBorder + kTextHeight;
  const float kTextLeft = kTextBorder;
  const float kTextRight= (_displayWidth_px - kTextBorder - sTextRight.size()*kTextWidth);
  
  const Point2f topLeft(kTextLeft, kTextTop);
  image.DrawText(topLeft, sTextLeft, NamedColors::WHITE, kTextScale);
  
  const Point2f topRight(kTextRight, kTextTop);
  image.DrawText(topRight, sTextRight, NamedColors::WHITE, kTextScale);

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Convert BouncerState to string
const char * BehaviorBouncer::EnumToString(const BouncerState& state)
{
  switch (state) {
    case BouncerState::Init:
      return "Init";
    case BouncerState::GetIn:
      return "GetIn";
    case BouncerState::IdeaToPlay:
      return "IdeaToPlay";
    case BouncerState::RequestToPlay:
      return "RequestToPlay";
    case BouncerState::WaitToPlay:
      return "WaitToPlay";
    case BouncerState::Play:
      return "Play";
    case BouncerState::Timeout:
      return "Timeout";
    case BouncerState::ShowScore:
      return "ShowScore";
    case BouncerState::GetOut:
      return "GetOut";
    case BouncerState::Complete:
      return "Complete";
    default:
      DEV_ASSERT(false, "BehaviorBouncer.EnumToString.InvalidState");
      return "Invalid";
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Transition to given state
void BehaviorBouncer::TransitionToState(const BouncerState& state)
{
  if (_state != state) {
    LOG_TRACE("BehaviorBouncer.TransitionToState", "%s to %s", EnumToString(_state), EnumToString(state));
    SetDebugStateName(EnumToString(state));
    _state = state;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBouncer::StartAnimation(BehaviorExternalInterface& behaviorExternalInterface,
                                     const AnimationTrigger& animationTrigger,
                                     const BouncerState& nextState)
{
  LOG_TRACE("BehaviorBouncer.StartAnimation", "Start animation %s, nextState %s",
            AnimationTriggerToString(animationTrigger), EnumToString(nextState));
  
  // This should never be called when action is already in progress
  DEV_ASSERT(!IsControlDelegated(), "BehaviorBouncer.StartAnimation.ShouldBeInControl");
    
  auto callback = [this, nextState] {
    LOG_TRACE("BehaviorBouncer.StartAnimation.Callback", "Finish animation %s, nextState %s",
              AnimationTriggerToString(animationTrigger), EnumToString(nextState));
    TransitionToState(nextState);
  };
  
  IActionRunner* action = new TriggerAnimationAction(animationTrigger);
  DelegateIfInControl(action, callback);
  
  // DelegateIfInControl shouldn't fail
  DEV_ASSERT(IsControlDelegated(), "BehaviorBouncer.StartAnimation.ShouldNotBeInControl");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBouncer::StartAnimation(BehaviorExternalInterface& behaviorExternalInterface, const AnimationTrigger& animationTrigger)
{
  StartAnimation(behaviorExternalInterface, animationTrigger, _state);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBouncer::BehaviorBouncer(const Json::Value& config)
: ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Check if behavior can run at this time
bool BehaviorBouncer::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // Is this feature enabled?
  if (!IsFeatureEnabled(behaviorExternalInterface)) {
    LOG_TRACE("BehaviorBouncer.WantsToBeActivatedBehavior", "Feature is disabled");
    return false;
  }
  
  // Look for a target
  const auto & faceWorld = behaviorExternalInterface.GetFaceWorld();
  const auto & faceIDs = faceWorld.GetFaceIDs();
  
  if (faceIDs.empty()) {
    LOG_TRACE("BehaviorBouncer.WantsToBeActivatedBehavior", "No faces to track");
    return false;
  }

  std::set<SmartFaceID> smartIDs;
  for(auto& entry: faceIDs){
    smartIDs.insert(faceWorld.GetSmartFaceID(entry));
  }
  
  const auto & whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
  const bool preferKnownFaces = true;
  _target = whiteboard.GetBestFaceToTrack(smartIDs, preferKnownFaces);


  if (!_target.IsValid()) {
    LOG_WARNING("BehaviorBouncer.WantsToBeActivatedBehavior", "Best face (%s) is not valid", _target.GetDebugStr().c_str());
    return false;
  }
  
  LOG_TRACE("BehaviorBouncer.WantsToBeActivatedBehavior", "Behavior is activatable with target %s", _target.GetDebugStr().c_str());
  return true;
  
}

//
// Behavior is starting. Reset behavior state.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorBouncer::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  LOG_TRACE("BehaviorBouncer.InitInternal", "Init behavior");
  
  // Disable idle animation
  SmartPushIdleAnimation(behaviorExternalInterface, AnimationTrigger::Count);
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // Stash robot parameters
  _displayWidth_px = robot.GetDisplayWidthInPixels();
  _displayHeight_px = robot.GetDisplayHeightInPixels();
  
  // Reset player stats
  _playerHits = 0;
  _playerMisses = 0;
  
  // Paddle starts at center of bottom row
  _paddlePosX = (_displayWidth_px/2);
  _paddlePosY = (_displayHeight_px - kBouncerPaddleHeight_px);
  _paddleSpeedX = kBouncerMaxPaddleSpeed;

  // Ball starts at center of bottom row with small upward velocity.
  _ballPosX = (_displayWidth_px/2);
  _ballPosY = _displayHeight_px;
  _ballSpeedX = GetRandomSign(robot) * GetRandomFloat(robot, kBouncerMinBallSpeed, kBouncerMaxBallSpeed);
  _ballSpeedY = -1 * GetRandomFloat(robot, kBouncerMinBallSpeed, kBouncerMaxBallSpeed);
  
  // No pending hits
  _ballHitFloor = false;
  
  TransitionToState(BouncerState::GetIn);
  
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBouncer::UpdatePaddle(const Vision::TrackedFace * face)
{
  LOG_TRACE("BehaviorBouncer.UpdatePaddle", "Update paddle");
  
  // Fetch previous paddle state
  float paddlePosX = _paddlePosX;
  float paddleSpeedX = _paddleSpeedX;
  
  // Get tilt of player's head, clamped to allowable range
  float playerTilt = Util::Clamp(face->GetHeadRoll().getDegrees(), kBouncerTiltLeft_deg, kBouncerTiltRight_deg);
    
  // Scale tilt to [0,1]. Note that we are converting distance along a curve to distance along a line.
  // It might make more sense to use cos(tilt) to isolate the horizontal component, or switch
  // to "directional" control where player nudges paddle left or right.
  playerTilt = (playerTilt - kBouncerTiltLeft_deg)/(kBouncerTiltRight_deg-kBouncerTiltLeft_deg);
  
  // Scale tilt to screen coordinates [0,width].
  float playerPosX = (1.0f - playerTilt) * _displayWidth_px;
  
  // If paddle is close to player position, jump straight to it, else move one step closer
  if (Util::IsNear(playerPosX, paddlePosX, paddleSpeedX)) {
    paddlePosX = playerPosX;
  } else if (playerPosX < paddlePosX) {
    paddlePosX -= paddleSpeedX;
  } else {
    paddlePosX += paddleSpeedX;
  }
    
  // Paddle must stay within bounds
  paddlePosX = Util::Clamp(paddlePosX, 0.f, _displayWidth_px);
  
  // Save new paddle state
  _paddlePosX = paddlePosX;

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBouncer::UpdateBall()
{
  LOG_TRACE("BehaviorBouncer.UpdateBall", "Update ball");
  
  // Fetch previous ball state
  float ballPosX = _ballPosX;
  float ballPosY = _ballPosY;
  float ballSpeedX = _ballSpeedX;
  float ballSpeedY = _ballSpeedY;
  
  // Move the ball
  ballPosX += ballSpeedX;
  ballPosY += ballSpeedY;
  
  // Bounce the ball
  if (ballPosX <= 0.f) {
    ballPosX = (0.f - ballPosX);
    ballSpeedX *= -1.f;
  } else if (ballPosX >= _displayWidth_px) {
    ballPosX = _displayWidth_px - (ballPosX - _displayWidth_px);
    ballSpeedX *= -1.f;
  }
  
  if (ballPosY <= 0.f) {
    ballPosY = (0.f - ballPosY);
    ballSpeedY *= -1.f;
  } else if (ballPosY >= _displayHeight_px) {
    // If ball is within one paddle width of paddle, assume it has bounced.
    // TO DO: Calculate exact position of crossing
    // TO DO: Ball should bounce of surface of paddle, not wall
    // TO DO: Ball should increase speed after each bounce
    // TO DO: Ball should get random change in direction after each bounce
    if (Util::IsNear(ballPosX, _paddlePosX, kBouncerPaddleWidth_px)) {
      LOG_TRACE("BehaviorBouncer.UpdateBall", "Player hit the ball");
      ++_playerHits;
    } else {
      LOG_TRACE("BehaviorBouncer.UpdateBall", "Player missed the ball");
      _ballHitFloor = true;
      ++_playerMisses;
    }
    ballPosY = _displayHeight_px - (ballPosY - _displayHeight_px);
    ballSpeedY *= -1.f;
  }
  
  // Ensure that ball never leaves screen
  ballPosX = Util::Clamp(ballPosX, 0.f, _displayWidth_px);
  ballPosY = Util::Clamp(ballPosY, 0.f, _displayHeight_px);
  
  // Save new ball state
  _ballPosX = ballPosX;
  _ballPosY = ballPosY;
  _ballSpeedX = ballSpeedX;
  _ballSpeedY = ballSpeedY;
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBouncer::UpdateSound(BehaviorExternalInterface& behaviorExternalInterface)
{
  // TODO: Use arcade bounce sounds when available
  static const auto animationTrigger = AnimationTrigger::SoundOnlyLiftEffortPlaceLow;
  
  // Do we need to play a bounce sound?
  bool playBounceSound = false;
  
  if (!IsControlDelegated()) {
    if (_ballHitFloor) {
      playBounceSound = true;
      _ballHitFloor = false;
    }
  }
  
  if (playBounceSound) {
    LOG_TRACE("BehaviorBouncer.UpdateSound", "Start sound action");
    StartAnimation(behaviorExternalInterface, animationTrigger);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBouncer::UpdateDisplay(BehaviorExternalInterface& behaviorExternalInterface)
{
  LOG_TRACE("BehaviorBouncer.UpdateInternal", "paddleX=%.2f ballX=%.2f ballY=%.2f", _paddlePosX, _ballPosX, _ballPosY);
  
  // Do we need to draw a new face?
  if (!IsControlDelegated()) {
    
    // Init background
    Vision::Image image(_displayHeight_px, _displayWidth_px, NamedColors::BLACK);
    
    DrawPaddle(image);
    DrawBall(image);
    DrawScore(image);
    
    // Display image
    LOG_TRACE("BehaviorBouncer.UpdateDisplay", "Start face action");
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    robot.GetAnimationComponent().DisplayFaceImageBinary(image, ANIM_TIME_STEP_MS);
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus BehaviorBouncer::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  LOG_TRACE("BehaviorBouncer.UpdateIntern_Legacy", "Update behavior with state=%s", EnumToString(_state));

  // Check elapsed time
  if (GetActivatedDuration() > kBouncerTimeout_sec) {
    LOG_WARNING("BehaviorBouncer.UpdateIntern_Legacy", "Behavior has timed out");
    return BehaviorStatus::Complete;
  }
  
  // Validate target state
  if (!_target.IsValid()) {
    LOG_WARNING("BehaviorBouncer.UpdateIntern_Legacy", "Target face (%s) is not valid", _target.GetDebugStr().c_str());
    return BehaviorStatus::Complete;
  }
  
  // Get target face
  const auto & faceWorld = behaviorExternalInterface.GetFaceWorld();
  const auto * face = faceWorld.GetFace(_target);
  if (nullptr == face) {
    LOG_WARNING("BehaviorBouncer.UpdateIntern_Legacy", "Target face (%s) has disappeared", _target.GetDebugStr().c_str());
    return BehaviorStatus::Complete;
  }
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  // Validate face
  TimeStamp_t lastObserved_ms = face->GetTimeStamp();
  TimeStamp_t lastImage_ms = robot.GetLastImageTimeStamp();
  if (lastImage_ms - lastObserved_ms > Util::SecToMilliSec(kBouncerMissingFace_sec)) {
    LOG_WARNING("BehaviorBouncer.UpdateIntern_Legacy", "Target face (%s) has gone stale",
                _target.GetDebugStr().c_str());
    return BehaviorStatus::Complete;
  }
  
  switch (_state) {
    case BouncerState::Init:
    {
      // This should never happen. InitInternal (above) sets state to GetIn.
      DEV_ASSERT(_state != BouncerState::Init, "BehaviorBouncer.Update.InvalidState");
      break;
    }
    case BouncerState::GetIn:
    {
      if (!IsControlDelegated()) {
        StartAnimation(behaviorExternalInterface, AnimationTrigger::BouncerGetIn, BouncerState::IdeaToPlay);
      }
      break;
    }
    case BouncerState::IdeaToPlay:
    {
      if (!IsControlDelegated()) {
        StartAnimation(behaviorExternalInterface, AnimationTrigger::BouncerIdeaToPlay, BouncerState::RequestToPlay);
      }
      break;
    }
    case BouncerState::RequestToPlay:
    {
      if (!IsControlDelegated()) {
        StartAnimation(behaviorExternalInterface, AnimationTrigger::BouncerRequestToPlay, BouncerState::WaitToPlay);
      }
      break;
    }
    case BouncerState::WaitToPlay:
    {
      if (!IsControlDelegated()) {
        StartAnimation(behaviorExternalInterface, AnimationTrigger::BouncerWait, BouncerState::Play);
      }
      break;
    }
    case BouncerState::Play:
    {
      // This is the actual game loop
      UpdatePaddle(face);
      UpdateBall();
      UpdateSound(behaviorExternalInterface);
      UpdateDisplay(behaviorExternalInterface);
      break;
    }
    case BouncerState::Timeout:
    {
      if (!IsControlDelegated()) {
        StartAnimation(behaviorExternalInterface, AnimationTrigger::BouncerTimeout, BouncerState::ShowScore);
      }
      break;
    }
    case BouncerState::ShowScore:
    {
      if (!IsControlDelegated()) {
        // TO DO: Choose between BouncerIntoScore1, BouncerIntoScore2, BouncerIntoScore3
        StartAnimation(behaviorExternalInterface, AnimationTrigger::BouncerIntoScore1, BouncerState::GetOut);
      }
      break;
    }
    case BouncerState::GetOut:
    {
      if (!IsControlDelegated()) {
        StartAnimation(behaviorExternalInterface, AnimationTrigger::BouncerGetOut, BouncerState::Complete);
      }
      break;
    }
    case BouncerState::Complete:
    {
      if (!IsControlDelegated()) {
        LOG_TRACE("BehaviorBouncer.Update.Complete", "Behavior complete");
        return BehaviorStatus::Complete;
      }
    }
  }
    
  return BehaviorStatus::Running;
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBouncer::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  LOG_TRACE("BehaviorBouncer.StopInternal", "Stop behavior");
  
  // TODO: Record relevant DAS events here.
}
  
} // namespace Cozmo
} // namespace Anki
