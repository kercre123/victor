/**
 * File: conditionRobotPoked.h
 *
 * Author: Guillermo Bautista
 * Created: 2018/08/21
 *
 * Description: Condition that indicates when Victor has been poked recently
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionRobotPoked_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionRobotPoked_H__

#include "coretech/common/shared/types.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionRobotPoked : public IBEICondition{
public:
  explicit ConditionRobotPoked(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  // Time threshold for which robot is considered to have been recently "Poked"
  u32 _wasPokedRecentlyTimeThreshold_ms = 0;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BeiConditions_Conditions_ConditionRobotPoked_H__
