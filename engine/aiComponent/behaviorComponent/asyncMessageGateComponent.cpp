/**
* File: AsyncMessageGateComponent.h
*
* Author: Kevin Karol
* Date:   10/07/2017
*
* Description: Caches messages that come in asyncronously over the tick
* until they are requested syncronously
*
* Copyright: Anki, Inc. 2017
**/

#include "engine/aiComponent/behaviorComponent/asyncMessageGateComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "proto/external_interface/shared.pb.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AsyncMessageGateComponent::AsyncMessageGateComponent(IExternalInterface* externalInterface,
                                                     IGatewayInterface* gatewayInterface,
                                                     RobotInterface::MessageHandler* robotInterface)
: IDependencyManagedComponent(this, BCComponentID::AsyncMessageComponent)
, _externalInterface(externalInterface)
, _gatewayInterface(gatewayInterface)
, _robotInterface(robotInterface)
, _isCacheValid(false)
, _cachedTracker(new EventTracker())
, _activeTracker(new EventTracker())
{
  

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::PrepareCache()
{
  DEV_ASSERT(!_isCacheValid,
             "AsyncMessageGateComponent.PrepareCache.CacheNotClear");

  {
    std::lock_guard<std::mutex> lock(_activeTracker->_gameToEngineMutex);
    _cachedTracker->_gameToEngineEvents = std::move(_activeTracker->_gameToEngineEvents);
    _activeTracker->_gameToEngineEvents.clear();
  }
  {
    std::lock_guard<std::mutex> lock(_activeTracker->_engineToGameMutex);
    _cachedTracker->_engineToGameEvents = std::move(_activeTracker->_engineToGameEvents);
    _activeTracker->_engineToGameEvents.clear();
  }
  {
    std::lock_guard<std::mutex> lock(_activeTracker->_robotToEngineMutex);
    _cachedTracker->_robotToEngineEvents = std::move(_activeTracker->_robotToEngineEvents);
    _activeTracker->_robotToEngineEvents.clear();
  }
  {
    std::lock_guard<std::mutex> lock(_activeTracker->_appToEngineMutex);
    _cachedTracker->_appToEngineEvents = std::move(_activeTracker->_appToEngineEvents);
    _activeTracker->_appToEngineEvents.clear();
  }
  
  ///
  /// TODO: probably have to check if pair already exists
  ////
  
  for(int idx = 0; idx < _cachedTracker->_gameToEngineEvents.size(); idx++){
    auto eventTag = _cachedTracker->_gameToEngineEvents[idx].GetData().GetTag();
    auto subscriberIter = _gameToEngineSubscribers.find(eventTag);
    if(subscriberIter != _gameToEngineSubscribers.end()){
      for(auto& behavior: subscriberIter->second){
        // Create wrapper if necessary
        auto cacheMapEntry = _cachedTracker->_cacheMap.find(behavior);
        if(cacheMapEntry == _cachedTracker->_cacheMap.end()){
          EventListWrapper wrapper;
          wrapper._gameToEngineIdxs.push_back(idx);
          _cachedTracker->_cacheMap[behavior] = wrapper;
        }else{
          _cachedTracker->_cacheMap[behavior]._gameToEngineIdxs.push_back(idx);
        }
        
      }
    }
  }
  
  for(int idx = 0; idx < _cachedTracker->_engineToGameEvents.size(); idx++){
    auto eventTag = _cachedTracker->_engineToGameEvents[idx].GetData().GetTag();
    auto subscriberIter = _engineToGameSubscribers.find(eventTag);
    if(subscriberIter != _engineToGameSubscribers.end()){
      for(auto& behavior: subscriberIter->second){
        // Create wrapper if necessary
        auto cacheMapEntry = _cachedTracker->_cacheMap.find(behavior);
        if(cacheMapEntry == _cachedTracker->_cacheMap.end()){
          EventListWrapper wrapper;
          wrapper._engineToGameIdxs.push_back(idx);
          _cachedTracker->_cacheMap[behavior] = wrapper;
        }else{
          _cachedTracker->_cacheMap[behavior]._engineToGameIdxs.push_back(idx);
        }
        
      }
    }
  }
  
  for(int idx = 0; idx < _cachedTracker->_robotToEngineEvents.size(); idx++){
    auto eventTag = _cachedTracker->_robotToEngineEvents[idx].GetData().GetTag();

    auto subscriberIter = _robotToEngineSubscribers.find(eventTag);
    if(subscriberIter != _robotToEngineSubscribers.end()){
      for(auto& behavior: subscriberIter->second){
        // Create wrapper if necessary
        auto cacheMapEntry = _cachedTracker->_cacheMap.find(behavior);
        if(cacheMapEntry == _cachedTracker->_cacheMap.end()){
          EventListWrapper wrapper;
          wrapper._robotToEngineIdxs.push_back(idx);
          _cachedTracker->_cacheMap[behavior] = wrapper;
        }else{
          _cachedTracker->_cacheMap[behavior]._robotToEngineIdxs.push_back(idx);
        }
      }
    }
  }
  
  for(int idx = 0; idx < _cachedTracker->_appToEngineEvents.size(); idx++){
    auto eventTag = _cachedTracker->_appToEngineEvents[idx].GetData().GetTag();
    auto subscriberIter = _appToEngineSubscribers.find(eventTag);
    if(subscriberIter != _appToEngineSubscribers.end()){
      for(auto& behavior : subscriberIter->second){
        // Create wrapper if necessary
        auto cacheMapEntry = _cachedTracker->_cacheMap.find(behavior);
        if(cacheMapEntry == _cachedTracker->_cacheMap.end()){
          EventListWrapper wrapper;
          wrapper._appToEngineIdxs.push_back(idx);
          _cachedTracker->_cacheMap[behavior] = wrapper;
        }else{
          _cachedTracker->_cacheMap[behavior]._appToEngineIdxs.push_back(idx);
        }
        
      }
    }
  }
  
  _isCacheValid = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::GetEventsForBehavior(IBehavior* subscriber,
                                                     std::vector<const GameToEngineEvent>& events)
{
  if(!ANKI_VERIFY(events.empty(), 
      "AsyncMessageGateComponent.GetEventsForBehavior.GameToEngineEvents",
      "Events not empty")){
    events.clear();    
  }
  DEV_ASSERT(_isCacheValid,
             "AsyncMessageGateComponent.GetGTEEventsForBehavior.CacheIsClear");
  
  auto behaviorEntry = _cachedTracker->_cacheMap.find(subscriber);
  if(behaviorEntry != _cachedTracker->_cacheMap.end()){
    for(auto idx: behaviorEntry->second._gameToEngineIdxs){
      events.push_back(_cachedTracker->_gameToEngineEvents[idx]);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::GetEventsForBehavior(IBehavior* subscriber,
                                                     std::vector<const EngineToGameEvent>& events)
{
  if(!ANKI_VERIFY(events.empty(), 
      "AsyncMessageGateComponent.GetEventsForBehavior.EngineToGameEvents",
      "Events not empty")){
    events.clear();    
  }
  DEV_ASSERT(_isCacheValid,
             "AsyncMessageGateComponent.GetETGEventsForBehavior.CacheIsClear");
  
  auto behaviorEntry = _cachedTracker->_cacheMap.find(subscriber);
  if(behaviorEntry != _cachedTracker->_cacheMap.end()){
    for(auto idx: behaviorEntry->second._engineToGameIdxs){
      events.push_back(_cachedTracker->_engineToGameEvents[idx]);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::GetEventsForBehavior(IBehavior* subscriber,
                                                     std::vector<const RobotToEngineEvent>& events)
{
  if(!ANKI_VERIFY(events.empty(), 
      "AsyncMessageGateComponent.GetEventsForBehavior.RobotToEngineEvent",
      "Events not empty")){
    events.clear();    
  }
  DEV_ASSERT(_isCacheValid,
             "AsyncMessageGateComponent.GetRTEEventsForBehavior.CacheIsClear");
  auto behaviorEntry = _cachedTracker->_cacheMap.find(subscriber);
  if(behaviorEntry != _cachedTracker->_cacheMap.end()){
    for(auto idx: behaviorEntry->second._robotToEngineIdxs){
      events.push_back(_cachedTracker->_robotToEngineEvents[idx]);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::GetEventsForBehavior(IBehavior* subscriber,
                                                     std::vector<const AppToEngineEvent>& events)
{
  if(!ANKI_VERIFY(events.empty(),
      "AsyncMessageGateComponent.GetEventsForBehavior.AppToEngineEvents",
      "Events not empty")){
    events.clear();
  }
  DEV_ASSERT(_isCacheValid,
             "AsyncMessageGateComponent.GetATEEventsForBehavior.CacheIsClear");
  
  auto behaviorEntry = _cachedTracker->_cacheMap.find(subscriber);
  if(behaviorEntry != _cachedTracker->_cacheMap.end()){
    for(auto idx: behaviorEntry->second._appToEngineIdxs){
      events.push_back(_cachedTracker->_appToEngineEvents[idx]);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::ClearCache()
{
  _cachedTracker.reset(new EventTracker());
  _isCacheValid = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::SubscribeToTags(IBehavior* subscriber,
                                                std::set<ExternalInterface::MessageGameToEngineTag>&& tags)
{
  if(_externalInterface == nullptr){
    return;
  }
  
  for(auto& tag: tags){
    auto tagIter = _gameToEngineSubscribers.find(tag);
    if(tagIter == _gameToEngineSubscribers.end()){
      std::set<IBehavior*> subscribers = {subscriber};
      _gameToEngineSubscribers[tag] = subscribers;
      _eventHandles.push_back(_externalInterface->Subscribe(tag,
                                    [this](const GameToEngineEvent& event){
                                      std::lock_guard<std::mutex> lock(_activeTracker->_gameToEngineMutex);
                                      _activeTracker->_gameToEngineEvents.push_back(event);
                                    })
                              );
    }else{
      _gameToEngineSubscribers[tag].insert(subscriber);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::SubscribeToTags(IBehavior* subscriber,
                                                std::set<ExternalInterface::MessageEngineToGameTag>&& tags)
{
  if(_externalInterface == nullptr){
    return;
  }
  
  for(auto& tag: tags){
    auto tagIter = _engineToGameSubscribers.find(tag);
    if(tagIter == _engineToGameSubscribers.end()){
      std::set<IBehavior*> subscribers = {subscriber};
      _engineToGameSubscribers[tag] = subscribers;
      _eventHandles.push_back(_externalInterface->Subscribe(tag,
                                   [this](const EngineToGameEvent& event){
                                     std::lock_guard<std::mutex> lock(_activeTracker->_engineToGameMutex);
                                     _activeTracker->_engineToGameEvents.push_back(event);
                                   })
                               );
    }else{
      _engineToGameSubscribers[tag].insert(subscriber);
    }
  }
  
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::SubscribeToTags(IBehavior* subscriber,
                                                std::set<RobotInterface::RobotToEngineTag>&& tags)
{
  if(_robotInterface == nullptr){
    return;
  }
  for(auto& tag: tags){
    auto tagIter = _robotToEngineSubscribers.find(tag);
    if(tagIter == _robotToEngineSubscribers.end()){
      std::set<IBehavior*> subscribers = {subscriber};
      _robotToEngineSubscribers[tag] = subscribers;
      
      _eventHandles.push_back(_robotInterface->Subscribe(
                                  tag,
                                  [this](const RobotToEngineEvent& event){
                                     std::lock_guard<std::mutex> lock(_activeTracker->_robotToEngineMutex);
                                     _activeTracker->_robotToEngineEvents.push_back(event);
                                   })
                               );
    }else{
      _robotToEngineSubscribers[tag].insert(subscriber);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AsyncMessageGateComponent::SubscribeToTags(IBehavior* subscriber,
                                                std::set<AppToEngineTag>&& tags)
{
  if(_gatewayInterface == nullptr){
    return;
  }
  
  for(auto& tag: tags){
    auto tagIter = _appToEngineSubscribers.find(tag);
    if(tagIter == _appToEngineSubscribers.end()){
      std::set<IBehavior*> subscribers = {subscriber};
      _appToEngineSubscribers[tag] = subscribers;
      _eventHandles.push_back(_gatewayInterface->Subscribe(tag,
                                    [this](const AppToEngineEvent& event){
                                      std::lock_guard<std::mutex> lock(_activeTracker->_appToEngineMutex);
                                      _activeTracker->_appToEngineEvents.push_back(event);
                                    })
                              );
    }else{
      _appToEngineSubscribers[tag].insert(subscriber);
    }
  }
}

} // namespace Cozmo
} // namespace Anki
