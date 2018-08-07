/**
 * File: strategyLambda.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-11-30
 *
 * Description: A StateConceptStrategy that is implemented as a lmabda. This one cannot be created (solely)
 *              from config, it must be created in code. This can be useful to use the existing infrastructure
 *              and framework with a custom hard-coded strategy
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionLambda.h"

namespace Anki {
namespace Vector {

ConditionLambda::ConditionLambda(std::function<bool(BehaviorExternalInterface& bei)> areConditionsMetFunc,
                                 const std::string& ownerDebugLabel)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::Lambda))
  , _lambda(areConditionsMetFunc)
{
  SetOwnerDebugLabel(ownerDebugLabel);
}

ConditionLambda::ConditionLambda(std::function<bool(BehaviorExternalInterface& bei)> areConditionsMetFunc,
                                 const VisionModeSet& requiredVisionModes,
                                 const std::string& ownerDebugLabel)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::Lambda))
  , _lambda(areConditionsMetFunc)
  , _requiredVisionModes(requiredVisionModes)
{
  SetOwnerDebugLabel(ownerDebugLabel);
}

ConditionLambda::ConditionLambda(std::function<bool(BehaviorExternalInterface& bei)> areConditionsMetFunc,
                                 std::function<void(BehaviorExternalInterface& bei, bool setActive)> setActiveFunc,
                                 const std::string& ownerDebugLabel)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::Lambda))
  , _lambda(areConditionsMetFunc)
  , _setActiveFunc(setActiveFunc)
{
  SetOwnerDebugLabel(ownerDebugLabel);
}

ConditionLambda::ConditionLambda(std::function<bool(BehaviorExternalInterface& bei)> areConditionsMetFunc,
                                 std::function<void(BehaviorExternalInterface& bei, bool setActive)> setActiveFunc,
                                 const VisionModeSet& requiredVisionModes,
                                 const std::string& ownerDebugLabel)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::Lambda))
  , _lambda(areConditionsMetFunc)
  , _setActiveFunc(setActiveFunc)
  , _requiredVisionModes(requiredVisionModes)
{
  SetOwnerDebugLabel(ownerDebugLabel);
}

void ConditionLambda::SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool setActive)
{
  if(_setActiveFunc){
    _setActiveFunc(behaviorExternalInterface, setActive);
  }
}

void ConditionLambda::GetRequiredVisionModes(VisionModeSet& requests) const
{
  if(!_requiredVisionModes.empty()){
    requests = _requiredVisionModes;
  }
}

bool ConditionLambda::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return _lambda(behaviorExternalInterface);
}


}
}
