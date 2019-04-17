/**
 * File: BehaviorCoordinateWhileInAir.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-23
 *
 * Description: Behavior responsible for managing the robot's behaviors when picked up/in the air
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/coordinators/behaviorCoordinateWhileInAir.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/components/animationComponent.h"
#include "engine/components/movementComponent.h"

namespace Anki {
namespace Vector {


namespace{
// The set of behaviors which maintain control of the robot, even when it's in the air
static const std::set<BehaviorID> kBehaviorStatesToSuppressInAirReaction = {{ BEHAVIOR_ID(BlackJack),
                                                                              BEHAVIOR_ID(HeldInPalmDispatcher),
                                                                              BEHAVIOR_ID(ReactToTouchPetting),
                                                                              BEHAVIOR_ID(TakeAPhotoCoordinator),
                                                                              BEHAVIOR_ID(WeatherResponses),
                                                                              BEHAVIOR_ID(ShowWallTime),
                                                                              BEHAVIOR_ID(SingletonCancelTimer),
                                                                              BEHAVIOR_ID(SingletonTimerAlreadySet),
                                                                              BEHAVIOR_ID(SingletonAnticShowClock),
                                                                              BEHAVIOR_ID(SingletonTimerCheckTime),
                                                                              BEHAVIOR_ID(SingletonTimerSet)  }};
  
static const std::set<BehaviorID> kBehaviorStatesToNotLockTracks =
  {{
    BEHAVIOR_ID(WhatsMyNameVoiceCommand),
    BEHAVIOR_ID(MeetVictor)
  }};

const BehaviorID kWhileInAirDispatcher = BEHAVIOR_ID(WhileInAirDispatcher);
const TimeStamp_t kMaxTimeForInitialPickupReaction_ms = 1000;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateWhileInAir::BehaviorCoordinateWhileInAir(const Json::Value& config)
: BehaviorDispatcherPassThrough(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateWhileInAir::~BehaviorCoordinateWhileInAir()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::InitPassThrough()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _whileInAirDispatcher = BC.FindBehaviorByID(kWhileInAirDispatcher);
  _initialPickupReaction = BC.FindBehaviorByID(BEHAVIOR_ID(InitialPickupAnimation));
  _onBackReaction = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToRobotOnBack));
  _onFaceReaction = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToRobotOnFace));
  _heldInPalmDispatcher = BC.FindBehaviorByID(BEHAVIOR_ID(HeldInPalmDispatcher));
  {
    _suppressInAirBehaviorSet = std::make_unique<AreBehaviorsActivatedHelper>(BC, kBehaviorStatesToSuppressInAirReaction);
    _dontLockTracksBehaviorSet = std::make_unique<AreBehaviorsActivatedHelper>(BC, kBehaviorStatesToNotLockTracks);
  }
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::OnPassThroughActivated() 
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::OnPassThroughDeactivated()
{
  _areTreadsLocked = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::PassThroughUpdate() 
{
  if( !IsActivated() ) {
    return;
  }

  const OffTreadsState offTreadsState = GetBEI().GetRobotInfo().GetOffTreadsState();

  if(offTreadsState == OffTreadsState::OnTreads){
    _lastTimeWasOnTreads_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  }

  const bool isInAirReactionPlaying = _whileInAirDispatcher->IsActivated() ||
                                      _onBackReaction->IsActivated() ||
                                      _onFaceReaction->IsActivated() ||
                                      _heldInPalmDispatcher->IsActivated();
  
  // Only lock tracks if the "picked up reaction" behavior is _not_ running (since the "react to picked up" animations
  // make use of the treads).
  //
  // Same applies to onBack and onFace reactions, and the HeldInPalmDispatcher as well. Since they can transition through
  // the InAir state while their reaction animations are playing or in between different behaviors of the held-in-palm
  // dispatcher, we don't want to lock tracks in the middle since that can cause the wheels to keep moving based on the
  // last BodyMotionKeyFrame that went through before the lock.
  if(((offTreadsState == OffTreadsState::InAir) ||
      (offTreadsState == OffTreadsState::Falling)) &&
     !isInAirReactionPlaying){
    SuppressInAirReactionIfAppropriate();
    LockTracksIfAppropriate();
    SuppressInitialPickupReactionIfAppropriate();
  }else{
    if(_areTreadsLocked){
      SmartUnLockTracks(GetDebugLabel());
      _areTreadsLocked = false;
    }
  }

  if(offTreadsState == OffTreadsState::OnTreads){
    _hasPickupReactionPlayed = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::LockTracksIfAppropriate()
{
  const bool shouldLockTracks = !_dontLockTracksBehaviorSet->AreBehaviorsActivated();
  if(!_areTreadsLocked && shouldLockTracks){
    SmartLockTracks(static_cast<u8>(AnimTrackFlag::BODY_TRACK), GetDebugLabel(), GetDebugLabel());
    _areTreadsLocked = true;
  } else if (_areTreadsLocked && !shouldLockTracks) {
    SmartUnLockTracks(GetDebugLabel());
    _areTreadsLocked = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::SuppressInAirReactionIfAppropriate()
{
  const bool shouldSuppress = _suppressInAirBehaviorSet->AreBehaviorsActivated();

  if(shouldSuppress){
    _whileInAirDispatcher->SetDontActivateThisTick(GetDebugLabel());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::SuppressInitialPickupReactionIfAppropriate()
{
  const EngineTimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  _hasPickupReactionPlayed |= (currTime_ms > (_lastTimeWasOnTreads_ms + kMaxTimeForInitialPickupReaction_ms));

  // Only play the pickup reaction
  if(_hasPickupReactionPlayed){
    _initialPickupReaction->SetDontActivateThisTick(GetDebugLabel());
  }else{
    if(_initialPickupReaction->IsActivated()){
      _hasPickupReactionPlayed = true;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::OnFirstPassThroughUpdate()
{
  // Safety check - behaviors that are  in the list need to internally list that they can be activated while in the air
  // or response will not work as expected.
  const auto& BC = GetBEI().GetBehaviorContainer();
  for(const auto& id : kBehaviorStatesToSuppressInAirReaction){
    const auto behaviorPtr = BC.FindBehaviorByID(id);
    const BehaviorOperationModifiers& modifiers = behaviorPtr->GetBehaviorOperationModifiersPostInit();
    ANKI_VERIFY(modifiers.wantsToBeActivatedWhenOffTreads,
                "BehaviorCoordinateWhileInAir.OnFirstUpdate.BehaviorCantRunInAir",
                "Behavior %s is listed as a behavior that suppresses the in air reaction, \
                but modifier says it can't run while in the air", BehaviorTypesWrapper::BehaviorIDToString(id));
    
  }
  
  for(const auto& id : kBehaviorStatesToNotLockTracks){
    const auto behaviorPtr = BC.FindBehaviorByID(id);
    const BehaviorOperationModifiers& modifiers = behaviorPtr->GetBehaviorOperationModifiersPostInit();
    ANKI_VERIFY(modifiers.wantsToBeActivatedWhenOffTreads,
                "BehaviorCoordinateWhileInAir.OnFirstUpdate.BehaviorCantRunInAir",
                "Behavior %s is listed as a behavior that unlocks the tracks in the air, \
                but modifier says it can't run while in the air", BehaviorTypesWrapper::BehaviorIDToString(id));
    
  }
}

}
}
