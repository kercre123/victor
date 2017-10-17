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
#include "engine/aiComponent/behaviorComponent/asyncMessageGateComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/stateChangeComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/viz/vizManager.h"

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
BehaviorSystemManager::BehaviorSystemManager()
: _initializationStage(InitializationStage::SystemNotInitialized)
{
  _runnableStack.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSystemManager::~BehaviorSystemManager()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorSystemManager::InitConfiguration(IBehavior* baseRunnable,
                                                BehaviorExternalInterface& behaviorExternalInterface,
                                                AsyncMessageGateComponent* asyncMessageComponent)
{
  // do not support multiple initialization. A) we don't need it, B) it's easy to forget to clean up everything properly
  // when adding new stuff. During my refactoring I found several variables that were not properly reset, so
  // potentially double Init was never supported
  DEV_ASSERT(_initializationStage == InitializationStage::SystemNotInitialized &&
             baseRunnable != nullptr,
             "BehaviorSystemManager.InitConfiguration.AlreadyInitialized");
  _initializationStage = InitializationStage::StackNotInitialized;
  _baseRunnableTmp = baseRunnable;

  _asyncMessageComponent = asyncMessageComponent;
  // Assumes there's only one instance of the behavior external Intarfec
  _behaviorExternalInterface = &behaviorExternalInterface;
  _runnableStack.reset(new RunnableStack(_behaviorExternalInterface));
  
  
  Robot& robot = behaviorExternalInterface.GetRobot();
  if(robot.HasExternalInterface()){
    _eventHandles.push_back(robot.GetExternalInterface()->Subscribe(EngineToGameTag::RobotCompletedAction,
                                            [this](const EngineToGameEvent& event) {
                                              DEV_ASSERT(event.GetData().GetTag() == EngineToGameTag::RobotCompletedAction,
                                                         "ICozmoBehavior.RobotCompletedAction.WrongEventTypeFromCallback");
                                              _actionsCompletedThisTick.push_back(event.GetData().Get_RobotCompletedAction());
                                            }));
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
  
  // There's a delay between init and first robot update tick - this messes with
  // time checks in IBehavior, so Activate the base here instead of in init
  if(_initializationStage == InitializationStage::StackNotInitialized){
    _initializationStage = InitializationStage::Initialized;


    IBehavior* baseRunnable = _baseRunnableTmp;
    
    _runnableStack->InitRunnableStack(behaviorExternalInterface, baseRunnable);
    _baseRunnableTmp = nullptr;
  }

  for( const auto& completionMsg : _actionsCompletedThisTick ) {
    behaviorExternalInterface.GetDelegationComponent().HandleActionComplete( completionMsg.idTag );
  }
  
  _asyncMessageComponent->PrepareCache();
  
  std::set<IBehavior*> runnableUpdatesTickedInStack;
  // First update the runnable stack and allow it to make any delegation/canceling
  // decisions that it needs to make
  _runnableStack->UpdateRunnableStack(behaviorExternalInterface,
                                      _actionsCompletedThisTick,
                                      *_asyncMessageComponent,
                                      runnableUpdatesTickedInStack);
  _actionsCompletedThisTick.clear();
  // Then once all of that's done, update anything that's in activatable scope
  // but isn't currently on the runnable stack
  UpdateInActivatableScope(behaviorExternalInterface, runnableUpdatesTickedInStack);
  
  _asyncMessageComponent->ClearCache();
} // Update()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::UpdateInActivatableScope(BehaviorExternalInterface& behaviorExternalInterface, const std::set<IBehavior*>& tickedInStack)
{
  // This is innefficient and should be replaced, but not overengineering right now
  std::set<IBehavior*> allInActivatableScope;
  const RunnableStack::DelegatesMap& delegatesMap = _runnableStack->GetDelegatesMap();
  for(auto& entry: delegatesMap){
    for(auto& runnable : entry.second){
      if(tickedInStack.find(runnable)  == tickedInStack.end()){
          allInActivatableScope.insert(runnable);
      }
    }
  }
  for(auto& entry: allInActivatableScope){
    _asyncMessageComponent->GetEventsForBehavior(
       entry,
       behaviorExternalInterface.GetStateChangeComponent()._gameToEngineEvents);
    _asyncMessageComponent->GetEventsForBehavior(
       entry,
       behaviorExternalInterface.GetStateChangeComponent()._engineToGameEvents);
    _asyncMessageComponent->GetEventsForBehavior(
       entry,
       behaviorExternalInterface.GetStateChangeComponent()._robotToEngineEvents);

    entry->Update(behaviorExternalInterface);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSystemManager::IsControlDelegated(const IBehavior* delegator)
{
  return (_runnableStack->IsInStack(delegator)) &&
         (_runnableStack->GetTopOfStack() != delegator);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSystemManager::CanDelegate(IBehavior* delegator)
{
  return _runnableStack->GetTopOfStack() == delegator;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSystemManager::Delegate(IBehavior* delegator, IBehavior* delegated)
{
  // Ensure that the delegator is on top of the stack
  if(!ANKI_VERIFY(delegator == _runnableStack->GetTopOfStack(),
                  "BehaviorSystemManager.Delegate.DelegatorNotOnTopOfStack",
                  "")){
    return false;
  }
  
  if(!ANKI_VERIFY(delegated != nullptr,
                  "BehaviorSystemManager.Delegate.DelegatingToNullptr", "")){
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
void BehaviorSystemManager::CancelDelegates(IBehavior* delegator)
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
void BehaviorSystemManager::CancelSelf(IBehavior* delegator)
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

} // namespace Cozmo
} // namespace Anki
