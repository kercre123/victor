/**
* File: ConditionPetInitialDetection.h
*
* Author: Robert Cosgriff
* Created: 3/8/2018
*
* Description: 
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionEyeContact.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/faceWorld.h"

#define LOG_CHANNEL "BehaviorSystem"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionEyeContact::ConditionEyeContact(const Json::Value& config)
: IBEICondition(config)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionEyeContact::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if (behaviorExternalInterface.GetFaceWorld().IsMakingEyeContact()) {
    PRINT_NAMED_INFO("ConditionEyeContact.AreConditionsMetInternal", " is true");
  }
  return behaviorExternalInterface.GetFaceWorld().IsMakingEyeContact();
}

} // namespace Cozmo
} // namespace Anki
