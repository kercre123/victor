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

#include "engine/aiComponent/beiConditions/conditions/conditionFaceNormalDirectedAtRobot.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {

CONSOLE_VAR(u32, kMaxTimeSinceTrackedFaceUpdatedFIXME_ms, "Behaviors.ConditionEyeContact", 500);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionFaceNormalAtRobot::ConditionFaceNormalAtRobot(const Json::Value& config)
: IBEICondition(config)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionFaceNormalAtRobot::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return behaviorExternalInterface.GetFaceWorld().IsFaceDirectedAtRobot(kMaxTimeSinceTrackedFaceUpdatedFIXME_ms);
}

u32 ConditionFaceNormalAtRobot::GetMaxTimeSinceTrackedFaceUpdated_ms() {
  return kMaxTimeSinceTrackedFaceUpdatedFIXME_ms;
}

} // namespace Vector
} // namespace Anki
