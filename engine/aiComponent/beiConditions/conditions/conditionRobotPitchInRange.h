/**
 * File: conditionRobotPitchInRange.h
 *
 * Author: Guillermo Bautista
 * Created: 2018/12/14
 *
 * Description: Condition that indicates when Vector's pitch falls in a certain range.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionRobotPitchInRange_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionRobotPitchInRange_H__

#include "coretech/common/shared/types.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "coretech/common/shared/math/radians.h"

namespace Anki {
namespace Vector {

class ConditionRobotPitchInRange : public IBEICondition{
public:
  explicit ConditionRobotPitchInRange(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  // Angle thresholds for which robot is considered to be sufficiently pitched
  Radians _minPitchThreshold_rad;
  Radians _maxPitchThreshold_rad;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BeiConditions_Conditions_ConditionRobotPitchInRange_H__
