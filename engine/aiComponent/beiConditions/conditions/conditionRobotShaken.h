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

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotShaken_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotShaken_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionRobotShaken : public IBEICondition
{
public:
  ConditionRobotShaken(const Json::Value& config, BEIConditionFactory& factory);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotShaken_H__
