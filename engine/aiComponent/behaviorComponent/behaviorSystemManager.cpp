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
#include "engine/aiComponent/behaviorComponent/activities/activities/activityFactory.h"
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
BehaviorSystemManager::~BehaviorSystemManager()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorSystemManager::InitConfiguration(BehaviorExternalInterface& behaviorExternalInterface,
                                                const Json::Value& behaviorSystemConfig)
{
  // do not support multiple initialization. A) we don't need it, B) it's easy to forget to clean up everything properly
  // when adding new stuff. During my refactoring I found several variables that were not properly reset, so
  // potentially double Init was never supported
  DEV_ASSERT(_initializationStage == InitializationStage::SystemNotInitialized,
             "BehaviorSystemManager.InitConfiguration.AlreadyInitialized");
  _initializationStage = InitializationStage::StackNotInitialized;


  // Assumes there's only one instance of the behavior external Intarfec
  _behaviorExternalInterface = &behaviorExternalInterface;
  
  if(!behaviorSystemConfig.empty()){
    ActivityType type = IActivity::ExtractActivityTypeFromConfig(behaviorSystemConfig);
    
    IBSRunnable* baseRunnable = ActivityFactory::CreateActivity(behaviorExternalInterface,
                                                                type,
                                                                behaviorSystemConfig);
    baseRunnable->Init(behaviorExternalInterface);
    PushOntoStack(baseRunnable);
  }
  
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
  
  
  
  std::set<IBSRunnable*> runnableUpdatesTickedInStack;
  // First update the runnable stack and allow it to make any delegation/canceling
  // decisions that it needs to make
  UpdateRunnableStack(behaviorExternalInterface, runnableUpdatesTickedInStack);
  // Then once all of that's done, update anything that's in activatable scope
  // but isn't currently on the runnable stack
  UpdateInActivatableScope(behaviorExternalInterface, runnableUpdatesTickedInStack);
  
} // Update()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::UpdateRunnableStack(BehaviorExternalInterface& behaviorExternalInterface, std::set<IBSRunnable*>& tickedInStack)
{
  if(_runnableStack.size() == 0){
    PRINT_NAMED_WARNING("BehaviorSystemManager.UpdateRunnableStack.NoStackInitialized",
                        "");
    return;
  }
  
  // There's a delay between init and first robot update tick - this messes with
  // time checks in iBSRunnable, so Activate the base here instead of in init
  if(_initializationStage == InitializationStage::StackNotInitialized){
    _initializationStage = InitializationStage::Initialized;
    
    auto base = _runnableStack.begin();
    (*base)->OnEnteredActivatableScope();
    ANKI_VERIFY((*base)->WantsToBeActivated(behaviorExternalInterface),
                "BehaviorSystemManager.InitConfig.BaseRunnableDoesn'tWantToRun",
                "");
    (*base)->OnActivated(behaviorExternalInterface);
    PrepareDelegatesToEnterScope(*base);
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
void BehaviorSystemManager::UpdateInActivatableScope(BehaviorExternalInterface& behaviorExternalInterface, const std::set<IBSRunnable*>& tickedInStack)
{
  // This is innefficient and should be replaced, but not overengineering right now
  std::set<IBSRunnable*> allInActivatableScope;
  for(auto& entry: _delegatesMap){
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
  // If it's not on top of the stack, control is delegated
  auto res = _runnableToIndexMap.find(delegator);
  if(res != _runnableToIndexMap.end()){
    return ((res->second - 1) == _runnableStack.size());
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSystemManager::CanDelegate(IBSRunnable* delegator)
{
  return _runnableStack.back() == delegator;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSystemManager::Delegate(IBSRunnable* delegator, IBSRunnable* delegated)
{
  // Ensure that the delegator is on top of the stack
  if(!ANKI_VERIFY(delegator == _runnableStack.back(),
                  "BehaviorSystemManager.Delegate.DelegatorNotOnTopOfStack",
                  "")){
    return false;
  }
  
  {
    // Ensure that the delegated runnable is in the delegates map
    auto& availableDelegates = _delegatesMap[delegator];
    if(!ANKI_VERIFY(availableDelegates.find(delegated) != availableDelegates.end(),
                   "BehaviorSystemManager.Delegate.DelegateNotInAvailableDelegateMap",
                   "Delegator %s asked to delegate to %s which is not in available delegates map",
                   delegator->GetPrintableID().c_str(),
                   delegated->GetPrintableID().c_str())){
      return false;
    }
  }
  
  PrepareDelegatesToEnterScope(delegated);
  
  // Activate the new runnable and add it to the top of the stack
  PushOntoStack(delegated);
  delegated->OnActivated(*_behaviorExternalInterface);
  
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::CancelDelegates(IBSRunnable* delegator)
{
  auto iter = _runnableStack.begin();
  while((*iter != delegator) &&
        (iter != _runnableStack.end())){
    ++iter;
  }
  
  DEV_ASSERT(iter != _runnableStack.end(),
             "BehaviorSystemManager.CancelSelf.DelegatorNotInStack");
  auto revIter = _runnableStack.rbegin();
  while((*revIter != *iter) &&
        (revIter != _runnableStack.rend())){
    PrepareDelegateForRemovalFromStack(*revIter);
    ++revIter;
  }
  
  DEV_ASSERT(revIter != _runnableStack.rend(),
             "BehaviorSystemManager.CancelSelf.IteratorsNeverMet");
  
  // Clear off the stack on top of the delegator
  iter++;
  _runnableStack.erase(iter, _runnableStack.end());
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::CancelSelf(IBSRunnable* delegator)
{
  CancelDelegates(delegator);
  
  if(ANKI_VERIFY(!IsControlDelegated(delegator),
                 "BehaviorSystemManager.CancelSelf.ControlStillDelegated",
                 "CancelDelegates was called, but the delegator is not on the top of the stack")){
    PrepareDelegateForRemovalFromStack(delegator);
    PopStack();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::PushOntoStack(IBSRunnable* runnable)
{
  _runnableToIndexMap.insert(std::make_pair(runnable, _runnableStack.size()));
  _runnableStack.push_back(runnable);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::PopStack()
{
  _runnableToIndexMap.erase(_runnableStack.back());
  _runnableStack.pop_back();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::PrepareDelegatesToEnterScope(IBSRunnable* delegated)
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
void BehaviorSystemManager::PrepareDelegateForRemovalFromStack(IBSRunnable* delegated)
{
  delegated->OnDeactivated(*_behaviorExternalInterface);
  auto availableDelegates = _delegatesMap.find(delegated);
  for(auto& entry: availableDelegates->second){
    entry->OnLeftActivatableScope();
  }
  
  _delegatesMap.erase(availableDelegates);
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
