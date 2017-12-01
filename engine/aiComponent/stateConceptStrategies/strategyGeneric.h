/**
 * File: strategyGeneric.h
 *
 * Author: Lee Crippen - Al Chaussee
 * Created: 06/20/2017 - 07/11/2017
 *
 * Description: Generic strategy for responding to many configurations of events.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyGeneric_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyGeneric_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategyEventHandler.h"

#include <set>

namespace Anki {
namespace Cozmo {

class StateConceptStrategyMessageHelper;

class StrategyGeneric : public IStateConceptStrategy, private IStateConceptStrategyEventHandler
{
public:

  StrategyGeneric(BehaviorExternalInterface& behaviorExternalInterface,
                  IExternalInterface& robotExternalInterface,
                  const Json::Value& config);

  virtual ~StrategyGeneric();
  
  using E2GHandleCallbackType = std::function<bool(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)>;
  
  void ConfigureRelevantEvents(std::set<EngineToGameTag> relevantEvents,
                               E2GHandleCallbackType callback = E2GHandleCallbackType{});
  
  using G2EHandleCallbackType = std::function<bool(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface)>;
  
  void ConfigureRelevantEvents(std::set<GameToEngineTag> relevantEvents,
                               G2EHandleCallbackType callback = G2EHandleCallbackType{});
  
  using ShouldTriggerCallbackType = std::function<bool(BehaviorExternalInterface& behaviorExternalInterface)>;

  void SetShouldTriggerCallback(ShouldTriggerCallbackType callback) { _shouldTriggerCallback = callback; }
  
protected:

  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
  virtual void HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void HandleEvent(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:

  bool ShouldTrigger(BehaviorExternalInterface& behaviorExternalInterface) const;

  std::set<EngineToGameTag> _relevantEngineToGameEvents;
  E2GHandleCallbackType     _engineToGameHandleCallback;
  
  std::set<GameToEngineTag> _relevantGameToEngineEvents;
  G2EHandleCallbackType     _gameToEngineHandleCallback;
  
  ShouldTriggerCallbackType _shouldTriggerCallback;

  std::unique_ptr<StateConceptStrategyMessageHelper> _messageHelper;
  
  mutable bool _wantsToRun = false;
};

}
}

#endif
