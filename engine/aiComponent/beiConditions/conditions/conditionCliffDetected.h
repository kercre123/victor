/**
 * File: conditionCliffDetected.h
 *
 * Author: Matt Michini
 * Created: 2018/02/21
 *
 * Description: Condition that indicates when victor stops at a detected cliff
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __AiComponent_BeiConditions_ConditionCliffDetected__
#define __AiComponent_BeiConditions_ConditionCliffDetected__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;

class ConditionCliffDetected : public IBEICondition
{
public:
  explicit ConditionCliffDetected(const Json::Value& config);

  virtual ~ConditionCliffDetected() {};

protected:

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
private:

};


} // namespace Vector
} // namespace Anki

#endif // __AiComponent_BeiConditions_ConditionCliffDetected__
