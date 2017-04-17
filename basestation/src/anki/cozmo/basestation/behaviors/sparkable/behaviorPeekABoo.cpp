/**
 * File: behaviorPeekABoo.cpp
 *
 * Author: Molly Jameson
 * Created: 2016-02-15
 *
 * Description: Behavior to do PeekABoo
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPeekABoo.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/vision/basestation/faceTracker.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"
#include "anki/common/basestation/utils/timer.h"
#include "clad/types/animationTrigger.h"

#include "util/console/consoleInterface.h"
#include "util/math/numericCast.h"

namespace Anki {
namespace Cozmo {

namespace{
#define SET_STATE(s) SetState_internal(State::s, #s)
CONSOLE_VAR(uint, kFramesWithoutFaceForPeek, "Behavior.PeekABoo", 3);
CONSOLE_VAR(bool, kCenterFaceAfterPeekABoo, "Behavior.PeekABoo", true);
static constexpr float kPercentCompleteSmallReaction  = 0.3f;
static constexpr float kPercentCompleteMedReaction    = 0.6f;
static constexpr float kSparkShouldPlaySparkFailFlag  = -1.0f;
static constexpr int   kMaxTurnToFaceRetryCount       = 4;
static constexpr int   kMaxCountTrackingEyesEntries   = 50;
  
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersPeekABooArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::PyramidInitialDetection,      false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, false},
  {ReactionTrigger::UnexpectedMovement,           false},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersPeekABooArray),
              "Reaction triggers duplicate or non-sequential");
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPeekABoo::BehaviorPeekABoo(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _cachedFace(Vision::UnknownFaceID)
, _numPeeksRemaining(0)
, _numPeeksTotal(1)
, _noUserInteractionTimeout_Sec(0.0f)
, _nextTimeIsRunnable_Sec(0.0f)
, _lastRequestTime_Sec(0.0f)
, _hasMadeFollowUpRequest(false)
, _turnToFaceRetryCount(0)
, _stillSawFaceAfterRequest(false)
, _currentState(State::DoingInitialReaction)
, _timeSparkAboutToEnd_Sec(0.0f)
{
  JsonTools::GetValueOptional(config, "minTimesPeekBeforeQuit",       _params.minPeeks);
  JsonTools::GetValueOptional(config, "maxTimesPeekBeforeQuit",       _params.maxPeeks);
  JsonTools::GetValueOptional(config, "noUserInteractionTimeout_Sec", _params.noUserInteractionTimeout_Sec);
  JsonTools::GetValueOptional(config, "requireFaceConfirmBeforeRequest", _params.requireFaceConfirmBeforeRequest);
  JsonTools::GetValueOptional(config, "playGetIn",                       _params.playGetIn);
  JsonTools::GetValueOptional(config, "minCooldown_Sec",                 _params.minCoolDown_Sec);
  
  if( JsonTools::GetValueOptional(config, "maxTimeOldestFaceToConsider_Sec", _params.oldestFaceToConsider_MS) ) {
    _params.oldestFaceToConsider_MS *= 1000;
  }
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPeekABoo::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const float currentTime_Sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const Robot& robot = preReqData.GetRobot();
  // for COZMO-8914 - no way to play spark get out if no face is found during spark search
  // so run the peek a boo behavior with a flag set to indicate that we should just play the
  // spark get out animation
  if((robot.GetBehaviorManager().GetActiveSpark() == UnlockId::PeekABoo) &&
     robot.GetBehaviorManager().IsActiveSparkHard() &&
     (_timeSparkAboutToEnd_Sec > 0) &&
     (currentTime_Sec > _timeSparkAboutToEnd_Sec)){
    _timeSparkAboutToEnd_Sec = kSparkShouldPlaySparkFailFlag;
    return true;
  }
  
  // The sparked version of this behavior is grouped with look for faces behavior in case no faces were seen recently.
  _cachedFace = Vision::UnknownFaceID;

  return (_nextTimeIsRunnable_Sec < currentTime_Sec) &&
         (GetInteractionFace(robot) != Vision::UnknownFaceID) &&
         preReqData.GetRobot().GetContext()->GetFeatureGate()->IsFeatureEnabled(Anki::Cozmo::FeatureType::PeekABoo);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPeekABoo::InitInternal(Robot& robot)
{
  // for COZMO-8914
  if(_shouldStreamline &&
     (_timeSparkAboutToEnd_Sec == kSparkShouldPlaySparkFailFlag)){
    _timeSparkAboutToEnd_Sec = 0.0f;
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::SparkFailure));
    return RESULT_OK;
  }
  
  _hasMadeFollowUpRequest = false;
  _turnToFaceRetryCount = 0;
  _timestampEyeNotVisibleMap.clear();
  
  _noUserInteractionTimeout_Sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _params.noUserInteractionTimeout_Sec;
  _numPeeksTotal = _numPeeksRemaining = robot.GetRNG().RandIntInRange(_params.minPeeks, _params.maxPeeks);
  // Needs to be unique since sparked idle makes him look down
  robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::PeekABooIdle);
  SmartDisableReactionsWithLock(GetName(), kAffectTriggersPeekABooArray);
  
  
  if( _params.playGetIn )
  {
    TransitionToIntroAnim(robot);
  }
  else
  {
    TransitionTurnToFace(robot);
  }
  return RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorPeekABoo::UpdateInternal(Robot& robot)
{
  UpdateTimestampSets(robot);
  
  const bool  seeingEyes = !WasFaceHiddenAfterTimestamp(robot, robot.GetLastImageTimeStamp());
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Check to see if a face has appeared/disappeared every tick
  // these functions are their own callback, so allowing the callback
  // to run means that we have a holding loop with face tracking
  if(_currentState == State::WaitingToHideFace){
    if( !seeingEyes ) {
      StopActing(false);
      TransitionWaitToSeeFace(robot);
    }
    else if(_noUserInteractionTimeout_Sec < currentTime_s) {
      StopActing(false);
      TransitionToNoUserInteraction(robot);
      LOG_EVENT("robot.peekaboo_face_never_hidden","%u", _numPeeksRemaining);
    }
  }else if(_currentState == State::WaitingToSeeFace){
    if(seeingEyes){
      StopActing(false);
      TransitionSeeFaceAfterHiding(robot);
    }else if(_noUserInteractionTimeout_Sec < currentTime_s){
      StopActing(false);
      TransitionToNoUserInteraction(robot);
      LOG_EVENT("robot.peekaboo_face_never_came_back","%u", _numPeeksRemaining);
    }
  }
  
  return super::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::StopInternal(Robot& robot)
{
  robot.GetAnimationStreamer().PopIdleAnimation();
  _nextTimeIsRunnable_Sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _params.minCoolDown_Sec;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::TransitionToIntroAnim(Robot& robot)
{
  SET_STATE(DoingInitialReaction);
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PeekABooGetIn),&BehaviorPeekABoo::TransitionTurnToFace);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::TransitionTurnToFace(Robot& robot)
{
  SET_STATE(TurningToFace);
  TurnTowardsFaceAction* action = new TurnTowardsFaceAction(robot, GetInteractionFace(robot), M_PI_F, false);
  action->SetRequireFaceConfirmation(_params.requireFaceConfirmBeforeRequest);
  StartActing(action, [this, &robot](ActionResult ret )
  {
    if( ret == ActionResult::SUCCESS )
    {
      _turnToFaceRetryCount = 0;
      TransitionPlayPeekABooAnim(robot);
    }
    else
    {
      // If we've retried too many times for whatever reason, bail out
      // otherwise, retry if appropriate, ro try to select a new face to turn to
      ++_turnToFaceRetryCount;
      if(_turnToFaceRetryCount >= kMaxTurnToFaceRetryCount){
        TransitionExit(robot);
        return;
      }
      
      const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(ret);
      if(resCat == ActionResultCategory::RETRY){
        TransitionTurnToFace(robot);
      }else{
        // Failed because target face wasn't there, but maybe another one is
        if(GetInteractionFace(robot) != Vision::UnknownFaceID )
        {
          // Try to look for the next best face
          TransitionTurnToFace(robot);
        }
        else
        {
          // Failed because no faces were found
          TransitionExit(robot);
        }
      }
    }
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::TransitionPlayPeekABooAnim(Robot& robot)
{
  SET_STATE(RequestPeekaBooAnim);
  
  _lastRequestTime_Sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  // TODO: Peekaboo animations all end with the head looking up at a high angle
  // If the user doesn't have their face in this part of face world we have less
  // accuracy since we get a few frames for free at the end of the anim
  action->AddAction(new TriggerLiftSafeAnimationAction(robot, GetPeekABooAnimation()));
  if(kCenterFaceAfterPeekABoo){
    action->AddAction(new TurnTowardsFaceAction(robot, GetInteractionFace(robot)));
  }
  
  StartActing(action,[this](Robot& robot) {
    // If we saw a face in the frame buffer, assume that they haven't tried to peekaboo yet
    // if we didn't see a face, assume their face is hidden and they are about to finish the peekaboo
    const TimeStamp_t timestampHeadSteady = robot.GetLastImageTimeStamp();
    _noUserInteractionTimeout_Sec = _lastRequestTime_Sec + _params.noUserInteractionTimeout_Sec;
    if(WasFaceHiddenAfterTimestamp(robot, timestampHeadSteady)) {
      _stillSawFaceAfterRequest = false;
      TransitionWaitToSeeFace(robot);
    }
    else {
      _stillSawFaceAfterRequest = true;
      TransitionWaitToHideFace(robot);
    }
  });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::TransitionWaitToHideFace(Robot& robot)
{
  SET_STATE(WaitingToHideFace);
  // Transitioning out of this state is handled by the Update function
  StartActing(new TrackFaceAction(robot, GetInteractionFace(robot)));
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::TransitionWaitToSeeFace(Robot& robot)
{
  SET_STATE(WaitingToSeeFace);
  // Update should transition out of this state by canceling the action/callback
  // if we see eyes. Otherwise, if we wait until the no user interaciton timeout,
  // transition to no user interaction
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  StartActing(new WaitAction(robot, _noUserInteractionTimeout_Sec - currentTime_sec), [this](Robot& robot){
    LOG_EVENT("robot.peekaboo_face_never_came_back","%u", _numPeeksRemaining);
    TransitionToNoUserInteraction(robot);
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::TransitionSeeFaceAfterHiding(Robot& robot)
{
  SET_STATE(ReactingToPeekABooReturned);
  _numPeeksRemaining--;
  
  TimeStamp_t timeSinceStart = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _lastRequestTime_Sec;
  if( _stillSawFaceAfterRequest )
  {
    LOG_EVENT("robot.single_peekaboo_success.face_noface_face","%u",timeSinceStart);
  }
  else
  {
    LOG_EVENT("robot.single_peekaboo_success.noface_timepass_face","%u",timeSinceStart);
  }
  
  if(_numPeeksRemaining == 0) {
    TransitionExit(robot);
  }else{
    StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PeekABooSurprised),
                &BehaviorPeekABoo::TransitionTurnToFace);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::TransitionToNoUserInteraction(Robot& robot)
{
  SET_STATE(ReactingToNoUserInteraction);

  const bool shouldReRequest = (_numPeeksTotal == _numPeeksRemaining)  && !_hasMadeFollowUpRequest;
  _hasMadeFollowUpRequest = true;

  if(shouldReRequest){
    IActionRunner* failAnim = new TriggerAnimationAction(robot, AnimationTrigger::PeekABooNoUserInteraction);
    StartActing(failAnim, &BehaviorPeekABoo::TransitionTurnToFace);
  }else{
    TransitionExit(robot);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::TransitionExit(Robot& robot)
{
  SET_STATE(DoingFinalReaction);
  
  const bool anySuccessfullReactions = _numPeeksRemaining != _numPeeksTotal;
  
  // last state, just existing after this...
  StartActing(new TriggerLiftSafeAnimationAction(robot,
     anySuccessfullReactions ? AnimationTrigger::PeekABooGetOutHappy : AnimationTrigger::PeekABooGetOutSad));
  
  // Must be done after the animation so this plays
  BehaviorObjectiveAchieved(BehaviorObjective::PeekABooComplete);
  if(anySuccessfullReactions){
    BehaviorObjectiveAchieved(BehaviorObjective::PeekABooSuccess);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::UpdateTimestampSets(Robot& robot)
{
  // Prune list so it doesn't expand infinitely in size
  while(_timestampEyeNotVisibleMap.size() > kMaxCountTrackingEyesEntries){
    _timestampEyeNotVisibleMap.erase(_timestampEyeNotVisibleMap.begin());
  }
  
  // If no robot images have been received, don't bother updating
  if(!(_timestampEyeNotVisibleMap.size() == 0) &&
     (robot.GetLastImageTimeStamp() == _timestampEyeNotVisibleMap.rbegin()->first)){
    return;
  }
  
  
  const bool kRecognizableFacesOnly = true;
  std::set< Vision::FaceID_t > faceIDs = robot.GetFaceWorld().GetFaceIDsObservedSince(robot.GetLastImageTimeStamp(),
                                                                                      kRecognizableFacesOnly);
  // We originally kept a "Target face' to know where to initially turn, however
  // when they're constantly covering up their eyes it's likely our face ID is changing a lot. So just allow multiple faces
  // if multiple people are looking at cozmo this means it'll be easier for him to be happy.
  bool seeingFace = !faceIDs.empty();
  bool seeingEyes = false;
  if( seeingFace )
  {
    for(const auto& faceID : faceIDs)
    {
      const Vision::TrackedFace* face = robot.GetFaceWorld().GetFace(faceID);
      if(face != nullptr )
      {
        // If we've seen any eyes go for it...
        Vision::TrackedFace::Feature eyeFeature = face->GetFeature(Vision::TrackedFace::FeatureName::LeftEye);
        if( !eyeFeature.empty() )
        {
          seeingEyes = true;
          break;
        }
      }
    }
  }
  
  const int cumulativeEyeMissingCount = _timestampEyeNotVisibleMap.size() == 0 ? 1 :
                                          _timestampEyeNotVisibleMap.rbegin()->second + 1;
  
  const int cumulativeEyeMissingCountNoWrap = cumulativeEyeMissingCount > 0 ?
                                                cumulativeEyeMissingCount : INT_MAX;
 
  const int framesEyesMissingCount = seeingEyes ? 0 : cumulativeEyeMissingCountNoWrap;

  
  _timestampEyeNotVisibleMap.insert(std::make_pair(robot.GetLastImageTimeStamp(), framesEyesMissingCount));
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPeekABoo::WasFaceHiddenAfterTimestamp(Robot& robot, TimeStamp_t timestamp)
{
  // make sure the list is up to date
  if(_timestampEyeNotVisibleMap.rbegin()->first != robot.GetLastImageTimeStamp()){
    UpdateTimestampSets(robot);
  }
  
  bool anyCountOverThreshold = false;
  auto reverseIter = _timestampEyeNotVisibleMap.rbegin();
  while((reverseIter != _timestampEyeNotVisibleMap.rend()) &&
        (reverseIter->first >= timestamp)){
    if(reverseIter->second > kFramesWithoutFaceForPeek){
      anyCountOverThreshold = true;
      break;
    }
    ++reverseIter;
  }
  
  return anyCountOverThreshold;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vision::FaceID_t BehaviorPeekABoo::GetInteractionFace(const Robot& robot) const
{
  const bool kUseRecognizableOnly = false;
  std::set< Vision::FaceID_t > faces = robot.GetFaceWorld().GetFaceIDsObservedSince(
                    robot.GetLastImageTimeStamp() - _params.oldestFaceToConsider_MS, kUseRecognizableOnly);
  
  if(faces.find(_cachedFace) == faces.end()){
    const AIWhiteboard& whiteboard = robot.GetAIComponent().GetWhiteboard();
    _cachedFace = whiteboard.GetBestFaceToTrack(faces, false);
  }

  return _cachedFace;
}
 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorPeekABoo::GetPeekABooAnimation()
{
  const bool shouldMakeShortRequest = (_numPeeksTotal == _numPeeksRemaining)  && _hasMadeFollowUpRequest;

  if(shouldMakeShortRequest){
    return AnimationTrigger::PeekABooShort;
  }
  
  float percentComplete = 1.f - (Util::numeric_cast<float>(_numPeeksRemaining) / Util::numeric_cast<float>(_numPeeksTotal));
  
  AnimationTrigger playTrigger = AnimationTrigger::PeekABooHighIntensity;
  if( percentComplete < kPercentCompleteSmallReaction)
  {
    playTrigger = AnimationTrigger::PeekABooLowIntensity;
  }
  else if( percentComplete < kPercentCompleteMedReaction)
  {
    playTrigger = AnimationTrigger::PeekABooMedIntensity;
  }
  
  return playTrigger;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::SetState_internal(State state, const std::string& stateName)
{
  _currentState = state;
  SetDebugStateName(stateName);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPeekABoo::PeekABooSparkStarted(float sparkTimeout)
{
  const float bufferBeforeSparkEnd = 2.0;
  _timeSparkAboutToEnd_Sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()
                                   + sparkTimeout - bufferBeforeSparkEnd;
}

  

} // namespace Cozmo
} // namespace Anki

