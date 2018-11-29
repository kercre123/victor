/**
 * File: conditionAlexaInteractionActive.h
 *
 * Author: Brad Neuman
 * Created: 2018-11-28
 *
 * Description: Condition that returns true if Alexa is doing something (speaking, listening, etc.). Will
 *              never return true if alexa isn't authorized / signed in
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionAlexaInteractionActive_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionAlexaInteractionActive_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionAlexaInteractionActive : public IBEICondition
{
public:
  explicit ConditionAlexaInteractionActive(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

};

} // namespace
} // namespace


#endif
