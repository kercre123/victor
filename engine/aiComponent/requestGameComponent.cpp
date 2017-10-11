/**
 * File: requestGameComponent.cpp
 *
 * Author: Kevin M. Karol
 * Created: 05/18/17
 *
 * Description: Component which keeps track of what game cozmo should request next
 * across all ways game requests can be initiated
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/requestGameComponent.h"

#include "engine/ankiEventUtil.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/robot.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/jsonTools.h"

#include "util/helpers/boundedWhile.h"

namespace Anki{
namespace Cozmo{

namespace{
const char * kUnlockIDConfigKey   = "unlockID";
const char * kWeightConfigKey     = "weight";
const int kMaxRetrys = 1000;
}
  


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RequestGameComponent::RequestGameComponent(Robot& robot)
: _lastGameRequested(UnlockId::Count)
, _canRequestGame(false)
, _shouldAutomaticallyResetDefaults(false)
{
  // load the lets play mapping in from JSON
  const Json::Value& letsPlayConfig = robot.GetContext()->GetDataLoader()->GetGameRequestWeightsConfig();
  for(const auto& entry: letsPlayConfig){
    UnlockId unlockID = UnlockIdFromString(
              JsonTools::ParseString(entry,
                                     kUnlockIDConfigKey,
                                     "RequestGameComponent.UnlockID"));
    
    int weight = JsonTools::ParseUint8(entry,
                                       kWeightConfigKey,
                                       "RequestGameComponent.Weight");
    _gameRequests.insert(
          std::make_pair(unlockID,
              GameRequestData(unlockID, weight)));
  }
  _defaultGameRequests = _gameRequests;
  
  // register to receive notification from the game about when game requests are allowed
  if ( robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _eventHandles);
    helper.SubscribeGameToEngine<ExternalInterface::MessageGameToEngineTag::CanCozmoRequestGame>();
    helper.SubscribeGameToEngine<ExternalInterface::MessageGameToEngineTag::SetOverrideGameRequestWeights>();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UnlockId RequestGameComponent::IdentifyNextGameTypeToRequest(const Robot& robot)
{
  // Ensure game has said games can be requested
  if(!_canRequestGame){
    return UnlockId::Count;
  }
  
  // Check to see if a request type has already been returned this tick - this
  // ensures randomization doesn't change the value returned within the same tick
  UnlockId nextRequest = UnlockId::Count;
  const BaseStationTime_t currentTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  if(_cacheData._lastTimeUpdated_ns >= currentTime_ns){
    return _cacheData._cachedRequest;
  }
  
  
  
  // Map weight to behavior for unlocked
  std::vector<std::pair<int, UnlockId>> unlockedBehaviors;
  int totalWeight = 0;
  for(const auto& entry: _gameRequests){
    if(robot.GetProgressionUnlockComponent().IsUnlocked(entry.second._unlockID)){
      totalWeight += entry.second._weight;
      unlockedBehaviors.emplace_back(std::make_pair(totalWeight,
                                                    entry.first));
    }
  }
  
  // Select the best game request - if there are more than one valid games to
  // request, choose one that is not the last game requested
  if(unlockedBehaviors.size() == 0){
    // don't do anything - cached value will be cleared
  }else if(unlockedBehaviors.size() == 1){
    nextRequest = (*unlockedBehaviors.begin()).second;
  }else{
    int randomIndicator = 0;
    BOUNDED_WHILE(kMaxRetrys, ((nextRequest == UnlockId::Count) ||
                               (_lastGameRequested == nextRequest)))
    {
      // to achieve weighted randomness the random indicator maps to the first entry
      // in the vector that it's value is less than
      randomIndicator = robot.GetRNG().RandInt(totalWeight);
      for(const auto& entry: unlockedBehaviors){
        if(randomIndicator <  entry.first){
          nextRequest = entry.second;
          break;
        }
      }
    }
  }
  
  _cacheData._cachedRequest = nextRequest;
  _cacheData._lastTimeUpdated_ns = currentTime_ns;
  return nextRequest;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RequestGameComponent::RegisterRequestingGameType(UnlockId unlockId)
{
  auto iter = _gameRequests.find(unlockId);
  if(ANKI_VERIFY(iter != _gameRequests.end(),
                 "RequestGameComponent.RegisterRequestingGame.UnlockIDNotFound",
                 "Game requestes does not contain entry for %s",
                 UnlockIdToString(unlockId))){
    _lastGameRequested = iter->first;
    if(_shouldAutomaticallyResetDefaults){
      _gameRequests = _defaultGameRequests;
      _shouldAutomaticallyResetDefaults = false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RequestGameComponent::HandleMessage(const ExternalInterface::CanCozmoRequestGame& msg)
{
  // set whether Cozmo is currently allowed to request games or not
  _canRequestGame = msg.canRequest;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RequestGameComponent::HandleMessage(const ExternalInterface::SetOverrideGameRequestWeights& msg)
{
  if( msg.useDefaults )
  {
    _gameRequests = _defaultGameRequests;
  }
  else
  {
    DEV_ASSERT(msg.unlockIDs.size() == msg.weights.size(), "RequestGameComponent.HandleMessage.SetOverrideGameRequestWeights");
    
    _gameRequests.clear();
    auto lenRequests = msg.unlockIDs.size();
    for( int i = 0; i < lenRequests; ++i )
    {
      _gameRequests.insert(std::make_pair(msg.unlockIDs[i],
                          GameRequestData(msg.unlockIDs[i],msg.weights[i])));
    }
  }
  _shouldAutomaticallyResetDefaults = msg.resetDefaultsOnNextRequest;
}



} // namespace Cozmo
} // namespace Anki
