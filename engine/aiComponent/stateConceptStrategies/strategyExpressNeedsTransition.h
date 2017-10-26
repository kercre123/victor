/**
* File: strategyExpressNeedsTransition.h
*
* Author: Brad Neuman - Kevin M. Karol
* Created: 2017-06-20 - 2017-07-05
*
* Description: Strategy which wants to run when cozmo's need bracket and expressed
* need state differ from each other
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyExpressNeedsTransition_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyExpressNeedsTransition_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"

#include "clad/types/needsSystemTypes.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class StrategyExpressNeedsTransition : public IStateConceptStrategy
{
public:
  StrategyExpressNeedsTransition(BehaviorExternalInterface& behaviorExternalInterface,
                                 IExternalInterface* robotExternalInterface,
                                 const Json::Value& config);

protected:
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  NeedId _need;
  
  bool InRequiredNeedBracket(BehaviorExternalInterface& behaviorExternalInterface) const;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyExpressNeedsTransition_H__
