/**
* File: strategyPlacedOnCharger.h
*
* Author: Kevin M. Karol
* Created: 12/08/16
*
* Description: Strategy which wants to run when Cozmo has just been placed
* on a charger
*
* Copyright: Anki, Inc. 2016
*
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyPlacedOnCharger_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyPlacedOnCharger_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"

namespace Anki {
namespace Cozmo {

class StrategyPlacedOnCharger : public IStateConceptStrategy{
public:
  StrategyPlacedOnCharger(BehaviorExternalInterface& behaviorExternalInterface,
                          IExternalInterface* robotExternalInterface,
                          const Json::Value& config);

protected:
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  mutable bool _shouldTrigger = false;
  
  // prevent Cozmo from asking to go to sleep for a period of time after connection
  mutable float _dontRunUntilTime_sec;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyPlacedOnCharger_H__
