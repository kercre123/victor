/**
 * File: behaviorResetState.cpp
 *
 * Author: ross
 * Created: 2018-08-12
 *
 * Description: Tries to reset things that may have caused behavior cycles
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/behaviorResetState.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/pathComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Vector {

namespace {
  const float kTimeWaitWhenOnTreads_s = 0.5f;
  const float kTimeWaitWhenOffTreads_s = 3.0f;
  
  const BehaviorID kBackupBehaviorID = BEHAVIOR_ID(Wait);
  
  #define LOG_CHANNEL "Behaviors"
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorResetState::BehaviorResetState(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorResetState::WantsToBeActivatedBehavior() const
{
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::SetFollowupBehaviorID( BehaviorID behaviorID )
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _nextBehavior = BC.FindBehaviorByID( behaviorID );
  if( !ANKI_VERIFY( _nextBehavior != nullptr,
                    "BehaviorResetState.SetFollowupBehaviorID.Invalid",
                    "Could not find behavior '%s', using '%s'",
                    BehaviorTypesWrapper::BehaviorIDToString(behaviorID),
                    BehaviorTypesWrapper::BehaviorIDToString(kBackupBehaviorID) ) )
  {
    _nextBehavior = BC.FindBehaviorByID( kBackupBehaviorID );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if( _nextBehavior != nullptr ) {
    delegates.insert( _nextBehavior.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::OnBehaviorActivated()
{
  _currBehavior = _nextBehavior;
  _nextBehavior = nullptr;
  
  // freeze all motor functions
  GetBEI().GetRobotInfo().GetMoveComponent().StopAllMotors();
  
  // stop any planning or path following
  auto& pathComponent = GetBEI().GetRobotInfo().GetPathComponent();
  pathComponent.Abort();
  
  // delocalize robot. this also resets blockworld, unexpected movement, and the navmap
  ExternalInterface::ForceDelocalizeRobot delocMsg;
  GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast( ExternalInterface::MessageGameToEngine{std::move(delocMsg)} );
  
  // don't do anything for a short while. If the robot thinks it's off treads, wait even longer,
  // in case it thrashing was causing it to not realize it's on treads
  const bool offTreads = (GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads);
  float timeToWait_s = offTreads ? kTimeWaitWhenOffTreads_s : kTimeWaitWhenOnTreads_s;
  auto* waitAction = new WaitAction{ timeToWait_s };
  DelegateIfInControl( waitAction, &BehaviorResetState::PutDownCubeIfNeeded );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::PutDownCubeIfNeeded()
{
  // put down cube
  const bool carrying = GetBEI().GetRobotInfo().IsCarryingObject();
  if( carrying ) {
    auto* putDownCube = new PlaceObjectOnGroundAction();
    DelegateIfInControl( putDownCube, [this](const ActionResult& res) {
      if( res == ActionResult::SUCCESS ) {
        ResetComponentsAndDelegate();
      } else {
        // try just putting the lift down
        auto* moveLiftDown = new MoveLiftToHeightAction{ MoveLiftToHeightAction::Preset::LOW_DOCK };
        DelegateIfInControl( moveLiftDown, &BehaviorResetState::ResetComponentsAndDelegate );
      }
    });
  } else {
    ResetComponentsAndDelegate();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::ResetComponentsAndDelegate()
{
  // ForceDelocalizeRobot already handled blockworld, unexpected movement, and the navmap because of
  // resulting calls to OnRobotDelocalized(). Now take care of other components:
  
  // In case the last known face was causing the robot to turn towards something...? Might not be necessary
  GetBEI().GetFaceWorldMutable().ClearAllFaces();
  
  if( _currBehavior != nullptr ) {
    ANKI_VERIFY( _currBehavior->WantsToBeActivated(),
                 "BehaviorResetState.ResetComponentsAndDelegate.DWTBA",
                 "Behavior '%s' doesnt want to activate as the base behavior",
                 BehaviorTypesWrapper::BehaviorIDToString( _currBehavior->GetID() ) );
    
    DelegateIfInControl( _currBehavior.get() );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::BehaviorUpdate()
{
  // just in case the delegate ends, start a new one.
  if( IsActivated() && !IsControlDelegated() && (_currBehavior != nullptr) ) {
    LOG_WARNING("BehaviorResetState.BehaviorUpdate.Ended", "Delegate should not end");
    
    ANKI_VERIFY( _currBehavior->WantsToBeActivated(),
                 "BehaviorResetState.BehaviorUpdate.DWTBA",
                 "Behavior '%s' doesnt want to activate as the base behavior",
                 BehaviorTypesWrapper::BehaviorIDToString( _currBehavior->GetID() ) );
    
    DelegateIfInControl( _currBehavior.get() );
  }
}

}
}
