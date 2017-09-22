/**
* File: iBSRunnable.cpp
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
#include "engine/aiComponent/behaviorComponent/iBSRunnable.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const int kBSTickInterval = 1;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBSRunnable::IBSRunnable(const std::string& idString)
: _idString(idString)
, _currentActivationState(ActivationState::NotInitialized)
, _lastTickWantsToBeActivatedCheckedOn(0)
, _lastTickOfUpdate(0)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::Init(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEV_ASSERT_MSG(_currentActivationState == ActivationState::NotInitialized,
                 "IBSRunnable.Init.WrongActivationState",
                 "Attempted to initialize %s while it was in state %s",
                 _idString.c_str(),
                 ActivationStateToString(_currentActivationState).c_str());
  
  InitInternal(behaviorExternalInterface);
  
  _currentActivationState = ActivationState::OutOfScope;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::OnEnteredActivatableScope()
{
  DEV_ASSERT_MSG(_currentActivationState == ActivationState::OutOfScope,
                 "IBSRunnable.EnteredActivatableScope.WrongActivationState",
                 "Attempted to make activatable %s while it was in state %s",
                 _idString.c_str(),
                 ActivationStateToString(_currentActivationState).c_str());

  // Update should be called immediately after entering activatable scope
  // so set the last tick count as being one tickInterval before the current tickCount
  _lastTickOfUpdate = (BaseStationTimer::getInstance()->GetTickCount() - kBSTickInterval);
  _currentActivationState = ActivationState::InScope;
  OnEnteredActivatableScopeInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::Update(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(USE_BSM){
    DEV_ASSERT_MSG(_currentActivationState != ActivationState::OutOfScope,
                   "IBSRunnable.Update.WrongActivationState",
                   "Attempted to activate %s while it was in state %s",
                   _idString.c_str(),
                   ActivationStateToString(_currentActivationState).c_str());
  
    // Ensure update is ticked every tick while in activatable scope
    const size_t tickCount = BaseStationTimer::getInstance()->GetTickCount();
    DEV_ASSERT_MSG(_lastTickOfUpdate == (tickCount - kBSTickInterval),
                   "IBSRunnable.Update.TickCountMismatch",
                   "BSRunnable %s is receiving tick on %zu, but hasn't been ticked since %zu",
                   _idString.c_str(),
                   _lastTickOfUpdate,
                   tickCount);
    _lastTickOfUpdate = tickCount;
  }
  
  UpdateInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBSRunnable::WantsToBeActivated(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if(USE_BSM){
    DEV_ASSERT_MSG(_currentActivationState == ActivationState::InScope,
                   "IBSRunnable.OnActivated.WrongActivationState",
                   "Attempted to activate %s while it was in state %s",
                   _idString.c_str(),
                   ActivationStateToString(_currentActivationState).c_str());
    _lastTickWantsToBeActivatedCheckedOn = BaseStationTimer::getInstance()->GetTickCount();
  }
  return WantsToBeActivatedInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::OnActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(USE_BSM){
    DEV_ASSERT_MSG(_currentActivationState == ActivationState::InScope,
                   "IBSRunnable.OnActivated.WrongActivationState",
                   "Attempted to activate %s while it was in state %s",
                   _idString.c_str(),
                   ActivationStateToString(_currentActivationState).c_str());

    const size_t tickCount = BaseStationTimer::getInstance()->GetTickCount();
    DEV_ASSERT_MSG(tickCount == _lastTickWantsToBeActivatedCheckedOn,
                   "IBSRunnable.OnActivated.WantsToRunNotCheckedThisTick",
                   "Attempted to activate %s on tick %zu, but wants to run was last checked on %zu",
                   _idString.c_str(),
                   tickCount,
                   _lastTickWantsToBeActivatedCheckedOn);
  }
  
  _currentActivationState = ActivationState::Activated;
  OnActivatedInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::OnDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(USE_BSM){
    DEV_ASSERT_MSG(_currentActivationState == ActivationState::Activated,
                   "IBSRunnable.OnDeactivated.WrongActivationState",
                   "Attempted to deactivate %s while it was in state %s",
                   _idString.c_str(),
                   ActivationStateToString(_currentActivationState).c_str());
  }
  _currentActivationState = ActivationState::InScope;
  OnDeactivatedInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::OnLeftActivatableScope()
{
  DEV_ASSERT_MSG(_currentActivationState == ActivationState::InScope,
                 "IBSRunnable.LeftActivatableScope.WrongActivationState",
                 "Attempted to leave activation scope %s while it was in state %s",
                 _idString.c_str(),
                 ActivationStateToString(_currentActivationState).c_str());
  _currentActivationState = ActivationState::OutOfScope;
  OnLeftActivatableScopeInternal();
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string IBSRunnable::ActivationStateToString(ActivationState state) const
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
