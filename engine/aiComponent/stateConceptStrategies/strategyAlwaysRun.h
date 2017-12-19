/**
* File: strategyAlwaysRun.h
*
* Author: Raul - Kevin M. Karol
* Created: 08/10/2016 - 7/5/17
*
* Description: Strategy which always wants to run
*
* Copyright: Anki, Inc. 2016 - 2017
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyAlwaysRun_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyAlwaysRun_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class StrategyAlwaysRun : public IStateConceptStrategy
{
public:
  // constructor
  explicit StrategyAlwaysRun(const Json::Value& config);

protected:
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override{ return true;}

};
  
} // namespace
} // namespace

#endif // endif __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyAlwaysRun_H__
