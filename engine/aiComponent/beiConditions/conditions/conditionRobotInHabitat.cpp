/**
 * File: conditionRobotInHabitat
 *
 * Author: Arjun Menon
 * Created: 07-30-18
 *
 * Description: 
 * condition which returns true if the robot is in the habitat, false otherwise
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionRobotInHabitat.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/habitatDetectorComponent.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionRobotInHabitat::ConditionRobotInHabitat(const Json::Value& config)
: IBEICondition(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionRobotInHabitat::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return behaviorExternalInterface.GetHabitatDetectorComponent().GetHabitatBeliefState()==HabitatBeliefState::InHabitat;
}
  
void ConditionRobotInHabitat::BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const
{
}
 
  

} // namespace
} // namespace

