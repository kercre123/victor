/**
 * File: conditionRobotPickedUp.h
 *
 * Author: Guillermo Bautista
 * Created: 2018/09/05
 *
 * Description: Condition that indicates when Vector has been picked up.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __AiComponent_BeiConditions_ConditionPickedUp__
#define __AiComponent_BeiConditions_ConditionPickedUp__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;

class ConditionRobotPickedUp : public IBEICondition
{
public:
  explicit ConditionRobotPickedUp(const Json::Value& config);
  virtual ~ConditionRobotPickedUp() {};

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
};


} // namespace Vector
} // namespace Anki

#endif // __AiComponent_BeiConditions_ConditionPickedUp__
