/**
* File: conditionStare.cpp
*
* Author: Robert Cosgriff
* Created: 7/17/2018
*
* Description: see header
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionStare.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(u32, kMaxTimeSinceTrackedFaceUpdatedStare_ms, "Behaviors.ConditionStare", 200);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionStare::ConditionStare(const Json::Value& config)
: IBEICondition(config)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionStare::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return behaviorExternalInterface.GetFaceWorld().IsStaring(kMaxTimeSinceTrackedFaceUpdatedStare_ms);
}

u32 ConditionStare::GetMaxTimeSinceTrackedFaceUpdated_ms() {
  return kMaxTimeSinceTrackedFaceUpdatedStare_ms;
}

} // namespace Cozmo
} // namespace Anki
