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

#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/components/animationComponent.h"

namespace Anki {
namespace Cozmo {


namespace{
// The set of behaviors which can run while in the air, but will have their treads locked
static const std::set<BehaviorID> kBehaviorsToLockTreadsWhileInAir = {{ BEHAVIOR_ID(BlackJack),
                                                                        BEHAVIOR_ID(ReactToTouchPetting),
                                                                        BEHAVIOR_ID(TakeAPhotoCoordinator),
                                                                        BEHAVIOR_ID(ShowWallTime),
                                                                        BEHAVIOR_ID(SingletonCancelTimer),
                                                                        BEHAVIOR_ID(SingletonTimerAlreadySet),
                                                                        BEHAVIOR_ID(SingletonTimerAntic),
                                                                        BEHAVIOR_ID(SingletonTimerCheckTime),
                                                                        BEHAVIOR_ID(SingletonTimerSet) }};

// The set of behaviors which maintain control of the robot, even when it's in the air
static const std::set<BehaviorID> kBehaviorStatesToSuppressInAirReaction = {{ BEHAVIOR_ID(BlackJack),
                                                                              BEHAVIOR_ID(ReactToTouchPetting),
                                                                              BEHAVIOR_ID(TakeAPhotoCoordinator),
                                                                              BEHAVIOR_ID(ShowWallTime),
                                                                              BEHAVIOR_ID(SingletonCancelTimer),
                                                                              BEHAVIOR_ID(SingletonTimerAlreadySet),
                                                                              BEHAVIOR_ID(SingletonTimerAntic),
                                                                              BEHAVIOR_ID(SingletonTimerCheckTime),
                                                                              BEHAVIOR_ID(SingletonTimerSet)  }};

const BehaviorID kWhileInAirDispatcher = BEHAVIOR_ID(WhileInAirDispatcher);

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
  {
    std::set<BehaviorID> inAirSet;
    inAirSet.insert(kWhileInAirDispatcher);
    _inAirDispatcherBehaviorSet = std::make_unique<AreBehaviorsActivatedHelper>(BC, inAirSet);
  }
  {
    _suppressInAirBehaviorSet = std::make_unique<AreBehaviorsActivatedHelper>(BC, kBehaviorStatesToSuppressInAirReaction);
  }
  {
    _lockTracksBehaviorSet = std::make_unique<AreBehaviorsActivatedHelper>(BC, kBehaviorsToLockTreadsWhileInAir);
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
                  but modifier says it can't run while in the air", BehaviorIDToString(id));
      
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
  if((offTreadsState == OffTreadsState::InAir) && !IsInAirDispatcherActive()){
    SuppressInAirReactionIfAppropriate();
    LockTracksIfAppropriate();
  }else{
    if(_areTreadsLocked){
      GetBEI().GetAnimationComponent().UnlockTracks(static_cast<u8>(AnimTrackFlag::BODY_TRACK));
      _areTreadsLocked = false;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCoordinateWhileInAir::IsInAirDispatcherActive() const
{
  return _inAirDispatcherBehaviorSet->AreBehaviorsActivated();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileInAir::LockTracksIfAppropriate()
{
  if(!_areTreadsLocked){
    const bool shouldLock = _lockTracksBehaviorSet->AreBehaviorsActivated();

    if(shouldLock){
      GetBEI().GetAnimationComponent().LockTracks(static_cast<u8>(AnimTrackFlag::BODY_TRACK));
      _areTreadsLocked = true;
    }
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

}
}
