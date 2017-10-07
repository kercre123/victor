/**
* File: IBehavior.cpp
*
* Author: Kevin M. Karol
* Created: 08/251/17
*
* Description: Interface for "Runnable" elements of the behavior system
* such as activities and behaviors
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/common/basestation/utils/timer.h"

#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const int kBSTickInterval = 1;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::IBehavior(const std::string& idString)
: _idString(idString)
, _currentInScopeCount(0)
, _lastTickWantsToBeActivatedCheckedOn(0)
, _lastTickOfUpdate(0)
#if ANKI_DEV_CHEATS
, _currentActivationState(ActivationState::NotInitialized)
#endif
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::Init(BehaviorExternalInterface& behaviorExternalInterface)
{
  AssertActivationState_DevOnly(ActivationState::NotInitialized);
  SetActivationState_DevOnly(ActivationState::OutOfScope);
  
  InitInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::OnEnteredActivatableScope()
{
  AssertNotActivationState_DevOnly(ActivationState::NotInitialized);
  
  _currentInScopeCount++;
  // If this isn't the first EnteredActivatableScope don't call internal functions
  if(_currentInScopeCount != 1){
    PRINT_CH_INFO("Behaviors",
                  "IBehavior.OnEnteredActivatableScope.AlreadyInScope",
                  "Runnable %s is already in scope, ignoring request to enter scope",
                  _idString.c_str());
    return;
  }

  SetActivationState_DevOnly(ActivationState::InScope);

  // Update should be called immediately after entering activatable scope
  // so set the last tick count as being one tickInterval before the current tickCount
  _lastTickOfUpdate = (BaseStationTimer::getInstance()->GetTickCount() - kBSTickInterval);
  OnEnteredActivatableScopeInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::Update(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(USE_BSM){
    AssertNotActivationState_DevOnly(ActivationState::NotInitialized);
    AssertNotActivationState_DevOnly(ActivationState::OutOfScope);
  
    // Ensure update is ticked every tick while in activatable scope
    const size_t tickCount = BaseStationTimer::getInstance()->GetTickCount();
    DEV_ASSERT_MSG(_lastTickOfUpdate == (tickCount - kBSTickInterval),
                   "IBehavior.Update.TickCountMismatch",
                   "BSRunnable %s is receiving tick on %zu, but hasn't been ticked since %zu",
                   _idString.c_str(),
                   _lastTickOfUpdate,
                   tickCount);
    _lastTickOfUpdate = tickCount;
  }
  
  UpdateInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::WantsToBeActivated(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if(USE_BSM){
    AssertActivationState_DevOnly(ActivationState::InScope);
    _lastTickWantsToBeActivatedCheckedOn = BaseStationTimer::getInstance()->GetTickCount();
  }
  return WantsToBeActivatedInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::OnActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(USE_BSM){
    AssertActivationState_DevOnly(ActivationState::InScope);

    const size_t tickCount = BaseStationTimer::getInstance()->GetTickCount();
    DEV_ASSERT_MSG(tickCount == _lastTickWantsToBeActivatedCheckedOn,
                   "IBehavior.OnActivated.WantsToRunNotCheckedThisTick",
                   "Attempted to activate %s on tick %zu, but wants to run was last checked on %zu",
                   _idString.c_str(),
                   tickCount,
                   _lastTickWantsToBeActivatedCheckedOn);
  }
  
  SetActivationState_DevOnly(ActivationState::Activated);
  OnActivatedInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::OnDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(USE_BSM){
    AssertActivationState_DevOnly(ActivationState::Activated);
  }
  
  SetActivationState_DevOnly(ActivationState::InScope);
  OnDeactivatedInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::OnLeftActivatableScope()
{
  AssertActivationState_DevOnly(ActivationState::InScope);
  
  if(ANKI_VERIFY(_currentInScopeCount != 0,
                 "", "")){
    return;
  }
  
  _currentInScopeCount--;
  if(_currentInScopeCount != 0){
    PRINT_CH_INFO("Behaviors",
                  "IBehavior.OnLeftActivatableScope.StillInScope",
                  "There's still an in scope count of %d on %s",
                  _currentInScopeCount,
                  _idString.c_str());
    return;
  }
  
  
  SetActivationState_DevOnly(ActivationState::OutOfScope);
  OnLeftActivatableScopeInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SetActivationState_DevOnly(ActivationState state)
{
  PRINT_CH_INFO("Behaviors",
                "IBehavior.SetActivationState",
                "Activation state set to %s",
                ActivationStateToString(state).c_str());
  
  #if ANKI_DEV_CHEATS
    _currentActivationState = state;
  #endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::AssertActivationState_DevOnly(ActivationState state) const
{
  #if ANKI_DEV_CHEATS
  DEV_ASSERT_MSG(_currentActivationState == state,
                 "IBehavior.AssertActivationState_DevOnly.WrongActivationState",
                 "Runnable %s is not in state %s which it should be",
                 _idString.c_str(),
                 ActivationStateToString(_currentActivationState).c_str());
  #endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::AssertNotActivationState_DevOnly(ActivationState state) const
{
  #if ANKI_DEV_CHEATS
  DEV_ASSERT_MSG(_currentActivationState != state,
                 "IBehavior.AssertNotActivationState_DevOnly.WrongActivationState",
                 "Runnable %s is in state %s when it shouldn't be",
                 _idString.c_str(),
                 ActivationStateToString(_currentActivationState).c_str());
  #endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string IBehavior::ActivationStateToString(ActivationState state) const
{
  switch(state){
    case ActivationState::NotInitialized : return "NotInitialized";
    case ActivationState::OutOfScope     : return "OutOfScope";
    case ActivationState::Activated      : return "Activated";
    case ActivationState::InScope        : return "InScope";
  }
}
  
} // namespace Cozmo
} // namespace Anki
