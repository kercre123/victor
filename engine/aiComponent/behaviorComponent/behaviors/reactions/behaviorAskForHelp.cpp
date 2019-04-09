/**
 * File: BehaviorAskForHelp.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2018-09-04
 *
 * Description: Behavior that periodically plays a "distressed" animation because
 *              he's stuck and needs help from the user to get out of his situation.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorAskForHelp.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Vector {

namespace {
  const u8 kTracksToLock = (u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::LIFT_TRACK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAskForHelp::DynamicVariables::DynamicVariables()
{
  getInTrigger = AnimationTrigger::StuckOnEdgeGetIn;
  idleTrigger  = AnimationTrigger::StuckOnEdgeIdle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAskForHelp::BehaviorAskForHelp(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // Read config into _iConfig here if necessary.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskForHelp::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  SetAnimTriggers();
  TriggerGetInAnim();
  ICozmoBehavior::SmartRequestPowerSaveMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskForHelp::SetAnimTriggers()
{
  _dVars.getInTrigger = AnimationTrigger::StuckOnEdgeGetIn;
  _dVars.idleTrigger  = AnimationTrigger::StuckOnEdgeIdle;

  // Check if a cliff is detected on either side of ther robot in the location where we're stuck.
  const auto& cliffComp = GetBEI().GetRobotInfo().GetCliffSensorComponent();
  bool leftCliffs = cliffComp.IsCliffDetected(CliffSensor::CLIFF_FL) && cliffComp.IsCliffDetected(CliffSensor::CLIFF_BL);
  bool rightCliffs = cliffComp.IsCliffDetected(CliffSensor::CLIFF_FR) && cliffComp.IsCliffDetected(CliffSensor::CLIFF_BR);

  // Only pick a "sided" animation if we're seeing cliffs on one side only
  if (leftCliffs != rightCliffs) {
    if (leftCliffs) {
      _dVars.getInTrigger = AnimationTrigger::StuckOnEdgeLeftGetIn;
      _dVars.idleTrigger  = AnimationTrigger::StuckOnEdgeLeftIdle;
    } else {
      _dVars.getInTrigger = AnimationTrigger::StuckOnEdgeRightGetIn;
      _dVars.idleTrigger  = AnimationTrigger::StuckOnEdgeRightIdle;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskForHelp::TriggerGetInAnim()
{
  PRINT_CH_INFO("Behaviors", "BehaviorAskForHelp.TriggerGetInAnim", "%s", EnumToString(_dVars.getInTrigger));
  auto action = new TriggerAnimationAction(_dVars.getInTrigger, 1, true, kTracksToLock);
  DelegateIfInControl(action, &BehaviorAskForHelp::TriggerIdleAnim);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskForHelp::TriggerIdleAnim()
{
  PRINT_CH_INFO("Behaviors", "BehaviorAskForHelp.TriggerIdleAnim", "%s", EnumToString(_dVars.idleTrigger));
  auto action = new TriggerAnimationAction(_dVars.idleTrigger, 1, true, kTracksToLock);
  DelegateIfInControl(action, &BehaviorAskForHelp::TriggerIdleAnim);
}

}
}
