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
#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"
#include "engine/components/sensors/cliffSensorComponent.h"

#include "util/logging/DAS.h"
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
  _offTreadsCondition = std::make_shared<ConditionOffTreadsState>( OffTreadsState::OnFace, GetDebugLabel() );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnFace::WantsToBeActivatedBehavior() const
{
  return _offTreadsCondition->AreConditionsMet(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::InitBehavior()
{
  _offTreadsCondition->Init(GetBEI());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorEnteredActivatableScope() {
  _offTreadsCondition->SetActive(GetBEI(), true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorLeftActivatableScope()
{
  _offTreadsCondition->SetActive(GetBEI(), false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorActivated()
{
  FlipOverIfNeeded();
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
      PRINT_NAMED_INFO("BehaviorReactToRobotOnFace.FlipOverIfNeeded.CalibratingHead",
                       "%d %d %d %d", cliffs[0], cliffs[1], cliffs[2], cliffs[3]);
      DelegateIfInControl(new CalibrateMotorAction(true, false, EnumToString(MotorCalibrationReason::BehaviorReactToOnFace)),
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
  }else{
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnFace);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorDeactivated()
{
}

} // namespace Vector
} // namespace Anki
