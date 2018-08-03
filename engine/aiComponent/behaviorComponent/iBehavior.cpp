/**
* File: IBehavior.cpp
*
* Author: Kevin M. Karol
* Created: 08/251/17
*
* Description: Interface for "Behavior" elements of the behavior system
* such as activities and behaviors
*
* Copyright: Anki, Inc. 2017
*
**/

#include "coretech/common/engine/utils/timer.h"

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
IBehavior::IBehavior(const std::string& debugLabel)
: _debugLabel(debugLabel)
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
  AssertActivationState_DevOnly(ActivationState::NotInitialized, "IBehavior.Init");
  SetActivationState_DevOnly(ActivationState::OutOfScope, "IBehavior.Init");
  
  _beiWrapper = std::make_unique<BEIWrapper>(behaviorExternalInterface);
  InitInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::OnEnteredActivatableScope()
{
  AssertNotActivationState_DevOnly(ActivationState::NotInitialized, "IBehavior.OnEnteredActivatableScope");
  
  _currentInScopeCount++;
  // If this isn't the first EnteredActivatableScope don't call internal functions
  if(_currentInScopeCount != 1){
    PRINT_CH_INFO("Behaviors",
                  "IBehavior.OnEnteredActivatableScope.AlreadyInScope",
                  "Behavior '%s' is already in scope, ignoring request to enter scope",
                  _debugLabel.c_str());
    return;
  }

  SetActivationState_DevOnly(ActivationState::InScope, "IBehavior.OnEnteredActivatableScope");

  // Update should be called immediately after entering activatable scope
  // so set the last tick count as being one tickInterval before the current tickCount
  _lastTickOfUpdate = (BaseStationTimer::getInstance()->GetTickCount() - kBSTickInterval);
  OnEnteredActivatableScopeInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::Update()
{
  AssertNotActivationState_DevOnly(ActivationState::NotInitialized, "IBehavior.Update");
  AssertNotActivationState_DevOnly(ActivationState::OutOfScope, "IBehavior.Update");

  // Ensure update is ticked every tick while in activatable scope
  const size_t tickCount = BaseStationTimer::getInstance()->GetTickCount();
  DEV_ASSERT_MSG(_lastTickOfUpdate == (tickCount - kBSTickInterval),
                "IBehavior.Update.TickCountMismatch",
                "Behavior '%s' is receiving tick on %zu, but hasn't been ticked since %zu",
                _debugLabel.c_str(),
                tickCount,
                _lastTickOfUpdate);
  _lastTickOfUpdate = tickCount;
  
  UpdateInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::WantsToBeActivated() const
{
#if ANKI_DEV_CHEATS
  // It's possible that this behavior appears in multiple places in the tree, so it may be active when checking
  // WantsToBeActivated.
  DEV_ASSERT((_currentActivationState == ActivationState::Activated) ||
             (_currentActivationState == ActivationState::InScope),
             "IBehavior.WantsToBeActivated.InalidActivationState");
#endif
  _lastTickWantsToBeActivatedCheckedOn = BaseStationTimer::getInstance()->GetTickCount();
  auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
  return WantsToBeActivatedInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::OnActivated()
{
  AssertActivationState_DevOnly(ActivationState::InScope, "IBehavior.OnActivated");

  const size_t tickCount = BaseStationTimer::getInstance()->GetTickCount();
  DEV_ASSERT_MSG(tickCount == _lastTickWantsToBeActivatedCheckedOn,
                  "IBehavior.OnActivated.WantsToRunNotCheckedThisTick",
                  "Attempted to activate %s on tick %zu, but wants to run was last checked on %zu",
                  _debugLabel.c_str(),
                  tickCount,
                  _lastTickWantsToBeActivatedCheckedOn);
  
  SetActivationState_DevOnly(ActivationState::Activated, "IBehavior.OnActivated");
  OnActivatedInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::OnDeactivated()
{
  AssertActivationState_DevOnly(ActivationState::Activated, "IBehavior.OnDeactivated");
  
  SetActivationState_DevOnly(ActivationState::InScope, "IBehavior.OnDeactivated");
  OnDeactivatedInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::OnLeftActivatableScope()
{
  AssertActivationState_DevOnly(ActivationState::InScope, "IBehavior.OnLeftActivatableScope");
  
  if(!ANKI_VERIFY(_currentInScopeCount != 0,
                  "", "")){
    return;
  }
  _currentInScopeCount--;

  if(_currentInScopeCount != 0){
    PRINT_CH_INFO("Behaviors",
                  "IBehavior.OnLeftActivatableScope.StillInScope",
                  "There's still an in scope count of %d on %s",
                  _currentInScopeCount,
                  _debugLabel.c_str());
    return;
  }
  
  
  SetActivationState_DevOnly(ActivationState::OutOfScope, "IBehavior.OnLeftActivatableScope");
  OnLeftActivatableScopeInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SetActivationState_DevOnly(ActivationState state, const std::string& debugStr)
{
  PRINT_CH_DEBUG("Behaviors",
                 "IBehavior.SetActivationState",
                 "%s: Behavior '%s' Activation state set to %s",
                 debugStr.c_str(),
                 _debugLabel.c_str(),
                 ActivationStateToString(state).c_str());
  
  #if ANKI_DEV_CHEATS
    _currentActivationState = state;
  #endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::AssertActivationState_DevOnly(ActivationState state, const std::string& debugStr) const
{
  #if ANKI_DEV_CHEATS
  DEV_ASSERT_MSG(_currentActivationState == state,
                 "IBehavior.AssertActivationState_DevOnly.WrongActivationState",
                 "%s: Behavior '%s' is state %s, but should be in %s",
                 debugStr.c_str(),
                 _debugLabel.c_str(),
                 ActivationStateToString(_currentActivationState).c_str(),
                 ActivationStateToString(state).c_str());
  #endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::AssertNotActivationState_DevOnly(ActivationState state, const std::string& debugStr) const
{
  #if ANKI_DEV_CHEATS
  DEV_ASSERT_MSG(_currentActivationState != state,
                 "IBehavior.AssertNotActivationState_DevOnly.WrongActivationState",
                 "%s: Behavior '%s' is state %s, but should not be",
                 debugStr.c_str(),
                 _debugLabel.c_str(),
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
