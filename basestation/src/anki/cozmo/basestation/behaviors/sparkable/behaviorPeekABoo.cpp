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
#include "util/math/numericCast.h"

namespace Anki {
namespace Cozmo {

BehaviorPeekABoo::BehaviorPeekABoo(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
  , _targetFace(Vision::UnknownFaceID)
  , _numPeeksRemaining(0)
  , _numPeeksTotal(1)
  , _numTimesHumanHidAndShow(0)
  , _minPeeks(1)
  , _maxPeeks(3)
  , _percentCompleteSmallReaction(0.3f)
  , _percentCompleteMedReaction(0.6f)
  , _timeoutNoFacesTimeStamp_Sec(30.f)
  , _waitIncrementTime_Sec(0.1f)
  , _oldestFaceToConsider_MS(60000)
  , _waitTimeFirst_Sec(0.4f)
  , _wantsShortReaction(false)
{
  JsonTools::GetValueOptional(config, "minTimesPeekBeforeQuit", _minPeeks);
  JsonTools::GetValueOptional(config, "maxTimesPeekBeforeQuit", _maxPeeks);
  JsonTools::GetValueOptional(config, "maxTimeWithoutSeeingFace_Sec", _timeoutNoFacesTimeStamp_Sec);
  JsonTools::GetValueOptional(config, "waitIncrementTime_Sec", _waitIncrementTime_Sec);
  if( JsonTools::GetValueOptional(config, "maxTimeOldestFaceToConsider_Sec", _oldestFaceToConsider_MS) ) {
    _oldestFaceToConsider_MS *= 1000;
  }
  JsonTools::GetValueOptional(config, "waitTimeFirstSec",_waitTimeFirst_Sec);
}

bool BehaviorPeekABoo::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  // The sparked version of this behavior is grouped with look for faces behavior in case no faces were seen recently.
  _targetFace = Vision::UnknownFaceID;
  SelectFaceToTrack(preReqData.GetRobot());
  
  return _targetFace != Vision::UnknownFaceID &&
  preReqData.GetRobot().GetContext()->GetFeatureGate()->IsFeatureEnabled(Anki::Cozmo::FeatureType::PeekABoo);
}

Result BehaviorPeekABoo::InitInternal(Robot& robot)
{
  _timeSinceSeen_ms = robot.GetLastImageTimeStamp();
  
  _endTimeStamp = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _timeoutNoFacesTimeStamp_Sec;
  _numPeeksTotal = _numPeeksRemaining = robot.GetRNG().RandIntInRange(_minPeeks,_maxPeeks);
  _numTimesHumanHidAndShow = 0;
  // Needs to be unique since sparked idle makes him look down
  robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::PeekABooIdle);
  
  TransitionToIntroAnim(robot);
  
  return RESULT_OK;
}

void BehaviorPeekABoo::StopInternal(Robot& robot)
{
  robot.GetAnimationStreamer().PopIdleAnimation();
}

void BehaviorPeekABoo::TransitionToIntroAnim(Robot& robot)
{
  DEBUG_SET_STATE(DoingInitialReaction);
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PeekABooGetIn),&BehaviorPeekABoo::TransitionTurnToFace);
}

void BehaviorPeekABoo::TransitionTurnToFace(Robot& robot)
{
  DEBUG_SET_STATE(TurningToFace);
  TurnTowardsFaceAction* action = new TurnTowardsFaceAction(robot, _targetFace, M_PI_F, false);
  action->SetRequireFaceConfirmation(true);
  StartActing(action, [this, &robot](ActionResult ret )
  {
    if( ret == ActionResult::SUCCESS )
    {
      TransitionReactToFace(robot);
    }
    else
    {
      // Failed because target face wasn't there, but maybe another one is
      Vision::FaceID_t oldTargetFace = _targetFace;
      SelectFaceToTrack(robot);
      if( _targetFace != oldTargetFace && _targetFace != Vision::UnknownFaceID )
      {
        // Try to look for the next best face
        TransitionTurnToFace(robot);
      }
      else
      {
        // Failed because no faces were found
        TransitionExit(robot, false);
      }
    }
  });
}
  
void BehaviorPeekABoo::TransitionReactToFace(Robot& robot)
{
  DEBUG_SET_STATE(RequestPeekaBooAnim);
  
  _endTimeStamp = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _timeoutNoFacesTimeStamp_Sec;
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  float percentComplete = 1.f - (Util::numeric_cast<float>(_numPeeksRemaining) / Util::numeric_cast<float>(_numPeeksTotal));
  
  AnimationTrigger playTrigger = AnimationTrigger::PeekABooHighIntensity;
  if( percentComplete < _percentCompleteSmallReaction)
  {
    playTrigger = AnimationTrigger::PeekABooLowIntensity;
  }
  else if( percentComplete < _percentCompleteMedReaction)
  {
    playTrigger = AnimationTrigger::PeekABooMedIntensity;
  }
  action->AddAction(new TriggerLiftSafeAnimationAction(robot, _wantsShortReaction ?
                                                       AnimationTrigger::PeekABooShort : playTrigger));
  action->AddAction( new WaitAction(robot, _waitTimeFirst_Sec) );
  StartActing(action,[this, &robot](ActionResult ret ) {
    _wantsShortReaction = false;
    // If they just stared at the robot the whole time, wait for them to hide their face, eventually prompting again.
    // if no face is hidden, we assume they accepted the call to action to peek a boo back and just need to wait to show again.
    if( IsFaceHiddenNow(robot) ) {
      TransitionWaitToSeeFace(robot);
    }
    else {
      TransitionWaitToHideFace(robot);
    }
  });
}

