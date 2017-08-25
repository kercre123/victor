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

#include "engine/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"

namespace Anki {
namespace Cozmo {

class StrategyGeneric : public IWantsToRunStrategy
{
public:

  StrategyGeneric(Robot& robot, const Json::Value& config);
  
  using E2GHandleCallbackType = std::function<bool(const EngineToGameEvent& event, const Robot& robot)>;
  
  void ConfigureRelevantEvents(std::set<EngineToGameTag> relevantEvents,
                               E2GHandleCallbackType callback = E2GHandleCallbackType{});
  
  using G2EHandleCallbackType = std::function<bool(const GameToEngineEvent& event, const Robot& robot)>;
  
  void ConfigureRelevantEvents(std::set<GameToEngineTag> relevantEvents,
                               G2EHandleCallbackType callback = G2EHandleCallbackType{});
  
  using ShouldTriggerCallbackType = std::function<bool(const Robot& robot)>;

  void SetShouldTriggerCallback(ShouldTriggerCallbackType callback) { _shouldTriggerCallback = callback; }
  
protected:

  virtual bool WantsToRunInternal(const Robot& robot) const override;
  
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
  virtual void AlwaysHandleInternal(const GameToEngineEvent& event, const Robot& robot) override;
  
private:

  bool ShouldTrigger(const Robot& robot) const;

  std::set<EngineToGameTag> _relevantEngineToGameEvents;
  E2GHandleCallbackType     _engineToGameHandleCallback;
  
  std::set<GameToEngineTag> _relevantGameToEngineEvents;
  G2EHandleCallbackType     _gameToEngineHandleCallback;
  
  ShouldTriggerCallbackType _shouldTriggerCallback;
  
  mutable bool _wantsToRun = false;
};

}
}

#endif
