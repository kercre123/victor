/**
 * File: conditionBeingHeld.h
 *
 * Author: Kevin Yoon
 * Created: 2018-08-15
 *
 * Description: Checks if the IS_BEING_HELD state matches the one supplied in config
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionBeingHeld_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionBeingHeld_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionBeingHeld : public IBEICondition
{
public:
  explicit ConditionBeingHeld(const Json::Value& config);
  explicit ConditionBeingHeld(const bool shouldBeHeld, const std::string& ownerDebugLabel);
  
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;

private:
  bool _shouldBeHeld;
  
};

}
}


#endif
