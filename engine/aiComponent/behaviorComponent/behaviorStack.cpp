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

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/asyncMessageGateComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "engine/viz/vizManager.h"
#include "util/logging/logging.h"
// #include "webServerProcess/src/webService.h"

// TODO:(bn) put viz manager in BehaviorExternalInterface, then remove these includes
#include "engine/cozmoContext.h"

namespace Anki {
namespace Cozmo {

namespace{
  const std::string kWebVizModuleName = "behaviors";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStack::~BehaviorStack()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::InitBehaviorStack(IBehavior* baseOfStack)
{
  ANKI_VERIFY(_behaviorStack.empty(),
              "BehaviorSystemManager.BehaviorStack.InitBehaviorStack.StackNotEmptyOnInit",
              "");
  
  baseOfStack->OnEnteredActivatableScope();
  ANKI_VERIFY(baseOfStack->WantsToBeActivated(),
              "BehaviorSystemManager.BehaviorStack.InitConfig.BasebehaviorDoesn'tWantToRun",
              "");
  PushOntoStack(baseOfStack);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::ClearStack()
{
  const bool behaviorStackAlreadyEmpty = _behaviorStack.empty();
  
  const size_t stackSize = _behaviorStack.size();
  for(int i = 0; i + 1 < stackSize; i++){
    PopStack();
  }

  // the base of the stack was manually put into scope during InitBehaviorStack, so undo that manually here
  if(!behaviorStackAlreadyEmpty &&
     ANKI_VERIFY(! _behaviorStack.empty(),
                 "BehaviorStack.ClearStack.StackImproperlyEmpty",
                 "") ) {
    IBehavior* oldBaseBehavior = _behaviorStack.back();
    PopStack();
    oldBaseBehavior->OnLeftActivatableScope();
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
  behaviorExternalInterface.GetBehaviorEventComponent()._actionsCompletedThisTick.clear();  
  for(int idx = 0; idx < _behaviorStack.size(); idx++){
    tickedInStack.insert(_behaviorStack.at(idx));
    behaviorExternalInterface.GetBehaviorEventComponent()._gameToEngineEvents.clear();
    behaviorExternalInterface.GetBehaviorEventComponent()._engineToGameEvents.clear();
    behaviorExternalInterface.GetBehaviorEventComponent()._robotToEngineEvents.clear();
    
    asyncMessageGateComp.GetEventsForBehavior(
       _behaviorStack.at(idx),
       behaviorExternalInterface.GetBehaviorEventComponent()._gameToEngineEvents);
    asyncMessageGateComp.GetEventsForBehavior(
       _behaviorStack.at(idx),
       behaviorExternalInterface.GetBehaviorEventComponent()._engineToGameEvents);
    asyncMessageGateComp.GetEventsForBehavior(
       _behaviorStack.at(idx),
       behaviorExternalInterface.GetBehaviorEventComponent()._robotToEngineEvents);
    
    // Set the actions completed this tick for the top of the stack
    if(idx == (_behaviorStack.size() - 1)){
      behaviorExternalInterface.GetBehaviorEventComponent()._actionsCompletedThisTick = actionsCompletedThisTick;
    }
    
    _behaviorStack.at(idx)->Update();
  }

  if( ANKI_DEV_CHEATS ) {
    SendDebugVizMessages(behaviorExternalInterface);
    
    if( behaviorWebVizDirty ) {
      SendDebugBehaviorTreeToWebViz( behaviorExternalInterface );
      behaviorWebVizDirty = false;
    }
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
  behavior->OnActivated();
  
  behaviorWebVizDirty = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::PopStack()
{
  PrepareDelegatesForRemovalFromStack(_behaviorStack.back());

  _behaviorStack.back()->OnDeactivated();
  
  _behaviorToIndexMap.erase(_behaviorStack.back());
  _behaviorStack.pop_back();
  
  behaviorWebVizDirty = true;
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
                   _behaviorStack[i]->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::SendDebugVizMessages(BehaviorExternalInterface& behaviorExternalInterface) const
{
  VizInterface::BehaviorStackDebug data;

  for( const auto& behavior : _behaviorStack ) {
    data.debugStrings.push_back( behavior->GetDebugLabel() );
  }
  
  auto context = behaviorExternalInterface.GetRobotInfo().GetContext();
  if(context != nullptr){
    auto vizManager = context->GetVizManager();
    if(vizManager != nullptr){
      vizManager->SendBehaviorStackDebug(std::move(data));
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStack::SendDebugBehaviorTreeToWebViz(BehaviorExternalInterface& behaviorExternalInterface) const
{
  Json::Value data = BuildDebugBehaviorTree(behaviorExternalInterface);
  
  const auto* context = behaviorExternalInterface.GetRobotInfo().GetContext();
  if( context != nullptr ) {
    // const auto* webService = context->GetWebService();
    // if( webService != nullptr ){
      // webService->SendToWebViz( kWebVizModuleName, data );
    // }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value BehaviorStack::BuildDebugBehaviorTree(BehaviorExternalInterface& behaviorExternalInterface) const
{
   
  Json::Value data;
  data["time"] = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto& tree = data["tree"];
  auto& stack = data["stack"];
  
  // construct flat table of tree relationships
  for( const auto& elem : _delegatesMap ) {
    for( const auto& child : elem.second ) {
      Json::Value relationship;
      relationship["behaviorID"] = child->GetDebugLabel();
      relationship["parent"] = elem.first->GetDebugLabel();
      tree.append( relationship );
    }
  }
  if( !_behaviorStack.empty() ) {
    Json::Value relationship;
    relationship["behaviorID"] = _behaviorStack.front()->GetDebugLabel();
    relationship["parent"] = Json::nullValue;
    tree.append( relationship );
  }
  for( const auto& stackElem : _behaviorStack ) {
    stack.append( stackElem->GetDebugLabel() );
  }
  
  return data;
}


} // namespace Cozmo
} // namespace Anki
