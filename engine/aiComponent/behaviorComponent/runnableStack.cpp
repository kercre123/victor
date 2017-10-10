/**
* File: behaviorSystemManager.cpp
*
* Author: Kevin Karol
* Date:   8/17/2017
*
* Description: Manages and enforces the lifecycle and transitions
* of parts of the behavior system
*
* Copyright: Anki, Inc. 2017
**/

#include "engine/aiComponent/behaviorComponent/runnableStack.h"

#include "engine/aiComponent/behaviorComponent/asyncMessageGateComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/stateChangeComponent.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

namespace{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RunnableStack::InitRunnableStack(BehaviorExternalInterface& behaviorExternalInterface,
                                                             IBehavior* baseOfStack)
{
  ANKI_VERIFY(_runnableStack.empty(),
              "BehaviorSystemManager.RunnableStack.InitRunnableStack.StackNotEmptyOnInit",
              "");
  
  baseOfStack->OnEnteredActivatableScope();
  ANKI_VERIFY(baseOfStack->WantsToBeActivated(behaviorExternalInterface),
              "BehaviorSystemManager.RunnableStack.InitConfig.BaseRunnableDoesn'tWantToRun",
              "");
  PushOntoStack(baseOfStack);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RunnableStack::UpdateRunnableStack(BehaviorExternalInterface& behaviorExternalInterface,
                                        std::vector<ExternalInterface::RobotCompletedAction>& actionsCompletedThisTick,
                                        AsyncMessageGateComponent& asyncMessageGateComp,
                                        std::set<IBehavior*>& tickedInStack)
{
  if(_runnableStack.size() == 0){
    PRINT_NAMED_WARNING("BehaviorSystemManager.RunnableStack.UpdateRunnableStack.NoStackInitialized",
                        "");
    return;
  }

  
  
  // The stack can be altered during update ticks through the cancel delegation
  // functions - so track the index in the stack rather than the iterator directly
  // One side effect of this is that if a BSrunnable ends this tick without queueing
  // an action we potentially lose a tick before the next time a BSRunnable queues
  // an action - to save on complexity we're accepting this tradeoff for the time being
  // but may decide to address it directly here or within the BSRunnable/one of its subclasses
  // in the future
  for(int idx = 0; idx < _runnableStack.size(); idx++){
    tickedInStack.insert(_runnableStack.at(idx));
    
    asyncMessageGateComp.GetEventsForBehavior(
       _runnableStack.at(idx),
       behaviorExternalInterface.GetStateChangeComponent()._gameToEngineEvents);
    asyncMessageGateComp.GetEventsForBehavior(
       _runnableStack.at(idx),
       behaviorExternalInterface.GetStateChangeComponent()._engineToGameEvents);
    asyncMessageGateComp.GetEventsForBehavior(
       _runnableStack.at(idx),
       behaviorExternalInterface.GetStateChangeComponent()._robotToEngineEvents);
    
    // Set the actions completed this tick for the top of the stack
    if(idx == (_runnableStack.size() - 1)){
      behaviorExternalInterface.GetStateChangeComponent()._actionsCompletedThisTick = actionsCompletedThisTick;
    }
    
    _runnableStack.at(idx)->Update(behaviorExternalInterface);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RunnableStack::PushOntoStack(IBehavior* runnable)
{
  _runnableToIndexMap.insert(std::make_pair(runnable, _runnableStack.size()));
  _runnableStack.push_back(runnable);
  
  PrepareDelegatesToEnterScope(runnable);
  runnable->OnActivated(*_behaviorExternalInterface);
  

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RunnableStack::PopStack()
{
  PrepareDelegatesForRemovalFromStack(_runnableStack.back());

  _runnableStack.back()->OnDeactivated(*_behaviorExternalInterface);
  
  _runnableToIndexMap.erase(_runnableStack.back());
  _runnableStack.pop_back();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RunnableStack::PrepareDelegatesToEnterScope(IBehavior* delegated)
{
  // Add the new available delegates to the map
  std::set<IBehavior*> newAvailableDelegates;
  delegated->GetAllDelegates(newAvailableDelegates);
  for(auto& entry: newAvailableDelegates){
    entry->OnEnteredActivatableScope();
  }
  
  _delegatesMap.insert(std::make_pair(delegated, std::move(newAvailableDelegates)));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RunnableStack::PrepareDelegatesForRemovalFromStack(IBehavior* delegated)
{
  auto availableDelegates = _delegatesMap.find(delegated);
  DEV_ASSERT(availableDelegates != _delegatesMap.end(),
             "RunnableStack.PrepareDelegateForRemovalFromStack.DelegateNotFound");
  for(auto& entry: availableDelegates->second){
    entry->OnLeftActivatableScope();
  }
  
  _delegatesMap.erase(delegated);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RunnableStack::DebugPrintStack(const std::string& debugStr) const
{
  for( size_t i=0; i<_runnableStack.size(); ++i) {
    PRINT_CH_INFO("BehaviorSystem", ("BehaviorSystemManager.Stack." + debugStr).c_str(),
                  "%zu: %s",
                  i,
                  _runnableStack[i]->GetPrintableID().c_str());
  }
}

} // namespace Cozmo
} // namespace Anki
