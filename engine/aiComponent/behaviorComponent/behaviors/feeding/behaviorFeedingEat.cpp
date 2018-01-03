/**
* File: behaviorFeedingEat.cpp
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to interact with an "energy" filled cube
* and drain the energy out of it
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iFeedingListener.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/animationComponent.h"
#include "engine/components/cubeAccelComponent.h"
#include "engine/components/cubeAccelComponentListeners.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"

#include "coretech/common/engine/utils/timer.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/console/consoleInterface.h"
#include "util/math/math.h"


namespace Anki {
namespace Cozmo {
  
namespace{
#define SET_STATE(s) SetState_internal(State::s, #s)

#define CONSOLE_GROUP "Behavior.FeedingEat"
  
CONSOLE_VAR(f32, kDistanceFromMarker_mm, CONSOLE_GROUP,  45.0f);
  
// Constants for the CubeAccelComponent MovementListener:
CONSOLE_VAR(f32, kHighPassFiltCoef,          CONSOLE_GROUP,  0.4f);
CONSOLE_VAR(f32, kMaxMovementScoreToAdd,     CONSOLE_GROUP,  3.f);
CONSOLE_VAR(f32, kMovementScoreDecay,        CONSOLE_GROUP,  2.f);
CONSOLE_VAR(f32, kFeedingMovementScoreMax,   CONSOLE_GROUP,  100.f);
CONSOLE_VAR(f32, kCubeMovedTooFastInterrupt, CONSOLE_GROUP,  8.f);

CONSOLE_VAR(f32, kFeedingPreActionAngleTol_deg, CONSOLE_GROUP, 15.0f);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingEat::BehaviorFeedingEat(const Json::Value& config)
: ICozmoBehavior(config)
, _timeCubeIsSuccessfullyDrained_sec(FLT_MAX)
, _hasRegisteredActionComplete(false)
, _currentState(State::DrivingToFood)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if(_targetID.IsSet()){
    if( IsCubeBad(behaviorExternalInterface, _targetID ) ) {
      _targetID.SetToUnknown();
      return false;
    }
    
    const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);

    // require a known object so we don't drive to and try to eat a moved cube
    const bool canRun = (obj != nullptr) && obj->IsPoseStateKnown();
    if(!canRun){
      _targetID.SetToUnknown();
    }
    return canRun;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::RemoveListeners(IFeedingListener* listener)
{
  size_t numRemoved = _feedingListeners.erase(listener);
  return numRemoved > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID) == nullptr){
    return;
  }
  
  _timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
  _hasRegisteredActionComplete = false;

  // generic lambda closure for cube accel listeners
  auto movementDetectedCallback = [this, &behaviorExternalInterface] (const float movementScore) {
    CubeMovementHandler(behaviorExternalInterface, movementScore);
  };
  
  if(behaviorExternalInterface.HasCubeAccelComponent()){
    auto listener = std::make_shared<CubeAccelListeners::MovementListener>(kHighPassFiltCoef,
                                                                          kMaxMovementScoreToAdd,
                                                                          kMovementScoreDecay,
                                                                          kFeedingMovementScoreMax, // max allowed movement score
                                                                          movementDetectedCallback);
                                                                        
    behaviorExternalInterface.GetCubeAccelComponent().AddListener(_targetID, listener);
    DEV_ASSERT(_cubeMovementListener == nullptr,
             "BehaviorFeedingEat.InitInternal.PreviousListenerAlreadySetup");
    // keep a pointer to this listener around so that we can remove it later:
    _cubeMovementListener = listener;
  }


  
  TransitionToDrivingToFood(behaviorExternalInterface);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(!IsActivated()){
    return;
  }

  // Feeding should be considered "complete" so long as the animation has reached
  // the point where all light has been drained from the cube.  If the behavior
  // is interrupted after that point in the animation or the animation completes
  // successfully, register the action as complete.  If it's interrupted before
  // reaching that time (indicated by _timeCubeIsSuccessfullyDrained_sec) then
  // Cozmo didn't successfully finish "eating" and doesn't get the energy for it
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(!_hasRegisteredActionComplete &&
     (currentTime_s > _timeCubeIsSuccessfullyDrained_sec)){
    _hasRegisteredActionComplete = true;
    if(behaviorExternalInterface.HasNeedsManager()){
      auto& needsManager = behaviorExternalInterface.GetNeedsManager();
      needsManager.RegisterNeedsActionCompleted(NeedsActionId::Feed);
    }
    for(auto& listener: _feedingListeners){
      listener->EatingComplete(behaviorExternalInterface);
    }
  }

  
  if((_currentState != State::ReactingToInterruption) &&
     (behaviorExternalInterface.GetOffTreadsState() != OffTreadsState::OnTreads) &&
     !_hasRegisteredActionComplete){
    TransitionToReactingToInterruption(behaviorExternalInterface);
  }  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::CubeMovementHandler(BehaviorExternalInterface& behaviorExternalInterface, const float movementScore)
{
  // Logic for determining whether the player has "stolen" cozmo's cube while he's
  // eating.  We only want to respond if the player pulls the cube away while
  // Cozmo is actively in the "eating" stage and has not drained the cube yet

  if(behaviorExternalInterface.GetRobotInfo().IsPhysical()){
    if(movementScore > kCubeMovedTooFastInterrupt){
      const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      const bool currentlyEating = (_currentState == State::Eating) &&
                       (_timeCubeIsSuccessfullyDrained_sec > currentTime_s);
      
      if(currentlyEating ||
         (_currentState == State::PlacingLiftOnCube)){
        CancelDelegates(false);
        TransitionToReactingToInterruption(behaviorExternalInterface);
      }else if(_currentState == State::DrivingToFood){
        CancelDelegates(false);
      }
      
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // If the behavior is being stopped while feeding is still ongoing notify
  // listeners that feeding is being interrupted
  if(!_hasRegisteredActionComplete &&
     (_currentState >= State::PlacingLiftOnCube)){
    for(auto& listener: _feedingListeners){
      listener->EatingInterrupted(behaviorExternalInterface);
    }
  }


  behaviorExternalInterface.GetRobotInfo().EnableStopOnCliff(true);
  
  const bool removeSuccessfull = behaviorExternalInterface.HasCubeAccelComponent() &&
    behaviorExternalInterface.GetCubeAccelComponent().RemoveListener(_targetID, _cubeMovementListener);
  ANKI_VERIFY(removeSuccessfull,
             "BehaviorFeedingEat.StopInternal.FailedToRemoveAccellComponent",
              "");
  _cubeMovementListener.reset();
  _targetID.UnSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToDrivingToFood(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(DrivingToFood);
  const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj == nullptr){
    return;
  }

  DriveToAlignWithObjectAction* action = new DriveToAlignWithObjectAction(_targetID,
                                                                          kDistanceFromMarker_mm);
  action->SetPreActionPoseAngleTolerance(DEG_TO_RAD(kFeedingPreActionAngleTol_deg));
  
  DelegateIfInControl(action, [this, &behaviorExternalInterface](ActionResult result){
    if( result == ActionResult::SUCCESS ){
      TransitionToPlacingLiftOnCube(behaviorExternalInterface);
    }
    else if( result == ActionResult::VISUAL_OBSERVATION_FAILED ) {
      // can't see the cube, maybe it's obstructed? give up on the cube until we see it again. Let the
      // behavior end (it may get re-selected with a different cube)
      MarkCubeAsBad(behaviorExternalInterface);
    } else if( result == ActionResult::NO_PREACTION_POSES){
      behaviorExternalInterface.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
    } else {
      const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(result);

      if( resCat == ActionResultCategory::RETRY ) {
        TransitionToDrivingToFood(behaviorExternalInterface);
      }
      else {
        // something else is wrong. Make this cube invalid, let the behavior end
        MarkCubeAsBad(behaviorExternalInterface);
      }
    }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToPlacingLiftOnCube(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(PlacingLiftOnCube);
  
  bool isNeedSevere = false;
  if(behaviorExternalInterface.HasNeedsManager()){
    auto& needsManager = behaviorExternalInterface.GetNeedsManager();
    
    NeedsState& currNeedState = needsManager.GetCurNeedsStateMutable();
    isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy,
                                                 NeedBracketId::Critical);
  }
  
  AnimationTrigger bestAnim = isNeedSevere ?
                               AnimationTrigger::FeedingPlaceLiftOnCube_Severe :
                               AnimationTrigger::FeedingPlaceLiftOnCube_Normal;
  
  DelegateIfInControl(new TriggerAnimationAction(bestAnim),
              &BehaviorFeedingEat::TransitionToEating);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToEating(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(Eating);
  behaviorExternalInterface.GetRobotInfo().EnableStopOnCliff(false);
  
  AnimationTrigger eatingAnim = CheckNeedsStateAndCalculateAnimation(behaviorExternalInterface);
  
  uint32_t timeDrainCube_s = 0;
  RobotManager* robot_mgr = behaviorExternalInterface.GetRobotInfo().GetContext()->GetRobotManager();
  if( robot_mgr->HasAnimationForTrigger(eatingAnim) )
  {
    // Extract the length of time that the animation will be playing for so that
    // it can be passed through to listeners
    const auto& animComponent = behaviorExternalInterface.GetAnimationComponent();
    const auto& animGroupName = robot_mgr->GetAnimationForTrigger(eatingAnim);
    const auto& animName = animComponent.GetAnimationNameFromGroup(animGroupName);

    AnimationComponent::AnimationMetaInfo metaInfo;
    if (animComponent.GetAnimationMetaInfo(animName, metaInfo) == RESULT_OK) {
      timeDrainCube_s = Util::MilliSecToSec((float)metaInfo.length_ms);
    } else {
      PRINT_NAMED_WARNING("BehaviorFeedingEat.TransitionToEating.AnimationLengthNotFound", "Anim: %s", animName.c_str());
      timeDrainCube_s = 2.f;
    }
  }
  
  
  for(auto & listener: _feedingListeners){
    if(ANKI_VERIFY(listener != nullptr,
                   "BehaviorFeedingEat.TransitionToEating.ListenerIsNull",
                   "")) {
      listener->StartedEating(behaviorExternalInterface, timeDrainCube_s);
    }
  }
  
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeCubeIsSuccessfullyDrained_sec = currentTime_s + timeDrainCube_s;
  
  // DelegateIfInControl(new TriggerAnimationAction(eatingAnim)); // TEMP: only for this branch
  DelegateIfInControl(new PlayAnimationAction("anim_energy_eat_01")); // TEMP: 
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToReactingToInterruption(BehaviorExternalInterface& behaviorExternalInterface)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool currentlyEating = (_currentState == State::Eating) &&
                 (_timeCubeIsSuccessfullyDrained_sec > currentTime_s);
  
  if(currentlyEating ||
     (_currentState == State::PlacingLiftOnCube)){
    for(auto& listener: _feedingListeners){
      listener->EatingInterrupted(behaviorExternalInterface);
    }
  }
  
  SET_STATE(ReactingToInterruption);
  _timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
  
  CancelDelegates(false);
  AnimationTrigger trigger = AnimationTrigger::FeedingInterrupted;
  
  if(NeedId::Energy == behaviorExternalInterface.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression()){
    trigger = AnimationTrigger::FeedingInterrupted_Severe;
  }
  {
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(trigger));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorFeedingEat::CheckNeedsStateAndCalculateAnimation(BehaviorExternalInterface& behaviorExternalInterface)
{
  bool isSeverePreFeeding  = false;
  bool isWarningPreFeeding = false;
  bool isSeverePostFeeding = false;
  bool isWarningPostFeeding = false;
  bool isFullPostFeeding = false;
  // Eating animation is dependent on both the current and post feeding energy level
  // Use PredictNeedsActionResult to estimate the ending needs bracket
  if(behaviorExternalInterface.HasNeedsManager()){
    auto& needsManager = behaviorExternalInterface.GetNeedsManager();
    NeedsState& currNeedState = needsManager.GetCurNeedsStateMutable();
    
    isSeverePreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
    isWarningPreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Warning);
    
    NeedsState predPostFeedNeed;
    needsManager.PredictNeedsActionResult(NeedsActionId::Feed, predPostFeedNeed);
    
    isSeverePostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                           NeedBracketId::Critical);
    isWarningPostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                            NeedBracketId::Warning);
    isFullPostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                         NeedBracketId::Full);
  }
  AnimationTrigger bestAnimation;
  if(isSeverePreFeeding && isSeverePostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteNotFullEnough_Severe;
  }else if(isSeverePreFeeding && isWarningPostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteFullEnough_Severe;
  }else if(isWarningPreFeeding && !isFullPostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteNotFullEnough_Normal;
  }else{
    bestAnimation = AnimationTrigger::FeedingAteFullEnough_Normal;
  }
  
  PRINT_CH_INFO("Feeding",
                "BehaviorFeedingEat.UpdateNeedsStateCalcAnim.AnimationSelected",
                "AnimationTrigger: %s SeverePreFeeding: %d severePostFeeding: %d warningPreFeeding: %d fullyFullPost: %d ",
                AnimationTriggerToString(bestAnimation),
                isSeverePreFeeding, isSeverePostFeeding,
                isWarningPreFeeding, isFullPostFeeding);
  
  return bestAnimation;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::MarkCubeAsBad(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( ! ANKI_VERIFY(_targetID.IsSet(), "BehaviorFeedingEat.MarkCubeAsBad.NoTargetID",
                    "Behavior %s trying to mark target cube as bad, but target is unset",
                    GetIDStr().c_str()) ) {
    return;
  }

  const TimeStamp_t lastPoseUpdateTime_ms = behaviorExternalInterface.GetObjectPoseConfirmer().GetLastPoseUpdatedTime(_targetID);
  _badCubesMap[_targetID] = lastPoseUpdateTime_ms;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::IsCubeBad(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& objectID) const
{ 
  const TimeStamp_t lastPoseUpdateTime_ms = behaviorExternalInterface.GetObjectPoseConfirmer().GetLastPoseUpdatedTime(objectID);

  auto iter = _badCubesMap.find( objectID );
  if( iter != _badCubesMap.end() ) {
    if( lastPoseUpdateTime_ms <= iter->second ) {
      // cube hasn't been re-observed, so is bad (shouldn't be used by the behavior
      return true;
    }
    // otherwise, the cube was invalid, but has a new pose, so consider it OK
  }

  // cube isn't bad
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::SetState_internal(State state, const std::string& stateName)
{
  _currentState = state;
  SetDebugStateName(stateName);
}


  
} // namespace Cozmo
} // namespace Anki

