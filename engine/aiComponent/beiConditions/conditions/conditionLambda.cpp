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
namespace Cozmo {

ConditionLambda::ConditionLambda(std::function<bool(BehaviorExternalInterface& bei)> areConditionsMetFunc)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::Lambda))
  , _lambda(areConditionsMetFunc)
{
}

bool ConditionLambda::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return _lambda(behaviorExternalInterface);
}


}
}
