/**
 * File: behaviorReactToRobotOnFace.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: Allows Cozmo to right himself when placed on his face
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnFace.h"

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

static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;
static const float kRobotMinLiftAngleForArmUpAnim_s = 45.f;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotOnFace::BehaviorReactToRobotOnFace(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    RobotInterface::RobotToEngineTag::animEvent
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnFace::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::InitBehavior()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  FlipOverIfNeeded();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::BehaviorUpdate()
{
  if (!IsActivated()) {
    return;
  }
  
  const bool onFace = (GetBEI().GetOffTreadsState() == OffTreadsState::OnFace);
  if (!onFace && _dVars.cancelIfNotOnFace) {
    LOG_WARNING("BehaviorReactToRobotOnFace.BehaviorUpdate.Canceling",
                "Canceling the reaction since the robot is unexpectedly no longer OnFace");
    CancelSelf();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::HandleWhileActivated(const RobotToEngineEvent& event)
{
  const auto& eventData = event.GetData();
  if (eventData.GetTag() == RobotInterface::RobotToEngineTag::animEvent) {
    if (eventData.Get_animEvent().event_id == AnimEvent::FLIP_DOWN_BEGIN) {
      // We are about to begin actually flipping down from OnFace. At this point, we expect the OffTreadsState to
      // transition away from OnFace, so we should not cancel the behavior if the OffTreadState changes.
      _dVars.cancelIfNotOnFace = false;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::FlipOverIfNeeded()
{
  if (GetBEI().GetRobotInfo().IsBeingHeld()) {
    CancelSelf();
  }

  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnFace ) {
    auto& robotInfo = GetBEI().GetRobotInfo();
    
    if (robotInfo.GetCliffSensorComponent().IsCliffDetected()) {
      AnimationTrigger anim;
      if(robotInfo.GetLiftAngle() < kRobotMinLiftAngleForArmUpAnim_s){
        anim = AnimationTrigger::FacePlantRoll;
      }else{
        anim = AnimationTrigger::FacePlantRollArmUp;
      }
      DelegateIfInControl(new TriggerAnimationAction(anim),
                          &BehaviorReactToRobotOnFace::DelayThenCheckState);
    } else {
      const auto cliffs = robotInfo.GetCliffSensorComponent().GetCliffDataRaw();
      PRINT_CH_INFO("Behaviors", "BehaviorReactToRobotOnFace.FlipOverIfNeeded.CalibratingHead",
                       "%d %d %d %d", cliffs[0], cliffs[1], cliffs[2], cliffs[3]);
      DelegateIfInControl(new CalibrateMotorAction(true, false, MotorCalibrationReason::BehaviorReactToOnFace),
                          &BehaviorReactToRobotOnFace::DelayThenCheckState);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::DelayThenCheckState()
{
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnFace ) {
    DelegateIfInControl(new WaitAction(kWaitTimeBeforeRepeatAnim_s),
                        &BehaviorReactToRobotOnFace::CheckFlipSuccess);
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::CheckFlipSuccess()
{
  if(GetBEI().GetOffTreadsState() == OffTreadsState::OnFace) {
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FailedToRightFromFace),
                        &BehaviorReactToRobotOnFace::FlipOverIfNeeded);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorDeactivated()
{
}

} // namespace Vector
} // namespace Anki