void BehaviorPeekABoo::TransitionWaitToHideFace(Robot& robot)
{
  DEBUG_SET_STATE(WaitingForHiddenFace);
  if( IsFaceHiddenNow(robot) ) {
    TransitionWaitToSeeFace(robot);
  }
  else {
    // If we just sat there and stared for several seconds, restart
    _timeSinceSeen_ms = robot.GetLastImageTimeStamp();
    // just run out of time in the behavior, or been too long since we've seen a face and have been looking for one...
    if( _endTimeStamp < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() )
    {
      _wantsShortReaction = true;
      TransitionTurnToFace(robot);
    }
    else
    {
      // we just need to keep looping this waiting behavior, eventually seeing a face or hitting the timeout
      StartActing(new WaitAction(robot, _waitIncrementTime_Sec), &BehaviorPeekABoo::TransitionWaitToHideFace);
    }
  }
}

bool BehaviorPeekABoo::IsFaceHiddenNow(Robot& robot)
{
  const bool kRecognizableFacesOnly = true;
  std::set< Vision::FaceID_t > faceIDs = robot.GetFaceWorld().GetFaceIDsObservedSince(_timeSinceSeen_ms,
                                                                                      kRecognizableFacesOnly);
  // We originally kept a "Target face' to know where to initially turn, however
  // when they're constantly covering up their eyes it's likely our face ID is changing a lot. So just allow multiple faces
  // if multiple people are looking at cozmo this means it'll be easier for him to be happy.
  bool seeingFace = !faceIDs.empty();
  bool seeingEyes = false;
  if( seeingFace  )
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
  return !seeingEyes;
}
  
void BehaviorPeekABoo::TransitionWaitToSeeFace(Robot& robot)
{
  DEBUG_SET_STATE(WaitingToSeeFace);
  // If it's been too long, time out
  // else if we see a face, play a reaction and start over
  
  bool seeingEyes = !IsFaceHiddenNow(robot);
  
  _timeSinceSeen_ms = robot.GetLastImageTimeStamp();
  // just run out of time in the behavior, or been too long since we've seen a face and have been looking for one...
  if( _endTimeStamp < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() )
  {
    TransitionExit(robot,false);
  }
  else if( seeingEyes )
  {
    _numTimesHumanHidAndShow++;
    TransitionSeeFaceAfterHiding(robot);
  }
  else
  {
    StartActing(new WaitAction(robot, _waitIncrementTime_Sec), &BehaviorPeekABoo::TransitionWaitToSeeFace);
  }
}
  
void BehaviorPeekABoo::TransitionSeeFaceAfterHiding(Robot& robot)
{
  DEBUG_SET_STATE(ReactingToPeekABooReturned);
  SelectFaceToTrack(robot);
  _numPeeksRemaining--;
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PeekABooSurprised),
              [this, &robot](ActionResult ret ) {
                if( _numPeeksRemaining > 0 ) {
                  TransitionTurnToFace(robot);
                }
                else {
                  TransitionExit(robot,true);
                }
  });
}
  
void BehaviorPeekABoo::TransitionExit(Robot& robot, bool facesFound)
{
  DEBUG_SET_STATE(DoingFinalReaction);
  // This is so spark is killed after the exit animation is played
  // Also the number of times the behavior is achieved is randomized so can't just use behavior_config value.
  // also time out differ based on if he is seeing a face, or just never saw anything...
  BehaviorObjectiveAchieved(BehaviorObjective::PeekedABoo);
  
  LOG_EVENT("peekaboo.ending","%s %d/%d",GetName().c_str(),_numTimesHumanHidAndShow, ( _numPeeksTotal - _numPeeksRemaining));
  // last state, just existing after this...
  StartActing(new TriggerLiftSafeAnimationAction(robot,
                                                facesFound ? AnimationTrigger::PeekABooGetOutHappy : AnimationTrigger::PeekABooGetOutSad));
}
  
void BehaviorPeekABoo::SelectFaceToTrack(const Robot& robot) const
{
  const bool kUseRecognizableOnly = false;
  std::set< Vision::FaceID_t > faces = robot.GetFaceWorld().GetFaceIDsObservedSince(robot.GetLastImageTimeStamp() - _oldestFaceToConsider_MS, kUseRecognizableOnly);
  
  const AIWhiteboard& whiteboard = robot.GetAIComponent().GetWhiteboard();
  _targetFace = whiteboard.GetBestFaceToTrack(faces, false);
  
}

}
}

