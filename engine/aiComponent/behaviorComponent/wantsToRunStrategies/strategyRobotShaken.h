/**
* File: strategyRobotShaken.h
*
* Author: Matt Michini - Kevin M. Karol
* Created: 2017/01/11  - 7/5/17
*
* Description: Strategy for responding to robot being shaken
*
* Copyright: Anki, Inc. 2017
*
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyRobotShaken_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyRobotShaken_H__

#include "engine/aiComponent/behaviorComponent/wantsToRunStrategies/iWantsToRunStrategy.h"

namespace Anki {
namespace Cozmo {

class StrategyRobotShaken : public IWantsToRunStrategy
{
public:
  StrategyRobotShaken(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);

protected:
  virtual bool WantsToRunInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyRobotShaken_H__
