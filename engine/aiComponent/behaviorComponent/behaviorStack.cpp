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

#include "engine/aiComponent/behaviorComponent/behaviorStack.h"

#include "engine/aiComponent/behaviorComponent/asyncMessageGateComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "engine/viz/vizManager.h"
#include "util/logging/logging.h"

// TODO:(bn) put viz manager in BehaviorExternalInterface, then remove these includes
#include "engine/robot.h"
#include "engine/cozmoContext.h"

namespace Anki {
namespace Cozmo {

namespace{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStack::~BehaviorStack()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::InitBehaviorStack(BehaviorExternalInterface& behaviorExternalInterface,
                                                             IBehavior* baseOfStack)
{
  ANKI_VERIFY(_behaviorStack.empty(),
              "BehaviorSystemManager.BehaviorStack.InitBehaviorStack.StackNotEmptyOnInit",
              "");
  
  baseOfStack->OnEnteredActivatableScope();
  ANKI_VERIFY(baseOfStack->WantsToBeActivated(behaviorExternalInterface),
              "BehaviorSystemManager.BehaviorStack.InitConfig.BasebehaviorDoesn'tWantToRun",
              "");
  PushOntoStack(baseOfStack);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::ClearStack()
{
  const size_t stackSize = _behaviorStack.size();
  for(int i = 0; i < stackSize; i++){
    PopStack();
  }
  DEV_ASSERT(_behaviorStack.empty(), "BehaviorStack.Destructor.NotAllBehaviorsPoppedFromStack");

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::UpdateBehaviorStack(BehaviorExternalInterface& behaviorExternalInterface,
                                        std::vector<ExternalInterface::RobotCompletedAction>& actionsCompletedThisTick,
                                        AsyncMessageGateComponent& asyncMessageGateComp,
                                        std::set<IBehavior*>& tickedInStack)
{
  if(_behaviorStack.size() == 0){
    PRINT_NAMED_WARNING("BehaviorSystemManager.BehaviorStack.UpdateBehaviorStack.NoStackInitialized",
                        "");
    return;
  }

  
  
  // The stack can be altered during update ticks through the cancel delegation
  // functions - so track the index in the stack rather than the iterator directly
  // One side effect of this is that if a Behavior ends this tick without queueing
  // an action we potentially lose a tick before the next time a BSbehavior queues
  // an action - to save on complexity we're accepting this tradeoff for the time being
  // but may decide to address it directly here or within the BSbehavior/one of its subclasses
  // in the future
  for(int idx = 0; idx < _behaviorStack.size(); idx++){
    tickedInStack.insert(_behaviorStack.at(idx));
    behaviorExternalInterface.GetStateChangeComponent()._gameToEngineEvents.clear();
    behaviorExternalInterface.GetStateChangeComponent()._engineToGameEvents.clear();
    behaviorExternalInterface.GetStateChangeComponent()._robotToEngineEvents.clear();
    behaviorExternalInterface.GetStateChangeComponent()._actionsCompletedThisTick.clear();
    
    asyncMessageGateComp.GetEventsForBehavior(
       _behaviorStack.at(idx),
       behaviorExternalInterface.GetStateChangeComponent()._gameToEngineEvents);
    asyncMessageGateComp.GetEventsForBehavior(
       _behaviorStack.at(idx),
       behaviorExternalInterface.GetStateChangeComponent()._engineToGameEvents);
    asyncMessageGateComp.GetEventsForBehavior(
       _behaviorStack.at(idx),
       behaviorExternalInterface.GetStateChangeComponent()._robotToEngineEvents);
    
    // Set the actions completed this tick for the top of the stack
    if(idx == (_behaviorStack.size() - 1)){
      behaviorExternalInterface.GetStateChangeComponent()._actionsCompletedThisTick = actionsCompletedThisTick;
    }
    
    _behaviorStack.at(idx)->Update(behaviorExternalInterface);
  }

  if( ANKI_DEV_CHEATS ) {
    SendDebugVizMessages(behaviorExternalInterface);
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const IBehavior* BehaviorStack::GetBehaviorInStackAbove(const IBehavior* behavior) const
{
  const auto it = _behaviorToIndexMap.find(behavior);
  if( it != _behaviorToIndexMap.end() ) {
    const int idxAbove = it->second + 1;
    if( _behaviorStack.size() > idxAbove ) {
      return _behaviorStack[idxAbove];
    }
  }

  return nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::PushOntoStack(IBehavior* behavior)
{
  _behaviorToIndexMap.insert(std::make_pair(behavior, _behaviorStack.size()));
  _behaviorStack.push_back(behavior);
  
  PrepareDelegatesToEnterScope(behavior);
  behavior->OnActivated(*_behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::PopStack()
{
  PrepareDelegatesForRemovalFromStack(_behaviorStack.back());

  _behaviorStack.back()->OnDeactivated(*_behaviorExternalInterface);
  
  _behaviorToIndexMap.erase(_behaviorStack.back());
  _behaviorStack.pop_back();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::PrepareDelegatesToEnterScope(IBehavior* delegated)
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
void BehaviorStack::PrepareDelegatesForRemovalFromStack(IBehavior* delegated)
{
  auto availableDelegates = _delegatesMap.find(delegated);
  DEV_ASSERT(availableDelegates != _delegatesMap.end(),
             "BehaviorStack.PrepareDelegateForRemovalFromStack.DelegateNotFound");
  for(auto& entry: availableDelegates->second){
    entry->OnLeftActivatableScope();
  }
  
  _delegatesMap.erase(delegated);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::DebugPrintStack(const std::string& debugStr) const
{
  for( size_t i=0; i<_behaviorStack.size(); ++i) {
    PRINT_CH_DEBUG("BehaviorSystem", ("BehaviorSystemManager.Stack." + debugStr).c_str(),
                   "%zu: %s",
                   i,
                   _behaviorStack[i]->GetPrintableID().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::SendDebugVizMessages(BehaviorExternalInterface& behaviorExternalInterface) const
{
  VizInterface::BehaviorStackDebug data;

  for( const auto& behavior : _behaviorStack ) {
    data.debugStrings.push_back( behavior->GetPrintableID() );
  }  
  
  behaviorExternalInterface.GetRobot().GetContext()->GetVizManager()->SendBehaviorStackDebug(std::move(data));
}


} // namespace Cozmo
} // namespace Anki
