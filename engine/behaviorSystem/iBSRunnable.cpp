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

#include "engine/behaviorSystem/iBSRunnable.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const int kBSTickInterval = 1;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBSRunnable::IBSRunnable(const std::string& idString)
: _idString(idString)
, _currentActivationState(ActivationState::OutOfScope)
, _lastTickWantsToRunCheckedOn(0)
, _lastTickOfUpdate(0)
{
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::EnteredActivatableScope()
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
  EnteredActivatableScopeInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus IBSRunnable::Update(Robot& robot)
{
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
  
  return UpdateInternal(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBSRunnable::WantsToBeActivated()
{
  DEV_ASSERT_MSG(_currentActivationState == ActivationState::InScope,
                 "IBSRunnable.OnActivated.WrongActivationState",
                 "Attempted to activate %s while it was in state %s",
                 _idString.c_str(),
                 ActivationStateToString(_currentActivationState).c_str());
  _lastTickWantsToRunCheckedOn = BaseStationTimer::getInstance()->GetTickCount();

  return WantsToBeActivatedInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::OnActivated()
{
  DEV_ASSERT_MSG(_currentActivationState == ActivationState::InScope,
                 "IBSRunnable.OnActivated.WrongActivationState",
                 "Attempted to activate %s while it was in state %s",
                 _idString.c_str(),
                 ActivationStateToString(_currentActivationState).c_str());
  
  const size_t tickCount = BaseStationTimer::getInstance()->GetTickCount();
  DEV_ASSERT_MSG(tickCount == _lastTickWantsToRunCheckedOn,
                 "IBSRunnable.OnActivated.WantsToRunNotCheckedThisTick",
                 "Attempted to activate %s on tick %zu, but wants to run was last checked on %zu",
                 _idString.c_str(),
                 tickCount,
                 _lastTickWantsToRunCheckedOn);
  
  _currentActivationState = ActivationState::Activated;
  OnActivatedInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::OnDeactivated()
{
  DEV_ASSERT_MSG(_currentActivationState == ActivationState::Activated,
                 "IBSRunnable.OnDeactivated.WrongActivationState",
                 "Attempted to deactivate %s while it was in state %s",
                 _idString.c_str(),
                 ActivationStateToString(_currentActivationState).c_str());
  _currentActivationState = ActivationState::InScope;
  OnDeactivatedInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBSRunnable::LeftActivatableScope()
{
  DEV_ASSERT_MSG(_currentActivationState == ActivationState::InScope,
                 "IBSRunnable.LeftActivatableScope.WrongActivationState",
                 "Attempted to leave activation scope %s while it was in state %s",
                 _idString.c_str(),
                 ActivationStateToString(_currentActivationState).c_str());
  _currentActivationState = ActivationState::OutOfScope;
  LeftActivatableScopeInternal();
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string IBSRunnable::ActivationStateToString(ActivationState state)
{
  switch(state){
    case ActivationState::OutOfScope: return "OutOfScope";
    case ActivationState::Activated : return "Activated";
    case ActivationState::InScope   : return "InScope";
  }
}

  
  
} // namespace Cozmo
} // namespace Anki
