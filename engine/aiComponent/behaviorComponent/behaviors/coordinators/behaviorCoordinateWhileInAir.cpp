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
                                                                              BEHAVIOR_ID(ReactToTouchPetting),
                                                                              BEHAVIOR_ID(TakeAPhotoCoordinator),
                                                                              BEHAVIOR_ID(ShowWallTime),
                                                                              BEHAVIOR_ID(SingletonCancelTimer),
                                                                              BEHAVIOR_ID(SingletonTimerAlreadySet),
                                                                              BEHAVIOR_ID(SingletonAnticShowClock),
                                                                              BEHAVIOR_ID(SingletonTimerCheckTime),
                                                                              BEHAVIOR_ID(SingletonTimerSet)  }};

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
  {
    _suppressInAirBehaviorSet = std::make_unique<AreBehaviorsActivatedHelper>(BC, kBehaviorStatesToSuppressInAirReaction);
  }

  // Saftey check - behaviors that are  in the list need to internally list that they can be activated while in the air
  // or response will not work as expected
  if(ANKI_DEV_CHEATS){
    for(const auto& id : kBehaviorStatesToSuppressInAirReaction){
      // This one works, but due to init order dependencies the modifiers aren't set up properly
      if(id == BEHAVIOR_ID(SingletonTimerAlreadySet)){
        continue;
      }
      const auto behaviorPtr = BC.FindBehaviorByID(id);
      BehaviorOperationModifiers modifiers;
      behaviorPtr->GetBehaviorOperationModifiers(modifiers);
      ANKI_VERIFY(modifiers.wantsToBeActivatedWhenOffTreads,
                  "BehaviorCoordinateWhileInAir.InitPassThrough.BehaviorCantRunInAir",
                  "Behavior %s is listed as a behavior that suppresses the in air reaction, \
                  but modifier says it can't run while in the air", BehaviorTypesWrapper::BehaviorIDToString(id));
      
    }
  }
  
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::OnPassThroughActivated() 
{
  
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

  if((offTreadsState == OffTreadsState::InAir) ||
     (offTreadsState == OffTreadsState::Falling)){
    SuppressInAirReactionIfAppropriate();
    LockTracksIfAppropriate();
    SuppressInitialPickupReactionIfAppropriate();
  }else{
    if(_areTreadsLocked){
      GetBEI().GetMovementComponent().UnlockTracks(static_cast<u8>(AnimTrackFlag::BODY_TRACK), GetDebugLabel());
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
  if(!_areTreadsLocked){
    GetBEI().GetMovementComponent().LockTracks(static_cast<u8>(AnimTrackFlag::BODY_TRACK), GetDebugLabel(), GetDebugLabel());
    _areTreadsLocked = true;
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


}
}
