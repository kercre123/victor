/**
 * File: behaviorReactToRobotOnBack.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-06
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnBack.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"
#include "engine/components/sensors/cliffSensorComponent.h"

#include "util/logging/DAS.h"
#include "clad/types/motorTypes.h"

namespace Anki {
namespace Vector {

using namespace ExternalInterface;

namespace {
  static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;
  const char* const kExitIfHeldKey = "exitIfHeld";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotOnBack::BehaviorReactToRobotOnBack(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _offTreadsCondition = std::make_shared<ConditionOffTreadsState>( OffTreadsState::OnBack, GetDebugLabel() );
  _exitIfHeld = config.get( kExitIfHeldKey, true ).asBool();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnBack::WantsToBeActivatedBehavior() const
{
  return _offTreadsCondition->AreConditionsMet(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::InitBehavior()
{
  _offTreadsCondition->Init(GetBEI());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::OnBehaviorEnteredActivatableScope() {
  _offTreadsCondition->SetActive(GetBEI(), true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::OnBehaviorLeftActivatableScope()
{
  _offTreadsCondition->SetActive(GetBEI(), false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::OnBehaviorActivated()
{
  FlipDownIfNeeded();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert(kExitIfHeldKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::FlipDownIfNeeded()
{
  if ( _exitIfHeld && GetBEI().GetRobotInfo().IsBeingHeld()) {
    CancelSelf();
  }

  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnBack ) {
    const auto& robotInfo = GetBEI().GetRobotInfo();
    // Check if cliff detected
    // If not, then calibrate head because we're not likely to be on back if no cliff detected.
    if (robotInfo.GetCliffSensorComponent().IsCliffDetected()) {
      AnimationTrigger anim = AnimationTrigger::FlipDownFromBack;

      DelegateIfInControl(new TriggerAnimationAction(anim),
                  &BehaviorReactToRobotOnBack::DelayThenFlipDown);
    } else {
      const auto cliffs = robotInfo.GetCliffSensorComponent().GetCliffDataRaw();
      PRINT_CH_INFO("Behaviors", "BehaviorReactToRobotOnBack.FlipDownIfNeeded.CalibratingHead", "%d %d %d %d", cliffs[0], cliffs[1], cliffs[2], cliffs[3]);
      DelegateIfInControl(new CalibrateMotorAction(true, false, EnumToString(MotorCalibrationReason::BehaviorReactToOnBack)),
                  &BehaviorReactToRobotOnBack::DelayThenFlipDown);
    }
  }
  else {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnBack);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::DelayThenFlipDown()
{
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnBack ) {
    DelegateIfInControl(new WaitAction(kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnBack::FlipDownIfNeeded);
  }
  else {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnBack);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::OnBehaviorDeactivated()
{
}

} // namespace Vector
} // namespace Anki
