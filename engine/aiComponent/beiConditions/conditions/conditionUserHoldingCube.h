/**
 * File: conditionUserHoldingCube.h
 *
 * Author: Sam Russell
 * Created: 2018 Jul 25
 *
 * Description: Condition which checks with the CubeInteractionTracker if we think the user is holding the cube right now
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionUserHoldingCube_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionUserHoldingCube_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionUserHoldingCube : public IBEICondition
{
public:
  explicit ConditionUserHoldingCube(const Json::Value& config);
  virtual ~ConditionUserHoldingCube() = default;

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;

};


} // namespace Cozmo
} // namespace Anki


#endif // __Engine_AiComponent_BeiConditions_Conditions_ConditionUserHoldingCube_H__
