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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iFeedingListener.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/animationComponent.h"
#include "engine/components/cubes/cubeAccelComponent.h"
#include "engine/components/cubes/cubeAccelListeners/movementListener.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotDataLoader.h"

#include "coretech/common/engine/utils/timer.h"
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
BehaviorFeedingEat::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingEat::DynamicVariables::DynamicVariables()
{
  currentState = State::DrivingToFood;
  hasRegisteredActionComplete = false;
  timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingEat::BehaviorFeedingEat(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::WantsToBeActivatedBehavior() const
{
  if(_dVars.targetID.IsSet()){
    if( IsCubeBad( _dVars.targetID ) ) {
      _dVars.targetID.SetToUnknown();
      return false;
    }
    
    const ObservableObject* obj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);

    // require a known object so we don't drive to and try to eat a moved cube
    const bool canRun = (obj != nullptr) && obj->IsPoseStateKnown();
    if(!canRun){
      _dVars.targetID.SetToUnknown();
    }
    return canRun;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::RemoveListeners(IFeedingListener* listener)
{
  size_t numRemoved = _iConfig.feedingListeners.erase(listener);
  return numRemoved > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::OnBehaviorActivated()
{
  if(GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID) == nullptr){
    return;
  }
  
  _dVars.timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
  _dVars.hasRegisteredActionComplete = false;

  // generic lambda closure for cube accel listeners
  auto movementDetectedCallback = [this] (const float movementScore) {
    CubeMovementHandler(movementScore);
  };
  
  if(GetBEI().HasCubeAccelComponent()){
    auto listener = std::make_shared<CubeAccelListeners::MovementListener>(kHighPassFiltCoef,
                                                                          kMaxMovementScoreToAdd,
                                                                          kMovementScoreDecay,
                                                                          kFeedingMovementScoreMax, // max allowed movement score
                                                                          movementDetectedCallback);
                                                                        
    GetBEI().GetCubeAccelComponent().AddListener(_dVars.targetID, listener);
    DEV_ASSERT(_iConfig.cubeMovementListener == nullptr,
             "BehaviorFeedingEat.InitInternal.PreviousListenerAlreadySetup");
    // keep a pointer to this listener around so that we can remove it later:
    _iConfig.cubeMovementListener = listener;
  }


  
  TransitionToDrivingToFood();
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  // Feeding should be considered "complete" so long as the animation has reached
  // the point where all light has been drained from the cube.  If the behavior
  // is interrupted after that point in the animation or the animation completes
  // successfully, register the action as complete.  If it's interrupted before
  // reaching that time (indicated by _dVars.timeCubeIsSuccessfullyDrained_sec) then
  // Cozmo didn't successfully finish "eating" and doesn't get the energy for it
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(!_dVars.hasRegisteredActionComplete &&
     (currentTime_s > _dVars.timeCubeIsSuccessfullyDrained_sec)){
    _dVars.hasRegisteredActionComplete = true;
    for(auto& listener: _iConfig.feedingListeners){
      listener->EatingComplete();
    }
  }

  
  if((_dVars.currentState != State::ReactingToInterruption) &&
     (GetBEI().GetOffTreadsState() != OffTreadsState::OnTreads) &&
     !_dVars.hasRegisteredActionComplete){
    TransitionToReactingToInterruption();
  }  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::CubeMovementHandler(const float movementScore)
{
  // Logic for determining whether the player has "stolen" cozmo's cube while he's
  // eating.  We only want to respond if the player pulls the cube away while
  // Cozmo is actively in the "eating" stage and has not drained the cube yet

  if(GetBEI().GetRobotInfo().IsPhysical()){
    if(movementScore > kCubeMovedTooFastInterrupt){
      const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      const bool currentlyEating = (_dVars.currentState == State::Eating) &&
                       (_dVars.timeCubeIsSuccessfullyDrained_sec > currentTime_s);
      
      if(currentlyEating ||
         (_dVars.currentState == State::PlacingLiftOnCube)){
        CancelDelegates(false);
        TransitionToReactingToInterruption();
      }else if(_dVars.currentState == State::DrivingToFood){
        CancelDelegates(false);
      }
      
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::OnBehaviorDeactivated()
{
  // If the behavior is being stopped while feeding is still ongoing notify
  // listeners that feeding is being interrupted
  if(!_dVars.hasRegisteredActionComplete &&
     (_dVars.currentState >= State::PlacingLiftOnCube)){
    for(auto& listener: _iConfig.feedingListeners){
      listener->EatingInterrupted();
    }
  }


  GetBEI().GetRobotInfo().EnableStopOnCliff(true);
  
  // Release the cube accel listener
  _iConfig.cubeMovementListener.reset();
  _dVars.targetID.UnSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToDrivingToFood()
{
  SET_STATE(DrivingToFood);
  const ObservableObject* obj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);
  if(obj == nullptr){
    return;
  }

  DriveToAlignWithObjectAction* action = new DriveToAlignWithObjectAction(_dVars.targetID,
                                                                          kDistanceFromMarker_mm);
  action->SetPreActionPoseAngleTolerance(DEG_TO_RAD(kFeedingPreActionAngleTol_deg));
  
  DelegateIfInControl(action, [this](ActionResult result){
    if( result == ActionResult::SUCCESS ){
      TransitionToPlacingLiftOnCube();
    }
    else if( result == ActionResult::VISUAL_OBSERVATION_FAILED ) {
      // can't see the cube, maybe it's obstructed? give up on the cube until we see it again. Let the
      // behavior end (it may get re-selected with a different cube)
      MarkCubeAsBad();
    } else {
      const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(result);

      if( resCat == ActionResultCategory::RETRY ) {
        TransitionToDrivingToFood();
      }
      else {
        // something else is wrong. Make this cube invalid, let the behavior end
        MarkCubeAsBad();
      }
    }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToPlacingLiftOnCube()
{
  SET_STATE(PlacingLiftOnCube);
  
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::DEPRECATED_FeedingPlaceLiftOnCube),
                      &BehaviorFeedingEat::TransitionToEating);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToEating()
{
  SET_STATE(Eating);
  GetBEI().GetRobotInfo().EnableStopOnCliff(false);
  
  AnimationTrigger eatingAnim = AnimationTrigger::DEPRECATED_FeedingEat;
  
  uint32_t timeDrainCube_s = 0;
  auto* data_ldr = GetBEI().GetRobotInfo().GetContext()->GetDataLoader();
  if( data_ldr->HasAnimationForTrigger(eatingAnim) )
  {
    // Extract the length of time that the animation will be playing for so that
    // it can be passed through to listeners
    const auto& animComponent = GetBEI().GetAnimationComponent();
    const auto& animGroupName = data_ldr->GetAnimationForTrigger(eatingAnim);
    const auto& animName = animComponent.GetAnimationNameFromGroup(animGroupName);

    AnimationComponent::AnimationMetaInfo metaInfo;
    if (animComponent.GetAnimationMetaInfo(animName, metaInfo) == RESULT_OK) {
      timeDrainCube_s = Util::MilliSecToSec((float)metaInfo.length_ms);
    } else {
      PRINT_NAMED_WARNING("BehaviorFeedingEat.TransitionToEating.AnimationLengthNotFound", "Anim: %s", animName.c_str());
      timeDrainCube_s = 2.f;
    }
  }
  
  
  for(auto & listener: _iConfig.feedingListeners){
    if(ANKI_VERIFY(listener != nullptr,
                   "BehaviorFeedingEat.TransitionToEating.ListenerIsNull",
                   "")) {
      listener->StartedEating(timeDrainCube_s);
    }
  }
  
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.timeCubeIsSuccessfullyDrained_sec = currentTime_s + timeDrainCube_s;
  
  // DelegateIfInControl(new TriggerAnimationAction(eatingAnim)); // TEMP: only for this branch
  DelegateIfInControl(new PlayAnimationAction("anim_energy_eat_01")); // TEMP: 
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToReactingToInterruption()
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool currentlyEating = (_dVars.currentState == State::Eating) &&
                 (_dVars.timeCubeIsSuccessfullyDrained_sec > currentTime_s);
  
  if(currentlyEating ||
     (_dVars.currentState == State::PlacingLiftOnCube)){
    for(auto& listener: _iConfig.feedingListeners){
      listener->EatingInterrupted();
    }
  }
  
  SET_STATE(ReactingToInterruption);
  _dVars.timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
  
  CancelDelegates(false);
  AnimationTrigger trigger = AnimationTrigger::DEPRECATED_FeedingInterrupted;
  
  {
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(trigger));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::MarkCubeAsBad()
{
  if( ! ANKI_VERIFY(_dVars.targetID.IsSet(), "BehaviorFeedingEat.MarkCubeAsBad.NoTargetID",
                    "Behavior %s trying to mark target cube as bad, but target is unset",
                    GetDebugLabel().c_str()) ) {
    return;
  }

  const TimeStamp_t lastPoseUpdateTime_ms = GetBEI().GetObjectPoseConfirmer().GetLastPoseUpdatedTime(_dVars.targetID);
  _dVars.badCubesMap[_dVars.targetID] = lastPoseUpdateTime_ms;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::IsCubeBad(const ObjectID& objectID) const
{ 
  const TimeStamp_t lastPoseUpdateTime_ms = GetBEI().GetObjectPoseConfirmer().GetLastPoseUpdatedTime(objectID);

  auto iter = _dVars.badCubesMap.find( objectID );
  if( iter != _dVars.badCubesMap.end() ) {
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
  _dVars.currentState = state;
  SetDebugStateName(stateName);
}


  
} // namespace Cozmo
} // namespace Anki

