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

#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"

namespace Anki {
namespace Cozmo {

class StrategyRobotShaken : public IWantsToRunStrategy
{
public:
  StrategyRobotShaken(Robot& robot, const Json::Value& config);

protected:
  virtual bool WantsToRunInternal(const Robot& robot) const override;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyRobotShaken_H__
