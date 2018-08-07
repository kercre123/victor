/**
 * File: conditionUnexpectedMovement
 *
 * Author: Matt Michini
 * Created: 4/10/18
 *
 * Description: Condition which is true when the robot detects unexpected movement (e.g.
 *              driving into an object and not following the expected path)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionUnexpectedMovement.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/movementComponent.h"
#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionUnexpectedMovement::ConditionUnexpectedMovement(const Json::Value& config)
: IBEICondition(config)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionUnexpectedMovement::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return behaviorExternalInterface.GetMovementComponent().IsUnexpectedMovementDetected();
}
 
  

} // namespace
} // namespace
