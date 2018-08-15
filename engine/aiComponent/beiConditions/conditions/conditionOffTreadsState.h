/**
 * File: conditionOffTreadsState.h
 *
 * Author: Brad Neuman
 * Created: 2018-01-22
 *
 * Description: Checks if the off treads state matches the one supplied in config
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionOffTreadsState_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionOffTreadsState_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "clad/types/offTreadsStates.h"

namespace Anki {
namespace Vector {

class ConditionOffTreadsState : public IBEICondition
{
public:
  explicit ConditionOffTreadsState(const Json::Value& config);
  explicit ConditionOffTreadsState(const OffTreadsState& targetState, const std::string& ownerDebugLabel);
  
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;

private:
  OffTreadsState _targetState;
  int _minTimeSinceChange_ms;
  int _maxTimeSinceChange_ms; // ignored if negative
  
};

}
}


#endif
