/**
* File: conditionEyeContact.cpp
*
* Author: Robert Cosgriff
* Created: 3/8/2018
*
* Description: Strategy for responding to eye contact
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionEyeContact.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(u32, kMaxTimeSinceTrackedFaceUpdated_ms, "Behaviors.ConditionEyeContact", 500);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionEyeContact::ConditionEyeContact(const Json::Value& config)
: IBEICondition(config)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionEyeContact::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return behaviorExternalInterface.GetFaceWorld().IsMakingEyeContact(kMaxTimeSinceTrackedFaceUpdated_ms);
}

u32 ConditionEyeContact::GetMaxTimeSinceTrackedFaceUpdated_ms() {
  return kMaxTimeSinceTrackedFaceUpdated_ms;
}

} // namespace Cozmo
} // namespace Anki
