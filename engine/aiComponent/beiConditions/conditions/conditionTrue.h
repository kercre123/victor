/**
* File: strategyTrue.h
*
* Author: Raul - Kevin M. Karol
* Created: 08/10/2016 - 7/5/17
*
* Description: Condition which is always true
*
* Copyright: Anki, Inc. 2016 - 2017
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionTrue_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionTrue_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Vector {

class ConditionTrue : public IBEICondition
{
public:
  // constructor
  explicit ConditionTrue(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override{ return true;}

};
  
} // namespace
} // namespace

#endif // endif __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionTrue_H__
