/**
* File: strategyInNeedsBracket.h
*
* Author: Brad Neuman - Kevin M. Karol
* Created: 2017-06-20 - 2017-07-05
*
* Description: Strategy which wants to run whenever the specified need is in
* the specified need bracket
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyInNeedsBracket_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyInNeedsBracket_H__

#include "engine/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"

#include "clad/types/needsSystemTypes.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class StrategyInNeedsBracket : public IWantsToRunStrategy
{
public:
  StrategyInNeedsBracket(Robot& robot, const Json::Value& config);

protected:
  virtual bool WantsToRunInternal(const Robot& robot) const override;

private:
  NeedId _need;
  NeedBracketId _requiredBracket;

  bool InRequiredNeedBracket(const Robot& robot) const;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyInNeedsBracket_H__
