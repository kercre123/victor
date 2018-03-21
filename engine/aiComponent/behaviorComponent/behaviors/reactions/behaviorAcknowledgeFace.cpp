/**
 * File: behaviorAcknowledgeFace.cpp
 *
 * Author:  Andrew Stein
 * Created: 2016-06-16
 *
 * Description:  Simple quick reaction to a "new" face, just to show Cozmo has noticed you.
 *               Cozmo just turns towards the face and then plays a reaction animation.

 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorAcknowledgeFace.h"

#include "coretech/common/engine/utils/timer.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iReactToFaceListener.h"
#include "engine/aiComponent/faceSelectionComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace AcknowledgeFaceConsoleVars{
CONSOLE_VAR(u32, kNumImagesToWaitFor, "AcknowledgementBehaviors", 3);

CONSOLE_VAR(f32, kMaxTimeForInitialGreeting_s, "AcknowledgementBehaviors", 60.0f);
}

using namespace AcknowledgeFaceConsoleVars;
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAcknowledgeFace::BehaviorAcknowledgeFace(const Json::Value& config)
: super(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAcknowledgeFace::WantsToBeActivatedBehavior() const
{
  return !_desiredTargets.empty();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeFace::OnBehaviorActivated()
{
  // don't actually init until the first Update call. This gives other messages that came in this tick a
  // chance to be processed, in case we see multiple faces in the same tick.
  _shouldStart = true;

  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeFace::OnBehaviorDeactivated()
{
  for(auto& listener: _faceListeners){
    listener->ClearDesiredTargets();
  }
  _desiredTargets.clear();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeFace::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if( _shouldStart ) {
    _shouldStart = false;
    // now figure out which object to react to
    BeginIteration();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAcknowledgeFace::UpdateBestTarget()
{
  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  FaceSelectionComponent::FaceSelectionFactorMap criteriaMap;
  criteriaMap.insert(std::make_pair(FaceSelectionComponent::FaceSelectionPenaltyMultiplier::RelativeHeadAngleRadians, 1));
  criteriaMap.insert(std::make_pair(FaceSelectionComponent::FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians, 3));
  SmartFaceID bestFace  = faceSelection.GetBestFaceToUse(criteriaMap, _desiredTargets);
  
  if( !bestFace.IsValid() ) {
    return false;
  }
  else {
    _targetFace = bestFace;
    return true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeFace::BeginIteration()
{
  _targetFace.Reset();
  if( !UpdateBestTarget() ) {
    return;
  }

  const bool sayName = true;
  TurnTowardsFaceAction* turnAction = new TurnTowardsFaceAction(_targetFace,
                                                                M_PI_F,
                                                                sayName);

  const float freeplayStartedTime_s = 0.f;//robot.GetBehaviorManager().GetFirstTimeFreeplayStarted();    
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();  
  const bool withinMinSessionTime = freeplayStartedTime_s >= 0.0f &&
    (currTime_s - freeplayStartedTime_s) <= kMaxTimeForInitialGreeting_s;
  const bool alreadyTurnedTowards = GetBEI().GetFaceWorld().HasTurnedTowardsFace(_targetFace);
  const bool shouldPlayInitialGreeting = !_hasPlayedInitialGreeting && withinMinSessionTime && !alreadyTurnedTowards;

  PRINT_CH_INFO("Behaviors", "AcknowledgeFace.DoAcknowledgement",
                "currTime = %f, alreadyTurned:%d, shouldPlayGreeting:%d",
                currTime_s,
                alreadyTurnedTowards ? 1 : 0,
                shouldPlayInitialGreeting ? 1 : 0);
  
  if( shouldPlayInitialGreeting ) {
    auto& moodManager = GetBEI().GetMoodManager();
    turnAction->SetSayNameTriggerCallback([this, &moodManager](const Robot& robot, const SmartFaceID& faceID){
        // only play the initial greeting once, so if we are going to use it, mark that here
        _hasPlayedInitialGreeting = true;
        moodManager.TriggerEmotionEvent("GreetingSayName", MoodManager::GetCurrentTimeInSeconds());
        return AnimationTrigger::NamedFaceInitialGreeting;
      });
  }
  else {
    turnAction->SetSayNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceNamed);
  }
  
  // if it's not named, always play this one
  turnAction->SetNoNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceUnnamed);
  
  turnAction->SetMaxFramesToWait(kNumImagesToWaitFor);

  DelegateIfInControl(turnAction, &BehaviorAcknowledgeFace::FinishIteration);
} // InitInternalReactionary()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeFace::FinishIteration()
{
  _desiredTargets.erase( _targetFace );
 
  // notify the listeners that a face reaction has completed fully
  for(auto& listener: _faceListeners){
    listener->FinishedReactingToFace(_targetFace);
  }
  
  BehaviorObjectiveAchieved(BehaviorObjective::ReactedAcknowledgedFace);
  // move on to the next target, if there is one
  BeginIteration();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeFace::AddListener(IReactToFaceListener* listener)
{
  _faceListeners.insert(listener);
}



} // namespace Cozmo
} // namespace Anki

