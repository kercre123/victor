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
#include "engine/components/sensors/cliffSensorComponent.h"

#include "util/logging/DAS.h"
#include "clad/robotInterface/messageRobotToEngine.h"
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
  _iConfig.exitIfHeld = config.get( kExitIfHeldKey, true ).asBool();
  
  SubscribeToTags({
    RobotInterface::RobotToEngineTag::animEvent
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnBack::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::InitBehavior()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  FlipDownIfNeeded();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert(kExitIfHeldKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::BehaviorUpdate()
{
  if (!IsActivated()) {
    return;
  }
  
  const bool onBack = (GetBEI().GetOffTreadsState() == OffTreadsState::OnBack);
  if (!onBack && _dVars.cancelIfNotOnBack) {
    LOG_WARNING("BehaviorReactToRobotOnBack.BehaviorUpdate.Canceling",
                "Canceling the reaction since the robot is unexpectedly no longer OnBack");
    CancelSelf();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::HandleWhileActivated(const RobotToEngineEvent& event)
{
  const auto& eventData = event.GetData();
  if (eventData.GetTag() == RobotInterface::RobotToEngineTag::animEvent) {
    if (eventData.Get_animEvent().event_id == AnimEvent::FLIP_DOWN_BEGIN) {
      // We are about to begin actually flipping down from OnBack. At this point, we expect the OffTreadsState to
      // transition away from OnBack, so we should not cancel the behavior if the OffTreadState changes.
      _dVars.cancelIfNotOnBack = false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::FlipDownIfNeeded()
{
  if ( _iConfig.exitIfHeld && GetBEI().GetRobotInfo().IsBeingHeld()) {
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
      DelegateIfInControl(new CalibrateMotorAction(true, false, MotorCalibrationReason::BehaviorReactToOnBack),
                          &BehaviorReactToRobotOnBack::DelayThenFlipDown);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::DelayThenFlipDown()
{
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnBack ) {
    DelegateIfInControl(new WaitAction(kWaitTimeBeforeRepeatAnim_s),
                        &BehaviorReactToRobotOnBack::FlipDownIfNeeded);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::OnBehaviorDeactivated()
{
}

} // namespace Vector
} // namespace Anki
