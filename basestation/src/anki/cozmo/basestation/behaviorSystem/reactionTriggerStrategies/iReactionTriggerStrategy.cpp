/**
 * File: iReactionTriggerStrategy.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Interface for defining when a reaction trigger should fire its
 * mapped behavior.  Strategy to behavior map is defined in behaviorManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"



namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IReactionTriggerStrategy::IReactionTriggerStrategy(Robot& robot, const Json::Value& config, const std::string& strategyName)
:  _robot(robot)
,  _strategyName(strategyName)
{
  // Get disable reaction messages
  
  SubscribeToTags({
    MessageGameToEngineTag::RequestEnableReactionTrigger
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::SubscribeToTags(std::set<GameToEngineTag> &&tags)
{
  if(_robot.HasExternalInterface()) {
    auto handlerCallback = [this](const GameToEngineEvent& event) {
      HandleEvent(event);
    };
    
    for(auto tag : tags) {
      _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::SubscribeToTags(std::set<EngineToGameTag> &&tags)
{
  if(_robot.HasExternalInterface()) {
    auto handlerCallback = [this](const EngineToGameEvent& event) {
      HandleEvent(event);
    };
    
    for(auto tag : tags) {
      _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
    }
  }
}
  
void IReactionTriggerStrategy::AlwaysHandle(const GameToEngineEvent& event, const Robot& robot)
{
  //Turn off reactionary behaviors based on name
  GameToEngineTag tag = event.GetData().GetTag();
  
  if (tag == GameToEngineTag::RequestEnableReactionTrigger){
    auto& data = event.GetData().Get_RequestEnableReactionTrigger();
    if(data.trigger == _triggerID){
      std::string requesterID = data.requesterID;
      const bool enable = data.enable;
      if(UpdateDisableIDs(requesterID, enable)){
        // Allow subclasses to respond to state changes
        if((!enable && _disableIDs.size() == 1) ||
           (enable && _disableIDs.size() == 0)){
          EnabledStateChanged(enable);
        }
      }
    }
  }else{
    AlwaysHandleInternal(event, robot);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  AlwaysHandleInternal(event, robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IReactionTriggerStrategy::UpdateDisableIDs(const std::string& requesterID, bool enable)
{
  
  PRINT_CH_DEBUG("ReactionTriggers", "BehaviorInterface.ReactionaryBehavior.UpdateDisableIDs",
                 "%s: requesting behavior %s be %s",
                 requesterID.c_str(),
                 _strategyName.c_str(),
                 enable ? "enabled" : "disabled");
  
  if(enable){
    int countRemoved = (int)_disableIDs.erase(requesterID);
    if(!countRemoved){
      PRINT_NAMED_WARNING("ReactionTriggerStrategy.UpdateDisableIDs.InvalidDisable",
                          "Attempted to enable reactionary behavior %s with invalid ID %s", _strategyName.c_str(), requesterID.c_str());
      return false;
    }
    
  }else{
    int countInList = (int)_disableIDs.count(requesterID);
    if(countInList){
      PRINT_NAMED_WARNING("ReactionTriggerStrategy.UpdateDisableIDs.DuplicateDisable",
                          "Attempted to disable reactionary behavior %s with previously registered ID %s", _strategyName.c_str(), requesterID.c_str());
      return false;
    }else{
      _disableIDs.insert(requesterID);
    }
    
  }
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& IReactionTriggerStrategy::GetRNG() const
{
  return _robot.GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
