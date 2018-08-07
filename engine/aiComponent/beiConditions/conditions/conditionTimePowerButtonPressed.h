/**
* File: conditionTimePowerButtonPressed.h
*
* Author: Kevin M. Karol
* Created: 7/19/18
*
* Description: Condition that returns true when the power button has been held down
* for a sufficient length of time
*
* Copyright: Anki, Inc. 2018
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionTimePowerButtonPressed_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionTimePowerButtonPressed_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "coretech/common/shared/types.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Vector {

class ConditionTimePowerButtonPressed : public IBEICondition
{
public:
  // constructor
  explicit ConditionTimePowerButtonPressed(const Json::Value& config);
  ConditionTimePowerButtonPressed(const TimeStamp_t minTimePressed_ms, const std::string& ownerDebugLabel);

protected:
  TimeStamp_t _minTimePressed_ms = 0;

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

};
  
} // namespace
} // namespace

#endif // endif __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionTimePowerButtonPressed_H__
