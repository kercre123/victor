/**
 * File: conditionRobotRollInRange.h
 *
 * Author: Guillermo Bautista
 * Created: 2019/02/27
 *
 * Description: Condition that indicates when Vector's roll falls in a certain range.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionRobotRollInRange_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionRobotRollInRange_H__

#include "coretech/common/shared/types.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "coretech/common/shared/math/radians.h"

namespace Anki {
namespace Vector {

class ConditionRobotRollInRange : public IBEICondition{
public:
  explicit ConditionRobotRollInRange(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  // Angle thresholds for which robot is considered to be sufficiently rolled
  Radians _minRollThreshold_rad;
  Radians _maxRollThreshold_rad;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BeiConditions_Conditions_ConditionRobotRollInRange_H__
