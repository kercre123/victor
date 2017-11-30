/**
* File: iStateConceptStrategy.h
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

#ifndef __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IStateConceptStrategy_H__
#define __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IStateConceptStrategy_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy_fwd.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include "engine/events/ankiEvent.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/behaviorComponent/strategyTypes.h"
#include "json/json-forwards.h"
#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"

#include <set>

namespace Anki {
namespace Cozmo {

class BehaviorExternalInterface;
class IExternalInterface;
class Robot;
  
class IStateConceptStrategy{
public:
  static Json::Value GenerateBaseStrategyConfig(StateConceptStrategyType type);  
  static StateConceptStrategyType ExtractStrategyType(const Json::Value& config);

  IStateConceptStrategy(BehaviorExternalInterface& behaviorExternalInterface,
                      IExternalInterface* robotExternalInterface,
                      const Json::Value& config);
  virtual ~IStateConceptStrategy() {};

  bool AreStateConditionsMet(BehaviorExternalInterface& behaviorExternalInterface) const;

  // A random number generator all subclasses can share
  Util::RandomGenerator& GetRNG() const;
  
  StateConceptStrategyType GetStrategyType(){return _strategyType;}
  
protected:
  using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
  using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
  using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
  using GameToEngineTag   = ExternalInterface::MessageGameToEngineTag;
  
  // Derived classes should use these methods to subscribe to any tags they
  // are interested in handling.
  void SubscribeToTags(std::set<GameToEngineTag>&& tags);
  void SubscribeToTags(std::set<EngineToGameTag>&& tags);
  
  virtual void AlwaysHandleInternal(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) {}
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) {}
  
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const = 0;

private:
  BehaviorExternalInterface& _behaviorExternalInterface;
  IExternalInterface* _robotExternalInterface;
  std::vector<::Signal::SmartHandle> _eventHandles;
  StateConceptStrategyType _strategyType;

  void AlwaysHandle(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface);
  void AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface);
  
  template<class EventType>
  void HandleEvent(const EventType& event);
  
};

template<class EventType>
void IStateConceptStrategy::HandleEvent(const EventType& event)
{
  AlwaysHandle(event, _behaviorExternalInterface);
}

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IStateConceptStrategy_H__
