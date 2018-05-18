/**
* File: conditionCarryingCube
*
* Author: Matt Michini
* Created: 05/17/2018
*
* Description: Condition that is true when the robot is carrying a cube (or at least _thinks_ he is).
*
* Copyright: Anki, Inc. 2018
*
*
**/


#include "engine/aiComponent/beiConditions/conditions/conditionCarryingCube.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/carryingComponent.h"
#include "engine/externalInterface/externalInterface.h"


namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionCarryingCube::ConditionCarryingCube(const Json::Value& config)
: IBEICondition(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionCarryingCube::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  const bool isCarryingCube = robotInfo.GetCarryingComponent().IsCarryingObject();
  return isCarryingCube;
}

  
} // namespace Cozmo
} // namespace Anki
