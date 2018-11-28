/**
 * File: conditionInCalmMode.h
 *
 * Author: Guillermo Bautista
 * Created: 2018/11/26
 *
 * Description: Condition that indicates when Vector's SysCon is in Calm Mode.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __AiComponent_BeiConditions_ConditionInCalmMode__
#define __AiComponent_BeiConditions_ConditionInCalmMode__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;

class ConditionInCalmMode : public IBEICondition
{
public:
  explicit ConditionInCalmMode(const Json::Value& config);

  virtual ~ConditionInCalmMode() {};

protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override {}
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

};


} // namespace Vector
} // namespace Anki

#endif // __AiComponent_BeiConditions_ConditionInCalmMode__
