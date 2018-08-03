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

#include "engine/aiComponent/beiConditions/conditions/conditionUserHoldingCube.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/cubes/cubeInteractionTracker.h"

namespace Anki{
namespace Cozmo{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionUserHoldingCube::ConditionUserHoldingCube(const Json::Value& config)
: IBEICondition(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionUserHoldingCube::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  if(bei.GetCubeInteractionTracker().IsUserHoldingCube()){
    return true;
  }
  return  false;
}

} // namespace Cozmo
} // namespace Anki
