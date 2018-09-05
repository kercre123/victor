/**
 * File: conditionIsNightTime.h
 *
 * Author: Brad Neuman
 * Created: 2018-09-04
 *
 * Description: Checks with the sleep tracker if it is currently considered to be night time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionIsNightTime_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionIsNightTime_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionIsNightTime : public IBEICondition
{
public:
  
  explicit ConditionIsNightTime(const Json::Value& config);

protected:
  
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
};
  
} // namespace
} // namespace


#endif
