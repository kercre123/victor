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

#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"

#include "engine/actions/actionContainers.h"
#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/iBSRunnable.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/viz/vizManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "clad/types/behaviorSystem/reactionTriggers.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/boundedWhile.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

// Forward declaration
class IReactionTriggerStrategy;


namespace{
const int kArbitrarilyLargeCancelBound = 1000000;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::RunnableStack::InitRunnableStack(BehaviorExternalInterface& behaviorExternalInterface,
                                                             IBSRunnable* baseOfStack)
{
  ANKI_VERIFY(_runnableStack.empty(),
              "BehaviorSystemManager.RunnableStack.InitRunnableStack.StackNotEmptyOnInit",
              "");
  
  baseOfStack->Init(behaviorExternalInterface);
  baseOfStack->OnEnteredActivatableScope();
  ANKI_VERIFY(baseOfStack->WantsToBeActivated(behaviorExternalInterface),
              "BehaviorSystemManager.RunnableStack.InitConfig.BaseRunnableDoesn'tWantToRun",
              "");
  PushOntoStack(baseOfStack);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::RunnableStack::UpdateRunnableStack(BehaviorExternalInterface& behaviorExternalInterface,
                                                               std::set<IBSRunnable*>& tickedInStack)
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
    _runnableStack.at(idx)->Update(behaviorExternalInterface);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::RunnableStack::PushOntoStack(IBSRunnable* runnable)
{
  _runnableToIndexMap.insert(std::make_pair(runnable, _runnableStack.size()));
  _runnableStack.push_back(runnable);
  
  PrepareDelegatesToEnterScope(runnable);
  runnable->OnActivated(*_behaviorExternalInterface);
  

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::RunnableStack::PopStack()
{
  for(auto& entry: _delegatesMap[_runnableStack.back()]){
    PrepareDelegateForRemovalFromStack(entry);
  }
  _runnableStack.back()->OnDeactivated(*_behaviorExternalInterface);
  
  _delegatesMap.erase(_runnableStack.back());
  _runnableToIndexMap.erase(_runnableStack.back());
  _runnableStack.pop_back();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::RunnableStack::PrepareDelegatesToEnterScope(IBSRunnable* delegated)
{
  // Add the new available delegates to the map
  std::set<IBSRunnable*> newAvailableDelegates;
  delegated->GetAllDelegates(newAvailableDelegates);
  for(auto& entry: newAvailableDelegates){
    entry->OnEnteredActivatableScope();
  }
  
  _delegatesMap.insert(std::make_pair(delegated, std::move(newAvailableDelegates)));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::RunnableStack::PrepareDelegateForRemovalFromStack(IBSRunnable* delegated)
{
  auto availableDelegates = _delegatesMap.find(delegated);
  for(auto& entry: availableDelegates->second){
    entry->OnLeftActivatableScope();
  }
  
  _delegatesMap.erase(availableDelegates);
}


/////////
// BehaviorSystemManager implementation
/////////


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSystemManager::BehaviorSystemManager()
: _initializationStage(InitializationStage::SystemNotInitialized)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorSystemManager::InitConfiguration(BehaviorExternalInterface& behaviorExternalInterface,
                                                IBSRunnable* baseRunnable)
{
  // do not support multiple initialization. A) we don't need it, B) it's easy to forget to clean up everything properly
  // when adding new stuff. During my refactoring I found several variables that were not properly reset, so
  // potentially double Init was never supported
  DEV_ASSERT(_initializationStage == InitializationStage::SystemNotInitialized &&
             baseRunnable != nullptr,
             "BehaviorSystemManager.InitConfiguration.AlreadyInitialized");
  _initializationStage = InitializationStage::StackNotInitialized;


  // Assumes there's only one instance of the behavior external Intarfec
  _behaviorExternalInterface = &behaviorExternalInterface;
  _runnableStack.reset(new RunnableStack(_behaviorExternalInterface));
  
  _baseRunnableTmp = baseRunnable;
  
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::Update(BehaviorExternalInterface& behaviorExternalInterface)
{
  ANKI_CPU_PROFILE("BehaviorSystemManager::Update");
  
  if(_initializationStage == InitializationStage::SystemNotInitialized) {
    PRINT_NAMED_ERROR("BehaviorSystemManager.Update.NotInitialized", "");
    return;
  }
  
  // There's a delay between init and first robot update tick - this messes with
  // time checks in iBSRunnable, so Activate the base here instead of in init
  if(_initializationStage == InitializationStage::StackNotInitialized){
    _initializationStage = InitializationStage::Initialized;
    _runnableStack->InitRunnableStack(behaviorExternalInterface, _baseRunnableTmp);
    _baseRunnableTmp = nullptr;
  }
  
  std::set<IBSRunnable*> runnableUpdatesTickedInStack;
  // First update the runnable stack and allow it to make any delegation/canceling
  // decisions that it needs to make
  _runnableStack->UpdateRunnableStack(behaviorExternalInterface, runnableUpdatesTickedInStack);
  // Then once all of that's done, update anything that's in activatable scope
  // but isn't currently on the runnable stack
  UpdateInActivatableScope(behaviorExternalInterface, runnableUpdatesTickedInStack);
  
} // Update()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::UpdateInActivatableScope(BehaviorExternalInterface& behaviorExternalInterface, const std::set<IBSRunnable*>& tickedInStack)
{
  // This is innefficient and should be replaced, but not overengineering right now
  std::set<IBSRunnable*> allInActivatableScope;
  const RunnableStack::DelegatesMap& delegatesMap = _runnableStack->GetDelegatesMap();
  for(auto& entry: delegatesMap){
    for(auto& runnable : entry.second){
      if(tickedInStack.find(runnable)  == tickedInStack.end()){
          allInActivatableScope.insert(runnable);
      }
    }
  }
  for(auto& entry: allInActivatableScope){
    entry->Update(behaviorExternalInterface);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSystemManager::IsControlDelegated(const IBSRunnable* delegator)
{
  return (_runnableStack->IsInStack(delegator)) &&
         (_runnableStack->GetTopOfStack() != delegator);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSystemManager::CanDelegate(IBSRunnable* delegator)
{
  return _runnableStack->GetTopOfStack() == delegator;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSystemManager::Delegate(IBSRunnable* delegator, IBSRunnable* delegated)
{
  // Ensure that the delegator is on top of the stack
  if(!ANKI_VERIFY(delegator == _runnableStack->GetTopOfStack(),
                  "BehaviorSystemManager.Delegate.DelegatorNotOnTopOfStack",
                  "")){
    return false;
  }
  
  {
    // Ensure that the delegated runnable is in the delegates map
    const RunnableStack::DelegatesMap& delegatesMap =  _runnableStack->GetDelegatesMap();
    auto iter = delegatesMap.find(delegator);
    if(!ANKI_VERIFY((iter != delegatesMap.end()) &&
                    (iter->second.find(delegated) != iter->second.end()),
                   "BehaviorSystemManager.Delegate.DelegateNotInAvailableDelegateMap",
                   "Delegator %s asked to delegate to %s which is not in available delegates map",
                   delegator->GetPrintableID().c_str(),
                   delegated->GetPrintableID().c_str())){
      return false;
    }
  }
  
  // Activate the new runnable and add it to the top of the stack
  _runnableStack->PushOntoStack(delegated);

  PRINT_CH_INFO("BehaviorSystem", "BehaviorSystemManager.Delegate.ToBSRunnable",
                "'%s' delegated to '%s'",
                delegator->GetPrintableID().c_str(),
                delegated->GetPrintableID().c_str());
  
  _runnableStack->DebugPrintStack("AfterDelegation");
  
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::CancelDelegates(IBSRunnable* delegator)
{
  if(_runnableStack->IsInStack(delegator)){
    BOUNDED_WHILE(kArbitrarilyLargeCancelBound,
                  _runnableStack->GetTopOfStack() != delegator){
      _runnableStack->PopStack();
    }
  }

  PRINT_CH_INFO("BehaviorSystem", "BehaviorSystemManager.CancelDelegates",
                "'%s' canceled it's delegates",
                delegator->GetPrintableID().c_str());

  _runnableStack->DebugPrintStack("AfterCancelDelgates");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO:(bn) kevink: consider rename to "stop" rather than cancel
void BehaviorSystemManager::CancelSelf(IBSRunnable* delegator)
{
  CancelDelegates(delegator);
  
  if(ANKI_VERIFY(!IsControlDelegated(delegator),
                 "BehaviorSystemManager.CancelSelf.ControlStillDelegated",
                 "CancelDelegates was called, but the delegator is not on the top of the stack")){
    _runnableStack->PopStack();
  }

  PRINT_CH_INFO("BehaviorSystem", "BehaviorSystemManager.CancelSelf",
                "'%s' canceled itself",
                delegator->GetPrintableID().c_str());

  _runnableStack->DebugPrintStack("AfterCancelSelf");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorSystemManager::FindBehaviorByID(BehaviorID behaviorID) const
{
  if(_behaviorExternalInterface != nullptr){
    return _behaviorExternalInterface->GetBehaviorContainer().FindBehaviorByID(behaviorID);
  }else{
    IBehaviorPtr empty;
    return empty;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorSystemManager::FindBehaviorByExecutableType(ExecutableBehaviorType type) const
{
  if(_behaviorExternalInterface != nullptr){
    return _behaviorExternalInterface->GetBehaviorContainer().FindBehaviorByExecutableType(type);
  }else{
    IBehaviorPtr empty;
    return empty;
  }
}



} // namespace Cozmo
} // namespace Anki
