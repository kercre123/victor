/**
* File: iWantsToRunStrategy.h
*
* Author: Kevin M. Karol
* Created: 7/3/17
*
* Description: Interface for generic strategies which can be used across 
* all parts of the behavior system to determine when a 
* behavior/reaction/activity wants to run
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_IWantsToRunStrategy_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_IWantsToRunStrategy_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior_fwd.h"

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/behaviorSystem/strategyTypes.h"
#include "json/json-forwards.h"
#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"

#include <set>

namespace Anki {
namespace Cozmo {

class Robot;
  
class IWantsToRunStrategy{
public:
  IWantsToRunStrategy(Robot& robot, const Json::Value& config);
  virtual ~IWantsToRunStrategy() {};

  bool WantsToRun(const Robot& robot) const;

  // A random number generator all subclasses can share
  Util::RandomGenerator& GetRNG() const;
  
  WantsToRunStrategyType GetStrategyType(){return _strategyType;}
  
  static WantsToRunStrategyType ExtractStrategyType(const Json::Value& config);
  
protected:
  using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
  using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
  using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
  using GameToEngineTag   = ExternalInterface::MessageGameToEngineTag;
  
  // Derived classes should use these methods to subscribe to any tags they
  // are interested in handling.
  void SubscribeToTags(std::set<GameToEngineTag>&& tags);
  void SubscribeToTags(std::set<EngineToGameTag>&& tags);
  
  virtual void AlwaysHandleInternal(const GameToEngineEvent& event, const Robot& robot) {}
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) {}
  
  virtual bool WantsToRunInternal(const Robot& robot) const = 0;

private:
  Robot& _robot;
  std::vector<::Signal::SmartHandle> _eventHandles;
  WantsToRunStrategyType _strategyType;

  void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot);
  void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot);
  
  template<class EventType>
  void HandleEvent(const EventType& event);
  
};

using IWantsToRunStrategyPtr = std::unique_ptr<IWantsToRunStrategy>;

template<class EventType>
void IWantsToRunStrategy::HandleEvent(const EventType& event)
{
  AlwaysHandle(event, _robot);
}

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_IWantsToRunStrategy_H__
